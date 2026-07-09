/**
 * @file bsp_time.h
 * @brief BSP 毫秒时基接口。
 */

#ifndef BSP_TIME_H
#define BSP_TIME_H

/* ======== include ======== */

#include <stdint.h>

#include "bsp_status.h"

/* ======== 可调参数宏定义 ======== */

/* SysTick 频率，单位 Hz；影响全系统毫秒时基精度。 */
#define BSP_TIME_SYSTICK_HZ            (1000U)

/* SysTick 中断优先级；需低于 I2C/UART 等实时外设中断。 */
#define BSP_TIME_SYSTICK_PRIORITY      (2U)

/* ======== 公开 API ======== */

/**
 * @brief 初始化 1 ms SysTick 时基。
 * @param void 无参数。
 * @return 成功返回 BSP_OK，否则返回 BSP_ERR_HW。
 */
bsp_status_e bsp_time_init(void);

/**
 * @brief 获取当前毫秒计数。
 * @param void 无参数。
 * @return uint32_t 毫秒计数，允许自然溢出。
 */
uint32_t bsp_time_get_ms(void);

#endif /* BSP_TIME_H */
