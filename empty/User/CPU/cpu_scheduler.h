/**
 * @file cpu_scheduler.h
 * @brief 裸机协作式任务调度器接口。
 */

#ifndef CPU_SCHEDULER_H
#define CPU_SCHEDULER_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

#include "app_drive_mode.h"

/* ======== 可调参数宏定义 ======== */

/* 灰度巡线采样任务周期，单位 ms；影响线位更新频率。 */
#define CPU_TASK_GRAY_PERIOD_MS                (1ULL)

/* MPU6050 状态机推进周期，单位 ms；需足够快以推进 I2C 异步事务。 */
#define CPU_TASK_MPU6050_PERIOD_MS             (1ULL)

/* 编码器 GPIO 软件解码采样周期，单位 ms；影响低速计数分辨率。 */
#define CPU_TASK_ENCODER_SAMPLE_PERIOD_MS      (1ULL)

/* 功能按键消抖采样周期，单位 ms；应小于 APP 模式切换任务周期。 */
#define CPU_TASK_KEY_PERIOD_MS                 (10ULL)

/* 行车模式任务周期，单位 ms；复用 APP 层模式管理周期。 */
#define CPU_TASK_DRIVE_MODE_PERIOD_MS          (APP_DRIVE_MODE_PERIOD_MS)

/* ======== 类型定义 ======== */

/**
 * @brief CPU 调度任务编号。
 */
typedef enum {
    CPU_TASK_GRAY = 0,              /**< 灰度传感器刷新任务。 */
    CPU_TASK_MPU6050,               /**< MPU6050 状态机刷新任务。 */
    CPU_TASK_ENCODER_SAMPLE,        /**< 编码器软件解码刷新任务。 */
    CPU_TASK_KEY,                   /**< 功能按键消抖刷新任务。 */
    CPU_TASK_DRIVE_MODE,            /**< 行车模式管理任务。 */
    CPU_TASK_COUNT                  /**< CPU 任务数量。 */
} cpu_task_id_e;

/* ======== 公开 API ======== */

/**
 * @brief 初始化芯片配置、BSP、MultiTimer 和任务表。
 * @param void 无参数。
 * @return 无。
 */
void cpu_init(void);

/**
 * @brief 运行裸机协作式调度器。
 * @param void 无参数。
 * @return 无；设计上不返回。
 */
void cpu_run(void);

/**
 * @brief 运行时使能或禁用指定任务。
 * @param id CPU 任务编号。
 * @param enabled true 表示使能，false 表示禁用。
 * @return 任务编号合法返回 true，否则返回 false。
 */
bool cpu_task_set_enabled(cpu_task_id_e id, bool enabled);

/**
 * @brief 查询指定任务是否处于使能状态。
 * @param id CPU 任务编号。
 * @return 任务使能返回 true，任务非法或禁用返回 false。
 */
bool cpu_task_is_enabled(cpu_task_id_e id);

#endif /* CPU_SCHEDULER_H */
