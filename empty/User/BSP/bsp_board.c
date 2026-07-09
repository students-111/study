/**
 * @file bsp_board.c
 * @brief 板级 BSP 初始化与空闲入口实现。
 */

/* ======== include ======== */

#include "bsp_board.h"

#include "bsp_i2c.h"
#include "bsp_time.h"
#include "bsp_uart.h"
#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 公开 API ======== */

bsp_status_e bsp_board_init(void)
{
    if (bsp_time_init() != BSP_OK) {
        return BSP_ERR_HW;
    }
    if (bsp_uart_init() != BSP_OK) {
        return BSP_ERR_HW;
    }
    if (bsp_i2c_init() != BSP_OK) {
        return BSP_ERR_HW;
    }
    return BSP_OK;
}

void bsp_board_idle(void)
{
    /* WHY: SLEEP0 下外设继续运行，主循环无任务时可降低空转功耗。 */
    __WFI();
}
