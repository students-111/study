/**
 * @file bsp_i2c.c
 * @brief I2C0 异步寄存器读写 BSP 实现。
 */

/* ======== include ======== */

#include "bsp_i2c.h"

#include <stdbool.h>

#include "bsp_time.h"
#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

typedef enum {
    BSP_I2C_IDLE = 0,       /**< 当前没有事务。 */
    BSP_I2C_TX,             /**< 正在发送寄存器地址或写数据。 */
    BSP_I2C_RX,             /**< 正在接收读数据。 */
    BSP_I2C_DONE,           /**< 事务已完成，等待 poll 消费。 */
    BSP_I2C_ERROR           /**< 硬件错误，等待 poll 清理。 */
} bsp_i2c_state_e;

typedef enum {
    BSP_I2C_OP_WRITE = 0,   /**< 寄存器写事务。 */
    BSP_I2C_OP_READ         /**< 寄存器读事务。 */
} bsp_i2c_op_e;

/* ======== 内部变量 ======== */

static volatile bsp_i2c_state_e g_bsp_i2c_state = BSP_I2C_IDLE;
static bsp_i2c_op_e g_bsp_i2c_op;
static uint8_t g_bsp_i2c_addr;
static uint8_t g_bsp_i2c_tx[BSP_I2C_TX_CAPACITY];
static uint8_t *g_bsp_i2c_rx;
static volatile uint16_t g_bsp_i2c_tx_count;
static uint16_t g_bsp_i2c_tx_len;
static volatile uint16_t g_bsp_i2c_rx_count;
static uint16_t g_bsp_i2c_rx_len;
static uint32_t g_bsp_i2c_deadline_ms;

/* ======== 内部函数 ======== */

/**
 * @brief 启动当前上下文中的 I2C TX 阶段。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_i2c_start_tx(void)
{
    g_bsp_i2c_tx_count = DL_I2C_fillControllerTXFIFO(
        I2C_MPU6050_INST, g_bsp_i2c_tx, g_bsp_i2c_tx_len);

    if (g_bsp_i2c_tx_count < g_bsp_i2c_tx_len) {
        DL_I2C_enableInterrupt(I2C_MPU6050_INST,
            DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    }

    g_bsp_i2c_state = BSP_I2C_TX;

    DL_I2C_startControllerTransferAdvanced(I2C_MPU6050_INST, g_bsp_i2c_addr,
        DL_I2C_CONTROLLER_DIRECTION_TX, g_bsp_i2c_tx_len,
        DL_I2C_CONTROLLER_START_ENABLE,
        (g_bsp_i2c_op == BSP_I2C_OP_READ) ?
            DL_I2C_CONTROLLER_STOP_DISABLE : DL_I2C_CONTROLLER_STOP_ENABLE,
        DL_I2C_CONTROLLER_ACK_DISABLE);
}

/**
 * @brief 启动当前上下文中的 I2C RX 阶段。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_i2c_start_rx(void)
{
    g_bsp_i2c_rx_count = 0U;
    g_bsp_i2c_state = BSP_I2C_RX;
    DL_I2C_startControllerTransferAdvanced(I2C_MPU6050_INST, g_bsp_i2c_addr,
        DL_I2C_CONTROLLER_DIRECTION_RX, g_bsp_i2c_rx_len,
        DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_ENABLE,
        DL_I2C_CONTROLLER_ACK_DISABLE);
}

/**
 * @brief 从 RX FIFO 搬运接收到的数据。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_i2c_drain_rx(void)
{
    uint8_t fifo_count = 0U;

    while (!DL_I2C_isControllerRXFIFOEmpty(I2C_MPU6050_INST) &&
           (fifo_count < BSP_I2C_FIFO_DRAIN_LIMIT)) {
        uint8_t data = DL_I2C_receiveControllerData(I2C_MPU6050_INST);

        if (g_bsp_i2c_rx_count < g_bsp_i2c_rx_len) {
            g_bsp_i2c_rx[g_bsp_i2c_rx_count++] = data;
        }
        fifo_count++;
    }
}

/**
 * @brief 根据 I2C 中断原因推进异步事务状态机。
 * @param iidx DriverLib 返回的中断原因。
 * @return 无。
 */
static void bsp_i2c_handle_irq(DL_I2C_IIDX iidx)
{
    if (g_bsp_i2c_state == BSP_I2C_ERROR) {
        return;
    }

    if (iidx == DL_I2C_IIDX_CONTROLLER_TXFIFO_TRIGGER) {
        if (g_bsp_i2c_tx_count < g_bsp_i2c_tx_len) {
            g_bsp_i2c_tx_count += DL_I2C_fillControllerTXFIFO(I2C_MPU6050_INST,
                &g_bsp_i2c_tx[g_bsp_i2c_tx_count],
                (uint16_t)(g_bsp_i2c_tx_len - g_bsp_i2c_tx_count));
        }
    } else if (iidx == DL_I2C_IIDX_CONTROLLER_TX_DONE) {
        DL_I2C_disableInterrupt(I2C_MPU6050_INST,
            DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
        if (g_bsp_i2c_op == BSP_I2C_OP_READ) {
            bsp_i2c_start_rx();
        } else {
            g_bsp_i2c_state = BSP_I2C_DONE;
        }
    } else if (iidx == DL_I2C_IIDX_CONTROLLER_RXFIFO_TRIGGER) {
        bsp_i2c_drain_rx();
    } else if (iidx == DL_I2C_IIDX_CONTROLLER_RX_DONE) {
        bsp_i2c_drain_rx();
        g_bsp_i2c_state = BSP_I2C_DONE;
    } else if ((iidx == DL_I2C_IIDX_CONTROLLER_NACK) ||
               (iidx == DL_I2C_IIDX_CONTROLLER_ARBITRATION_LOST)) {
        g_bsp_i2c_state = BSP_I2C_ERROR;
    }
}

/**
 * @brief 判断 I2C 软件状态机或硬件控制器是否忙。
 * @param void 无参数。
 * @return 忙返回 true，否则返回 false。
 */
static bool bsp_i2c_is_busy(void)
{
    return (g_bsp_i2c_state != BSP_I2C_IDLE) ||
        !(DL_I2C_getControllerStatus(I2C_MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE);
}

/* ======== 公开 API ======== */

bsp_status_e bsp_i2c_init(void)
{
    /*
     * WHY: 引脚、时钟、400 kHz 周期、FIFO 阈值和中断源由 SysConfig
     * 统一生成，BSP 只接管运行期事务状态和 NVIC 入口。
     */
    DL_I2C_resetControllerTransfer(I2C_MPU6050_INST);

    NVIC_SetPriority(I2C_MPU6050_INST_INT_IRQN, BSP_I2C_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(I2C_MPU6050_INST_INT_IRQN);
    NVIC_EnableIRQ(I2C_MPU6050_INST_INT_IRQN);
    g_bsp_i2c_state = BSP_I2C_IDLE;
    return BSP_OK;
}

bsp_status_e bsp_i2c_mem_read_async(
    uint8_t addr, uint8_t reg, uint8_t *buf, size_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > UINT16_MAX) ||
        (addr > BSP_I2C_7BIT_ADDR_MAX)) {
        return BSP_ERR_PARAM;
    }

    if (bsp_i2c_is_busy()) {
        return BSP_ERR_BUSY;
    }

    g_bsp_i2c_op = BSP_I2C_OP_READ;
    g_bsp_i2c_addr = addr;
    g_bsp_i2c_tx[0] = reg;
    g_bsp_i2c_tx_len = 1U;
    g_bsp_i2c_rx = buf;
    g_bsp_i2c_rx_len = (uint16_t)len;
    g_bsp_i2c_deadline_ms = bsp_time_get_ms() + BSP_I2C_TIMEOUT_MS;

    bsp_i2c_start_tx();
    return BSP_OK;
}

bsp_status_e bsp_i2c_mem_write_async(
    uint8_t addr, uint8_t reg, const uint8_t *buf, size_t len)
{
    size_t idx;

    if (((buf == NULL) && (len != 0U)) ||
        (len >= BSP_I2C_TX_CAPACITY) || (addr > BSP_I2C_7BIT_ADDR_MAX)) {
        return BSP_ERR_PARAM;
    }
    if (bsp_i2c_is_busy()) {
        return BSP_ERR_BUSY;
    }

    g_bsp_i2c_op = BSP_I2C_OP_WRITE;
    g_bsp_i2c_addr = addr;
    g_bsp_i2c_tx[0] = reg;
    for (idx = 0U; idx < len; idx++) {
        g_bsp_i2c_tx[idx + 1U] = buf[idx];
    }
    g_bsp_i2c_tx_len = (uint16_t)(len + 1U);
    g_bsp_i2c_deadline_ms = bsp_time_get_ms() + BSP_I2C_TIMEOUT_MS;
    bsp_i2c_start_tx();
    return BSP_OK;
}

bsp_status_e bsp_i2c_poll(void)
{
    bsp_i2c_state_e state = g_bsp_i2c_state;

    if (state == BSP_I2C_DONE) {
        g_bsp_i2c_state = BSP_I2C_IDLE;
        return BSP_OK;
    }
    if (state == BSP_I2C_ERROR) {
        DL_I2C_resetControllerTransfer(I2C_MPU6050_INST);
        g_bsp_i2c_state = BSP_I2C_IDLE;
        return BSP_ERR_HW;
    }

    if ((state != BSP_I2C_IDLE) &&
        ((int32_t)(bsp_time_get_ms() - g_bsp_i2c_deadline_ms) >= 0)) {
        DL_I2C_resetControllerTransfer(I2C_MPU6050_INST);
        g_bsp_i2c_state = BSP_I2C_IDLE;
        return BSP_ERR_TIMEOUT;
    }
    return (state == BSP_I2C_IDLE) ? BSP_OK : BSP_ERR_BUSY;
}

void I2C0_IRQHandler(void)
{
    bsp_i2c_handle_irq(DL_I2C_getPendingInterrupt(I2C_MPU6050_INST));
}
