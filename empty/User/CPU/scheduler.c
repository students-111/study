/**
 * @file scheduler.c
 * @brief 裸机协作式周期任务调度器实现。
 */

/* ======== include ======== */

#include "scheduler.h"

#include <stdbool.h>
#include <stddef.h>

#include "app.h"
#include "bsp_board.h"
#include "dal_encoder.h"
#include "dal_gray.h"
#include "bsp_time.h"
#include "dal_key.h"
#include "dal_mpu6050.h"
#include "ti_msp_dl_config.h"

/* ======== 类型定义 ======== */

typedef void (*scheduler_task_fn_t)(void);

typedef struct {
    scheduler_task_fn_t run;    /**< 周期运行函数。 */
    uint32_t period_ms;         /**< 调度周期，单位 ms。 */
    uint32_t last_run_ms;       /**< 上次运行时刻，单位 ms。 */
} scheduler_task_t;

/* ======== 全局实例 ======== */

volatile uint32_t uwTick = 0;
extern uint32_t g_bsp_time_ms;

/* ======== 内部变量 ======== */

static scheduler_task_t g_scheduler_tasks[] = {
    {dal_gray_refresh, APP_PERIOD_MS, 0U},
    {dal_encoder_update_speed, APP_PERIOD_MS, 0U},
    {dal_key_refresh, APP_PERIOD_MS, 0U},
    {dal_mpu6050_refresh, APP_PERIOD_MS, 0U},
    {app_refresh, APP_PERIOD_MS, 0U},
};

/* ======== 内部函数声明 ======== */

/**
 * @brief 判断周期任务是否到达本次调度时刻。
 * @param task 待检查的任务描述。
 * @param now_ms 当前 SysTick 时刻，单位 ms。
 * @return 已到期返回 true，否则返回 false。
 */
static bool scheduler_task_is_due(const scheduler_task_t *task, uint32_t now_ms);

/**
 * @brief 在启动阶段不可恢复错误发生时停机等待调试。
 * @param void 无参数。
 * @return 无。
 */
static void scheduler_fault_trap(void);

/* ======== 内部函数 ======== */

/**
 * @brief 判断周期任务是否到达本次调度时刻。
 * @param task 待检查的任务描述。
 * @param now_ms 当前 SysTick 时刻，单位 ms。
 * @return 已到期返回 true，否则返回 false。
 */
static bool scheduler_task_is_due(const scheduler_task_t *task, uint32_t now_ms)
{
    return ((task != NULL) &&
        ((uint32_t)(now_ms - task->last_run_ms) >= task->period_ms));
}

/**
 * @brief 在启动阶段不可恢复错误发生时停机等待调试。
 * @param void 无参数。
 * @return 无。
 */
static void scheduler_fault_trap(void)
{
    while (1) {
    }
}

/* ======== 公开 API ======== */

void scheduler_init(void)
{
    SYSCFG_DL_init();

    if (bsp_board_init() != BSP_OK) {
        scheduler_fault_trap();
    }

    dal_gray_init();
    dal_encoder_init();
    dal_key_init();
    dal_mpu6050_init();
    app_init();
}

void scheduler_run(void)
{
    uint32_t task_index;
    uint32_t now_ms;
    uint32_t task_count = (uint32_t)(sizeof(g_scheduler_tasks) /
        sizeof(g_scheduler_tasks[0]));

			while (1) {
        now_ms = g_bsp_time_ms;

        for (task_index = 0U; task_index < task_count; task_index++) {
            if (scheduler_task_is_due(&g_scheduler_tasks[task_index], now_ms)) {
                g_scheduler_tasks[task_index].last_run_ms = now_ms;
                g_scheduler_tasks[task_index].run();
            }
        }

        bsp_board_idle();
    }
}

void scheduler_delay_ms(uint32_t delay_ms)
{
    uint32_t start_tick = g_bsp_time_ms;

    while ((uint32_t)(g_bsp_time_ms - start_tick) < delay_ms) {
    }
}
