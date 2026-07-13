/**
 * @file bsp_encoder.c
 * @brief 两路正交编码器 GPIO 输入与中断分发实现。
 */

/* ======== include ======== */

#include "bsp_encoder.h"

#include <stddef.h>

#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* M1 编码器两相引脚掩码。 */
#define BSP_ENCODER_M1_PIN_MASK          (BOARD_GPIO_ENC_M1_A_PIN | \
                                         BOARD_GPIO_ENC_M1_B_PIN)

/* M2 编码器两相引脚掩码。 */
#define BSP_ENCODER_M2_PIN_MASK          (BOARD_GPIO_ENC_M2_A_PIN | \
                                         BOARD_GPIO_ENC_M2_B_PIN)

/* 两路编码器全部中断引脚掩码。 */
#define BSP_ENCODER_ALL_PIN_MASK         (BSP_ENCODER_M1_PIN_MASK | \
                                         BSP_ENCODER_M2_PIN_MASK)

/* ======== 内部变量 ======== */

static bsp_encoder_edge_callback_t g_bsp_encoder_edge_callback;
static void *g_bsp_encoder_callback_context;

/* ======== 内部函数 ======== */

/**
 * @brief 判断板级编码器通道编号是否合法。
 * @param channel 编码器通道编号。
 * @return 合法返回 true，否则返回 false。
 */
static bool bsp_encoder_channel_is_valid(bsp_encoder_channel_e channel)
{
    return ((unsigned int)channel < (unsigned int)BSP_ENCODER_COUNT);
}

/**
 * @brief 一次读取同一端口上的编码器 A/B 相电平。
 * @param port GPIO 端口。
 * @param phase_a_pin A 相引脚掩码。
 * @param phase_b_pin B 相引脚掩码。
 * @param phase_a 输出 A 相电平。
 * @param phase_b 输出 B 相电平。
 * @return 无。
 */
static void bsp_encoder_read_phase_pair(GPIO_Regs *port,
    uint32_t phase_a_pin, uint32_t phase_b_pin,
    bool *phase_a, bool *phase_b)
{
    uint32_t pin_states = DL_GPIO_readPins(port,
        phase_a_pin | phase_b_pin);

    /* WHY: 单次端口读取避免 A/B 两次读取之间插入新边沿形成混合状态。 */
    *phase_a = ((pin_states & phase_a_pin) != 0U);
    *phase_b = ((pin_states & phase_b_pin) != 0U);
}

/* ======== 公开 API ======== */

bsp_status_e bsp_encoder_init(bsp_encoder_edge_callback_t callback,
    void *context)
{
    if (callback == NULL) {
        return BSP_ERR_PARAM;
    }

    NVIC_DisableIRQ(BOARD_GPIO_INT_IRQN);
    g_bsp_encoder_edge_callback = callback;
    g_bsp_encoder_callback_context = context;

    DL_GPIO_clearInterruptStatus(BOARD_GPIO_PORT,
        BSP_ENCODER_ALL_PIN_MASK);
    NVIC_SetPriority(BOARD_GPIO_INT_IRQN, BSP_ENCODER_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(BOARD_GPIO_INT_IRQN);
    NVIC_EnableIRQ(BOARD_GPIO_INT_IRQN);
    return BSP_OK;
}

bsp_status_e bsp_encoder_read_phases(bsp_encoder_channel_e channel,
    bool *phase_a, bool *phase_b)
{
    if (!bsp_encoder_channel_is_valid(channel) || (phase_a == NULL) ||
        (phase_b == NULL)) {
        return BSP_ERR_PARAM;
    }

    if (channel == BSP_ENCODER_M1) {
        bsp_encoder_read_phase_pair(BOARD_GPIO_PORT,
            BOARD_GPIO_ENC_M1_A_PIN, BOARD_GPIO_ENC_M1_B_PIN,
            phase_a, phase_b);
    } else {
        bsp_encoder_read_phase_pair(BOARD_GPIO_PORT,
            BOARD_GPIO_ENC_M2_A_PIN, BOARD_GPIO_ENC_M2_B_PIN,
            phase_a, phase_b);
    }
    return BSP_OK;
}

uint32_t bsp_encoder_irq_lock(void)
{
    uint32_t irq_was_enabled = NVIC_GetEnableIRQ(BOARD_GPIO_INT_IRQN);

    NVIC_DisableIRQ(BOARD_GPIO_INT_IRQN);
    return irq_was_enabled;
}

void bsp_encoder_irq_unlock(uint32_t irq_was_enabled)
{
    if (irq_was_enabled != 0U) {
        NVIC_EnableIRQ(BOARD_GPIO_INT_IRQN);
    }
}

/**
 * @brief GPIOB Group1 中断入口，分发两路编码器边沿。
 * @param void 无参数。
 * @return 无。
 */
void GROUP1_IRQHandler(void)
{
    uint32_t pending_pins;
    bsp_encoder_edge_callback_t callback;
    void *context;

    if (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) !=
        BOARD_GPIO_INT_IIDX) {
        return;
    }

    pending_pins = DL_GPIO_getEnabledInterruptStatus(
        BOARD_GPIO_PORT, BSP_ENCODER_ALL_PIN_MASK);
    if (pending_pins == 0U) {
        return;
    }

    /* WHY: 先清除本次快照，处理期间到来的新边沿仍可重新挂起中断。 */
    DL_GPIO_clearInterruptStatus(BOARD_GPIO_PORT, pending_pins);

    callback = g_bsp_encoder_edge_callback;
    context = g_bsp_encoder_callback_context;
    if (callback == NULL) {
        return;
    }

    if ((pending_pins & BSP_ENCODER_M1_PIN_MASK) != 0U) {
        callback(BSP_ENCODER_M1, context);
    }
    if ((pending_pins & BSP_ENCODER_M2_PIN_MASK) != 0U) {
        callback(BSP_ENCODER_M2, context);
    }
}
