/**
 * @file bsp_uart.c
 * @brief UART0 异步收发 BSP 实现。
 */

/* ======== include ======== */

#include "bsp_uart.h"

#include <stdbool.h>

#include "ringbuffer.h"
#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 内部变量 ======== */

static uint8_t g_bsp_uart_printf_buf[BSP_UART_PRINTF_BUF_SIZE];
static uart_rx_callback_t g_bsp_uart_rx_callback[UART_ID_NUM] = {NULL};
static uart_err_callback_t g_bsp_uart_error_callback[UART_ID_NUM] = {NULL};
static struct rt_ringbuffer g_bsp_uart_tx_rb;
static struct rt_ringbuffer g_bsp_uart_rx_rb;
static uint8_t g_bsp_uart_tx_pool[BSP_UART_TX_CAPACITY];
static uint8_t g_bsp_uart_rx_pool[BSP_UART_RX_CAPACITY];
static uint8_t g_bsp_uart_rx_dma_buf[BSP_UART_RX_DMA_BUF_SIZE];
static volatile uint16_t g_bsp_uart_rx_frame_size;

/* ======== 内部函数声明 ======== */

/**
 * @brief 尽可能将 TX 环形缓冲区数据填入 UART FIFO。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_uart_fill_fifo(void);

/**
 * @brief 将 UART RX FIFO 中已有字节搬入 RX 环形缓冲区。
 * @param void 无参数。
 * @return 本次搬运的字节数。
 */
static uint16_t bsp_uart_drain_rx_fifo(void);

/**
 * @brief 启动 UART RX DMA 接收。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_uart_rx_dma_start(void);

/**
 * @brief 停止 RX DMA 并把已接收数据搬入 RX 环形缓冲区。
 * @param force_full true 表示 DMA 缓冲区已满。
 * @return 本次搬运的字节数。
 */
static uint16_t bsp_uart_rx_dma_commit(bool force_full);

/**
 * @brief 处理 UART 接收错误。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_uart_handle_error(void);

/* ======== 内部函数 ======== */

/**
 * @brief 进入 UART 环形缓冲区临界区。
 * @param void 无参数。
 * @return 进入临界区前的 PRIMASK。
 */
static uint32_t bsp_uart_critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

/**
 * @brief 退出 UART 环形缓冲区临界区。
 * @param primask 进入临界区前保存的 PRIMASK。
 * @return 无。
 */
static void bsp_uart_critical_exit(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/**
 * @brief 判断 UART 逻辑编号是否合法。
 * @param id UART 编号。
 * @return 合法返回 true，否则返回 false。
 */
static bool bsp_uart_is_valid_id(uart_id_e id)
{
    return id < UART_ID_NUM;
}

/**
 * @brief 向 UART TX 环形缓冲区写入单字节。
 * @param data 待写入字节。
 * @return 写入成功返回 BSP_OK，空间不足返回 BSP_ERR_BUSY。
 */
static bsp_status_e bsp_uart_write_byte(uint8_t data)
{
    uint32_t primask;
    rt_size_t written;

    primask = bsp_uart_critical_enter();
    written = rt_ringbuffer_putchar(&g_bsp_uart_tx_rb, data);
    if (written == 0U) {
        bsp_uart_critical_exit(primask);
        return BSP_ERR_BUSY;
    }

    bsp_uart_fill_fifo();

    if (rt_ringbuffer_data_len(&g_bsp_uart_tx_rb) != 0U) {
        DL_UART_Main_enableInterrupt(UART_DEBUG_INST,
            DL_UART_MAIN_INTERRUPT_TX);
    }

    bsp_uart_critical_exit(primask);
    return BSP_OK;
}

/**
 * @brief 尽可能将 TX 环形缓冲区数据填入 UART FIFO。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_uart_fill_fifo(void)
{
    uint8_t fifo_count = 0U;
    rt_uint8_t data;

    while ((fifo_count < BSP_UART_FIFO_DEPTH_BYTES) &&
           !DL_UART_Main_isTXFIFOFull(UART_DEBUG_INST) &&
           (rt_ringbuffer_getchar(&g_bsp_uart_tx_rb, &data) == 1U)) {
        DL_UART_Main_transmitData(UART_DEBUG_INST, data);
        fifo_count++;
    }
}

/**
 * @brief 将 UART RX FIFO 中已有字节搬入 RX 环形缓冲区。
 * @param void 无参数。
 * @return 本次搬运的字节数。
 */
static uint16_t bsp_uart_drain_rx_fifo(void)
{
    uint16_t moved = 0U;

    while (!DL_UART_Main_isRXFIFOEmpty(UART_DEBUG_INST)) {
        uint8_t data = DL_UART_Main_receiveData(UART_DEBUG_INST);

        if (rt_ringbuffer_putchar(&g_bsp_uart_rx_rb, data) == 0U) {
            break;
        }
        moved++;
    }

    return moved;
}

/**
 * @brief 启动 UART RX DMA 接收。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_uart_rx_dma_start(void)
{
#if defined(DMA_CH0_CHAN_ID)
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t)&UART_DEBUG_INST->RXDATA);
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID,
        (uint32_t)&g_bsp_uart_rx_dma_buf[0]);
    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID,
        BSP_UART_RX_DMA_BUF_SIZE);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
#endif
}

/**
 * @brief 停止 RX DMA 并把已接收数据搬入 RX 环形缓冲区。
 * @param force_full true 表示 DMA 缓冲区已满。
 * @return 本次搬运的字节数。
 */
static uint16_t bsp_uart_rx_dma_commit(bool force_full)
{
    uint16_t remaining;
    uint16_t received;
    uint16_t written;

#if defined(DMA_CH0_CHAN_ID)
    if (force_full) {
        received = BSP_UART_RX_DMA_BUF_SIZE;
    } else {
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
        remaining = DL_DMA_getTransferSize(DMA, DMA_CH0_CHAN_ID);
        received = (uint16_t)(BSP_UART_RX_DMA_BUF_SIZE - remaining);
    }

    written = rt_ringbuffer_put(&g_bsp_uart_rx_rb,
        (const rt_uint8_t *)&g_bsp_uart_rx_dma_buf[0], received);
    bsp_uart_rx_dma_start();
    return written;
#else
    (void)force_full;
    remaining = 0U;
    received = 0U;
    written = 0U;
    return 0U;
#endif
}

/**
 * @brief 处理 UART 错误中断并通知上层。
 * @param void 无参数。
 * @return 无。
 */
static void bsp_uart_handle_error(void)
{
#if defined(DMA_CH0_CHAN_ID)
    (void)bsp_uart_rx_dma_commit(false);
#else
    (void)bsp_uart_drain_rx_fifo();
#endif
    g_bsp_uart_rx_frame_size = 0U;

    if (g_bsp_uart_error_callback[UART_ID_UART0] != NULL) {
        g_bsp_uart_error_callback[UART_ID_UART0]();
    }
}

/* ======== 公开 API ======== */

void bsp_uart_register_rx_callback(uart_id_e id, uart_rx_callback_t cb)
{
    if (bsp_uart_is_valid_id(id)) {
        g_bsp_uart_rx_callback[id] = cb;
    }
}

void bsp_uart_register_error_callback(uart_id_e id, uart_err_callback_t cb)
{
    if (bsp_uart_is_valid_id(id)) {
        g_bsp_uart_error_callback[id] = cb;
    }
}

bsp_status_e bsp_uart_init(void)
{
    rt_ringbuffer_init(&g_bsp_uart_tx_rb,
        g_bsp_uart_tx_pool, (rt_int16_t)sizeof(g_bsp_uart_tx_pool));
    rt_ringbuffer_init(&g_bsp_uart_rx_rb,
        g_bsp_uart_rx_pool, (rt_int16_t)sizeof(g_bsp_uart_rx_pool));
    g_bsp_uart_rx_frame_size = 0U;

    /*
     * WHY: UART 引脚、BUSCLK、115200 分频和 FIFO 阈值由 SysConfig 生成；
     * BSP 只在有待发送数据时动态打开 TX 中断，RX 通过 FIFO/超时中断搬运。
     */
    DL_UART_Main_setRXInterruptTimeout(UART_DEBUG_INST,
        BSP_UART_RX_TIMEOUT_BITS);
    DL_UART_Main_disableInterrupt(UART_DEBUG_INST, DL_UART_MAIN_INTERRUPT_TX);
#if defined(DMA_CH0_CHAN_ID)
    bsp_uart_rx_dma_start();
    DL_UART_Main_enableInterrupt(UART_DEBUG_INST,
        DL_UART_MAIN_INTERRUPT_DMA_DONE_RX |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);
#else
    DL_UART_Main_enableInterrupt(UART_DEBUG_INST,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_NOISE_ERROR);
#endif
    NVIC_SetPriority(UART_DEBUG_INST_INT_IRQN, BSP_UART_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(UART_DEBUG_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_DEBUG_INST_INT_IRQN);
    return BSP_OK;
}

bsp_status_e bsp_uart_write_async(const uint8_t *buf, size_t len)
{
    rt_size_t written;
    uint32_t primask;

    if ((buf == NULL) && (len != 0U)) {
        return BSP_ERR_PARAM;
    }
    if (len == 0U) {
        return BSP_OK;
    }
    if (len > UINT16_MAX) {
        return BSP_ERR_PARAM;
    }

    primask = bsp_uart_critical_enter();
    written = rt_ringbuffer_put(&g_bsp_uart_tx_rb,
        (const rt_uint8_t *)buf, (rt_uint16_t)len);

    bsp_uart_fill_fifo();

    if (rt_ringbuffer_data_len(&g_bsp_uart_tx_rb) != 0U) {
        DL_UART_Main_enableInterrupt(UART_DEBUG_INST,
            DL_UART_MAIN_INTERRUPT_TX);
    }

    bsp_uart_critical_exit(primask);
    return (written == len) ? BSP_OK : BSP_ERR_BUSY;
}

bsp_status_e bsp_uart_read(uint8_t *buf, size_t len, size_t *out_len)
{
    rt_size_t read_len;
    uint32_t primask;

    if ((buf == NULL) && (len != 0U)) {
        return BSP_ERR_PARAM;
    }
    if (len > UINT16_MAX) {
        return BSP_ERR_PARAM;
    }

    primask = bsp_uart_critical_enter();
    read_len = rt_ringbuffer_get(&g_bsp_uart_rx_rb, buf, (rt_uint16_t)len);
    bsp_uart_critical_exit(primask);

    if (out_len != NULL) {
        *out_len = (size_t)read_len;
    }

    return BSP_OK;
}

void bsp_uart_send_dma(uart_id_e id, uint8_t *data, uint16_t len)
{
    uint16_t idx;

    if (!bsp_uart_is_valid_id(id) || (data == NULL)) {
        return;
    }

    for (idx = 0U; idx < len; idx++) {
        while (bsp_uart_write_byte(data[idx]) == BSP_ERR_BUSY) {
            /* WHY: 兼容旧接口的阻塞发送语义。 */
        }
    }
}

int bsp_uart_printf(uart_id_e id, const char *format, ...)
{
    va_list ap;
    int len;

    if (!bsp_uart_is_valid_id(id) || (format == NULL)) {
        return -1;
    }
    if (bsp_uart_pending() != 0U) {
        return -1;
    }

    va_start(ap, format);
    len = vsnprintf((char *)g_bsp_uart_printf_buf, BSP_UART_PRINTF_BUF_SIZE,
        format, ap);
    va_end(ap);

    if (len <= 0) {
        return len;
    }

    if ((uint32_t)len >= BSP_UART_PRINTF_BUF_SIZE) {
        len = (int)(BSP_UART_PRINTF_BUF_SIZE - 1U);
    }

    return (bsp_uart_write_async(g_bsp_uart_printf_buf,
        (size_t)len) == BSP_OK) ? len : -1;
}

int bsp_uart_try_printf(uart_id_e id, const char *format, ...)
{
    va_list ap;
    int len;

    if (!bsp_uart_is_valid_id(id) || (format == NULL)) {
        return -1;
    }
    if (bsp_uart_pending() != 0U) {
        return -1;
    }

    va_start(ap, format);
    len = vsnprintf((char *)g_bsp_uart_printf_buf, BSP_UART_PRINTF_BUF_SIZE,
        format, ap);
    va_end(ap);

    if (len <= 0) {
        return -1;
    }

    if ((uint32_t)len >= BSP_UART_PRINTF_BUF_SIZE) {
        len = (int)(BSP_UART_PRINTF_BUF_SIZE - 1U);
    }

    return (bsp_uart_write_async(g_bsp_uart_printf_buf,
        (size_t)len) == BSP_OK) ? len : -1;
}

size_t bsp_uart_pending(void)
{
    return rt_ringbuffer_data_len(&g_bsp_uart_tx_rb);
}

size_t bsp_uart_rx_available(void)
{
    return rt_ringbuffer_data_len(&g_bsp_uart_rx_rb);
}

void UART0_IRQHandler(void)
{
    uint16_t rx_size;

    switch (DL_UART_Main_getPendingInterrupt(UART_DEBUG_INST)) {
    case DL_UART_MAIN_IIDX_TX:
        bsp_uart_fill_fifo();

        if (rt_ringbuffer_data_len(&g_bsp_uart_tx_rb) == 0U) {
            DL_UART_Main_disableInterrupt(UART_DEBUG_INST,
                DL_UART_MAIN_INTERRUPT_TX);
        }
        break;
    case DL_UART_MAIN_IIDX_RX:
        g_bsp_uart_rx_frame_size =
            (uint16_t)(g_bsp_uart_rx_frame_size + bsp_uart_drain_rx_fifo());
        break;
    case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR:
#if defined(DMA_CH0_CHAN_ID)
        rx_size = bsp_uart_rx_dma_commit(false);
#else
        rx_size = bsp_uart_drain_rx_fifo();
#endif
        g_bsp_uart_rx_frame_size =
            (uint16_t)(g_bsp_uart_rx_frame_size + rx_size);
        rx_size = g_bsp_uart_rx_frame_size;
        g_bsp_uart_rx_frame_size = 0U;
        if ((rx_size != 0U) &&
            (g_bsp_uart_rx_callback[UART_ID_UART0] != NULL)) {
            g_bsp_uart_rx_callback[UART_ID_UART0](rx_size);
        }
        break;
    case DL_UART_MAIN_IIDX_DMA_DONE_RX:
        rx_size = bsp_uart_rx_dma_commit(true);
        g_bsp_uart_rx_frame_size = 0U;
        if ((rx_size != 0U) &&
            (g_bsp_uart_rx_callback[UART_ID_UART0] != NULL)) {
            g_bsp_uart_rx_callback[UART_ID_UART0](rx_size);
        }
        break;
    case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
    case DL_UART_MAIN_IIDX_BREAK_ERROR:
    case DL_UART_MAIN_IIDX_PARITY_ERROR:
    case DL_UART_MAIN_IIDX_FRAMING_ERROR:
    case DL_UART_MAIN_IIDX_NOISE_ERROR:
        bsp_uart_handle_error();
        break;
    default:
        break;
    }
}
