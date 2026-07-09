/**
 * @file cpu_scheduler.c
 * @brief 裸机协作式任务调度器实现。
 */

/* ======== include ======== */

#include "cpu_scheduler.h"

#include <stddef.h>
#include <stdint.h>

#include "MultiTimer.h"
#include "app_drive_mode.h"
#include "bsp_board.h"
#include "bsp_time.h"
#include "dal_encoder.h"
#include "dal_gray.h"
#include "dal_key.h"
#include "dal_mpu6050.h"
#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

typedef void (*cpu_task_fn_t)(void);

typedef struct {
    cpu_task_id_e id;          /**< CPU 任务编号。 */
    const char *name;          /**< 任务名称，仅用于调试识别。 */
    cpu_task_fn_t init;        /**< 任务初始化函数。 */
    cpu_task_fn_t run;         /**< 任务周期运行函数。 */
    uint64_t period_ms;        /**< 任务周期，单位 ms。 */
    bool enabled;              /**< 任务是否允许被调度。 */
    multi_timer_t timer;       /**< 任务使用的软件定时器。 */
} cpu_task_slot_t;

/* ======== 内部函数声明 ======== */

/**
 * @brief 软件定时器到期后的任务派发回调。
 * @param timer 触发回调的软件定时器。
 * @param user_data 任务槽指针。
 * @return 无。
 */
static void cpu_task_timer_callback(multi_timer_t *timer, void *user_data);

/**
 * @brief 获取 MultiTimer 使用的平台 tick。
 * @param void 无参数。
 * @return 当前毫秒 tick。
 */
static uint64_t cpu_platform_ticks(void);

/**
 * @brief 启动阶段不可恢复错误停机点。
 * @param void 无参数。
 * @return 无。
 */
static void cpu_fault_trap(void);

/**
 * @brief 启动单个任务的软件定时器。
 * @param task 任务槽指针。
 * @return 无。
 */
static void cpu_task_start_timer(cpu_task_slot_t *task);

/**
 * @brief 判断 CPU 任务编号是否合法。
 * @param id CPU 任务编号。
 * @return 合法返回 true，否则返回 false。
 */
static bool cpu_task_id_is_valid(cpu_task_id_e id);

/* ======== 全局实例 ======== */

static cpu_task_slot_t g_cpu_tasks[CPU_TASK_COUNT] = {
    {
        .id = CPU_TASK_GRAY,
        .name = "gray",
        .init = dal_gray_init,
        .run = dal_gray_refresh,
        .period_ms = CPU_TASK_GRAY_PERIOD_MS,
        .enabled = true,
    },
    {
        .id = CPU_TASK_MPU6050,
        .name = "mpu6050",
        .init = dal_mpu6050_init,
        .run = dal_mpu6050_refresh,
        .period_ms = CPU_TASK_MPU6050_PERIOD_MS,
        .enabled = true,
    },
    {
        .id = CPU_TASK_ENCODER_SAMPLE,
        .name = "encoder_sample",
        .init = dal_encoder_init,
        .run = dal_encoder_refresh,
        .period_ms = CPU_TASK_ENCODER_SAMPLE_PERIOD_MS,
        .enabled = true,
    },
    {
        .id = CPU_TASK_KEY,
        .name = "key",
        .init = dal_key_init,
        .run = dal_key_refresh,
        .period_ms = CPU_TASK_KEY_PERIOD_MS,
        .enabled = true,
    },
    {
        .id = CPU_TASK_DRIVE_MODE,
        .name = "drive_mode",
        .init = app_drive_mode_init,
        .run = app_drive_mode_refresh,
        .period_ms = CPU_TASK_DRIVE_MODE_PERIOD_MS,
        .enabled = true,
    },
};

/* ======== 内部函数 ======== */

/**
 * @brief 获取 BSP 毫秒时基作为调度 tick。
 * @param void 无参数。
 * @return 当前毫秒 tick。
 */
static uint64_t cpu_platform_ticks(void)
{
    return (uint64_t)bsp_time_get_ms();
}

/**
 * @brief 在不可恢复错误发生时停机等待调试器。
 * @param void 无参数。
 * @return 无。
 */
static void cpu_fault_trap(void)
{
    /* WHY: 启动阶段不可恢复错误停在这里，方便调试器查看现场。 */
    while (1) {
    }
}

/**
 * @brief 按任务配置启动或重启任务定时器。
 * @param task 任务槽指针。
 * @return 无。
 */
static void cpu_task_start_timer(cpu_task_slot_t *task)
{
    if ((task != NULL) && task->enabled && (task->run != NULL) &&
        (task->period_ms > 0ULL)) {
        (void)multi_timer_start(&task->timer, task->period_ms,
            cpu_task_timer_callback, task);
    }
}

/**
 * @brief 执行到期任务并重新安排下一次运行。
 * @param timer 触发回调的软件定时器。
 * @param user_data 任务槽指针。
 * @return 无。
 */
static void cpu_task_timer_callback(multi_timer_t *timer, void *user_data)
{
    cpu_task_slot_t *task = (cpu_task_slot_t *)user_data;

    (void)timer;

    /*
     * WHY: 回调由 cpu_run() 中的 multi_timer_yield() 触发，仍属于主循环上下文。
     */
    if ((task != NULL) && task->enabled && (task->run != NULL)) {
        task->run();
        cpu_task_start_timer(task);
    }
}

/**
 * @brief 判断 CPU 任务编号是否合法。
 * @param id CPU 任务编号。
 * @return 合法返回 true，否则返回 false。
 */
static bool cpu_task_id_is_valid(cpu_task_id_e id)
{
    return ((unsigned int)id < (unsigned int)CPU_TASK_COUNT);
}

/* ======== 公开 API ======== */

void cpu_init(void)
{
    unsigned int idx;

    SYSCFG_DL_init();

    if (bsp_board_init() != BSP_OK) {
        cpu_fault_trap();
    }

    if (multi_timer_install(cpu_platform_ticks) != MULTI_TIMER_OK) {
        cpu_fault_trap();
    }

    for (idx = 0U; idx < (unsigned int)CPU_TASK_COUNT; idx++) {
        if (g_cpu_tasks[idx].init != NULL) {
            g_cpu_tasks[idx].init();
        }
        cpu_task_start_timer(&g_cpu_tasks[idx]);
    }
}

void cpu_run(void)
{
    while (1) {
        (void)multi_timer_yield();
        bsp_board_idle();
    }
}

bool cpu_task_set_enabled(cpu_task_id_e id, bool enabled)
{
    cpu_task_slot_t *task;

    if (!cpu_task_id_is_valid(id)) {
        return false;
    }

    task = &g_cpu_tasks[id];
    task->enabled = enabled;

    if (enabled) {
        cpu_task_start_timer(task);
    } else {
        (void)multi_timer_stop(&task->timer);
    }

    return true;
}

bool cpu_task_is_enabled(cpu_task_id_e id)
{
    if (!cpu_task_id_is_valid(id)) {
        return false;
    }

    return g_cpu_tasks[id].enabled;
}
