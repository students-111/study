/**
 * @file bsp_time.c
 * @brief BSP 毫秒时基实现。
 */

/* ======== include ======== */

#include "bsp_time.h"

#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 内部变量 ======== */

static volatile uint32_t g_bsp_time_ms = 0U;

/* ======== 公开 API ======== */

bsp_status_e bsp_time_init(void)
{
    g_bsp_time_ms = 0U;

    if (SysTick_Config(CPUCLK_FREQ / BSP_TIME_SYSTICK_HZ) != 0U) {
        return BSP_ERR_HW;
    }

    NVIC_SetPriority(SysTick_IRQn, BSP_TIME_SYSTICK_PRIORITY);
    return BSP_OK;
}

uint32_t bsp_time_get_ms(void)
{
    return g_bsp_time_ms;
}

void SysTick_Handler(void)
{
    /* WHY: 时基中断只递增计数，避免 ISR 内引入业务延迟。 */
    g_bsp_time_ms++;
}
