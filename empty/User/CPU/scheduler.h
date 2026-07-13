/**
 * @file scheduler.h
 * @brief 裸机协作式周期任务调度器接口。
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

/* ======== include ======== */

#include <stdint.h>

/* ======== 全局实例 ======== */

extern volatile uint32_t uwTick;

/* ======== 公开 API ======== */

/**
 * @brief 初始化芯片配置、BSP、DAL/APP 初始化任务和调度器时基。
 * @param void 无参数。
 * @return 无。
 */
void scheduler_init(void);

/**
 * @brief 运行裸机协作式调度器主循环。
 * @param void 无参数。
 * @return 无；设计上不返回。
 */
void scheduler_run(void);

/**
 * @brief 基于 SysTick 毫秒时基进行阻塞延时。
 * @param delay_ms 延时时长，单位 ms。
 * @return 无。
 */
void scheduler_delay_ms(uint32_t delay_ms);

#endif /* SCHEDULER_H */
