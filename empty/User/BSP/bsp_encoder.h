/**
 * @file bsp_encoder.h
 * @brief 两路正交编码器 GPIO 输入与中断分发接口。
 */

#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

#include "bsp_status.h"

/* ======== 可调参数宏定义 ======== */

/* 编码器 GPIO 中断优先级；需优先于调试 UART，避免高速边沿被长时间延迟。 */
#define BSP_ENCODER_IRQ_PRIORITY          (0U)

/* ======== 类型定义 ======== */

/**
 * @brief 板级编码器通道编号。
 */
typedef enum {
    BSP_ENCODER_M1 = 0,          /**< M1 编码器通道。 */
    BSP_ENCODER_M2,              /**< M2 编码器通道。 */
    BSP_ENCODER_COUNT            /**< 编码器通道数量。 */
} bsp_encoder_channel_e;

/**
 * @brief 编码器边沿中断回调类型。
 * @param channel 发生边沿的编码器通道。
 * @param context 注册回调时保存的上下文指针。
 * @return 无。
 */
typedef void (*bsp_encoder_edge_callback_t)(bsp_encoder_channel_e channel,
    void *context);

/* ======== 公开 API ======== */

/**
 * @brief 注册编码器边沿回调并启用 GPIOB Group1 中断。
 * @param callback 编码器边沿回调，不能为空。
 * @param context 回调上下文，可以为空。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_encoder_init(bsp_encoder_edge_callback_t callback,
    void *context);

/**
 * @brief 读取指定编码器当前 A/B 相电平。
 * @param channel 编码器通道编号。
 * @param phase_a 输出 A 相电平。
 * @param phase_b 输出 B 相电平。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_encoder_read_phases(bsp_encoder_channel_e channel,
    bool *phase_a, bool *phase_b);

/**
 * @brief 临时屏蔽编码器 Group1 中断并返回原使能状态。
 * @param void 无参数。
 * @return 屏蔽前已使能返回 1，否则返回 0。
 */
uint32_t bsp_encoder_irq_lock(void);

/**
 * @brief 按保存状态恢复编码器 Group1 中断。
 * @param irq_was_enabled `bsp_encoder_irq_lock()` 返回的原使能状态。
 * @return 无。
 */
void bsp_encoder_irq_unlock(uint32_t irq_was_enabled);

#endif /* BSP_ENCODER_H */

