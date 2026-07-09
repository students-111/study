/**
 * @file bsp_board.h
 * @brief 板级 BSP 初始化与空闲入口。
 */

#ifndef BSP_BOARD_H
#define BSP_BOARD_H

/* ======== include ======== */

#include "bsp_status.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 公开 API ======== */

/**
 * @brief 初始化板级时基、UART 和 I2C BSP。
 * @param void 无参数。
 * @return 成功返回 BSP_OK，否则返回 BSP 错误码。
 */
bsp_status_e bsp_board_init(void);

/**
 * @brief 进入低功耗空闲状态，等待已使能中断唤醒 CPU。
 * @param void 无参数。
 * @return 无。
 */
void bsp_board_idle(void);

#endif /* BSP_BOARD_H */
