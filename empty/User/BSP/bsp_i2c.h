/**
 * @file bsp_i2c.h
 * @brief I2C0 异步寄存器读写 BSP 接口。
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

/* ======== include ======== */

#include <stddef.h>
#include <stdint.h>

#include "bsp_status.h"

/* ======== 可调参数宏定义 ======== */

/* I2C TX 临时缓冲区容量，单位字节；包含 1 字节寄存器地址。 */
#define BSP_I2C_TX_CAPACITY            (16U)

/* 单笔 I2C 事务超时时间，单位 ms；防止状态机永久 BUSY。 */
#define BSP_I2C_TIMEOUT_MS             (20U)

/* 单次 RX FIFO 搬运的最大字节数；避免 ISR 处理过长。 */
#define BSP_I2C_FIFO_DRAIN_LIMIT       (8U)

/* 7 位 I2C 地址最大值；用于参数校验。 */
#define BSP_I2C_7BIT_ADDR_MAX          (0x7FU)

/* I2C0 中断优先级；总线事务优先于普通调度任务。 */
#define BSP_I2C_IRQ_PRIORITY           (0U)

/* ======== 公开 API ======== */

/**
 * @brief 初始化 I2C0 引脚和控制器模式。
 * @param void 无参数。
 * @return 成功返回 BSP_OK。
 */
bsp_status_e bsp_i2c_init(void);

/**
 * @brief 启动异步 I2C 寄存器读事务。
 * @param addr 7 位 I2C 从设备地址。
 * @param reg 起始寄存器地址。
 * @param buf 接收缓冲区。
 * @param len 读取字节数。
 * @return 启动成功返回 BSP_OK，否则返回 BSP_ERR_BUSY 或 BSP_ERR_PARAM。
 */
bsp_status_e bsp_i2c_mem_read_async(
    uint8_t addr, uint8_t reg, uint8_t *buf, size_t len);

/**
 * @brief 启动异步 I2C 寄存器写事务。
 * @param addr 7 位 I2C 从设备地址。
 * @param reg 起始寄存器地址。
 * @param buf 待写数据缓冲区。
 * @param len 待写数据字节数。
 * @return 启动成功返回 BSP_OK，否则返回 BSP_ERR_BUSY 或 BSP_ERR_PARAM。
 */
bsp_status_e bsp_i2c_mem_write_async(
    uint8_t addr, uint8_t reg, const uint8_t *buf, size_t len);

/**
 * @brief 轮询当前异步 I2C 事务状态。
 * @param void 无参数。
 * @return 事务进行中返回 BSP_ERR_BUSY，结束后返回终态状态码。
 */
bsp_status_e bsp_i2c_poll(void);

#endif /* BSP_I2C_H */
