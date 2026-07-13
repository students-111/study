/**
 * @file bsp_time.c
 * @brief BSP 毫秒时基实现。
 */

/* ======== include ======== */

#include "bsp_time.h"
#include "scheduler.h"
#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 内部变量 ======== */

 volatile uint32_t g_bsp_time_ms = 0U;

/* ======== 公开 API ======== */

bsp_status_e bsp_time_init(void)
{
    g_bsp_time_ms = 0U;

    /* WHY: SysConfig 以 80 MHz CPUCLK 生成 80,000 计数的 1 ms 周期，BSP 不重复覆盖。 */
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
    uwTick++;

}
