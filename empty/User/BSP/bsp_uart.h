/**
 * @file bsp_uart.h
 * @brief UART0 异步收发与调试打印 BSP 接口。
 */

#ifndef BSP_UART_H
#define BSP_UART_H

/* ======== include ======== */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "bsp_status.h"

/* ======== 可调参数宏定义 ======== */

/* printf 格式化缓冲区大小，单位字节；过小会截断单行日志。 */
#define BSP_UART_PRINTF_BUF_SIZE       (256U)

/* UART TX 环形缓冲区容量，单位字节；影响突发日志缓存能力。 */
#define BSP_UART_TX_CAPACITY           (512U)

/* UART RX 环形缓冲区容量，单位字节；影响上层读取前可缓存的接收数据。 */
#define BSP_UART_RX_CAPACITY           (512U)

/* UART RX DMA 单次搬运缓冲区大小，单位字节；满包或空闲超时后转存到 RX 环形缓冲区。 */
#define BSP_UART_RX_DMA_BUF_SIZE       (64U)

/* UART0 硬件 TX FIFO 深度，单位字节；用于限制单次填充数量。 */
#define BSP_UART_FIFO_DEPTH_BYTES      (8U)

/* UART RX 空闲超时计数；SysConfig/硬件支持 0~15，用于产生一帧接收完成事件。 */
#define BSP_UART_RX_TIMEOUT_BITS       (15U)

/* bsp_uart_printf 等待上一帧发送完成的最长时间，单位 ms。 */
#define BSP_UART_PRINTF_TIMEOUT_MS     (50U)

/* UART0 中断优先级；需低于关键总线错误处理，高于普通任务。 */
#define BSP_UART_IRQ_PRIORITY          (1U)

/* ======== 类型定义 ======== */

/**
 * @brief UART 接收事件回调。
 * @param size 本次接收字节数。
 * @return 无。
 */
typedef void (*uart_rx_callback_t)(uint16_t size);

/**
 * @brief UART 错误事件回调。
 * @param void 无参数。
 * @return 无。
 */
typedef void (*uart_err_callback_t)(void);

/**
 * @brief UART 逻辑编号。
 */
typedef enum {
    UART_ID_UART0 = 0,     /**< 板载 UART0。 */
    UART_ID_NUM            /**< UART 实例数量。 */
} uart_id_e;

/* ======== 公开 API ======== */

/**
 * @brief 初始化 UART0 引脚、波特率和 TX 中断。
 * @param void 无参数。
 * @return 成功返回 BSP_OK。
 */
bsp_status_e bsp_uart_init(void);

/**
 * @brief 非阻塞写入 UART TX 环形缓冲区。
 * @param buf 待发送数据缓冲区。
 * @param len 待写入字节数。
 * @return 全部写入返回 BSP_OK，空间不足返回 BSP_ERR_BUSY。
 */
bsp_status_e bsp_uart_write_async(const uint8_t *buf, size_t len);

/**
 * @brief 从 UART RX 环形缓冲区读取数据。
 * @param buf 接收缓冲区。
 * @param len 期望读取字节数。
 * @param out_len 实际读取字节数，可传入 NULL。
 * @return 参数合法返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_uart_read(uint8_t *buf, size_t len, size_t *out_len);

/**
 * @brief 查询 UART TX 环形缓冲区待发送字节数。
 * @param void 无参数。
 * @return 待发送字节数。
 */
size_t bsp_uart_pending(void);

/**
 * @brief 查询 UART RX 环形缓冲区可读取字节数。
 * @param void 无参数。
 * @return 可读取字节数。
 */
size_t bsp_uart_rx_available(void);

/**
 * @brief 注册 UART 接收回调。
 * @param id UART 编号。
 * @param cb 接收回调，传入 NULL 表示清除。
 * @return 无。
 */
void bsp_uart_register_rx_callback(uart_id_e id, uart_rx_callback_t cb);

/**
 * @brief 注册 UART 错误回调。
 * @param id UART 编号。
 * @param cb 错误回调，传入 NULL 表示清除。
 * @return 无。
 */
void bsp_uart_register_error_callback(uart_id_e id, uart_err_callback_t cb);

/**
 * @brief 使用历史 DMA 风格接口名阻塞排队发送缓冲区。
 *
 * 当前实现使用 TX 环形缓冲区和 UART TX 中断，不使用 DMA。
 * @param id UART 编号。
 * @param data 待发送数据。
 * @param len 待发送字节数。
 * @return 无。
 */
void bsp_uart_send_dma(uart_id_e id, uint8_t *data, uint16_t len);

/**
 * @brief 格式化并发送 UART 调试文本。
 * @param id UART 编号。
 * @param format printf 风格格式字符串。
 * @return 成功返回发送字节数，参数非法或超时返回 -1。
 */
int bsp_uart_printf(uart_id_e id, const char *format, ...);

#endif /* BSP_UART_H */
