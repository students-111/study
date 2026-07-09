/**
 * @file drv_encoder.h
 * @brief 简单正交编码器驱动库接口。
 */

#ifndef DRV_ENCODER_H
#define DRV_ENCODER_H

/* ======== include ======== */

#include <stdint.h>

#include "bsp_gpio.h"

/* ======== 可调参数宏定义 ======== */

/* 编码器 A/B 相状态掩码；当前仅使用低 2 位。 */
#define ENCODER_STATE_MASK          (0x03U)

/* 编码器 A 相状态位。 */
#define ENCODER_PHASE_A_BIT         (0x01U)

/* 编码器 B 相状态位。 */
#define ENCODER_PHASE_B_BIT         (0x02U)

/* 正交解码转移表项数量；由 4 个旧状态和 4 个新状态组合得到。 */
#define ENCODER_TRANSITION_COUNT    (16U)

/* 正交状态组合时旧状态左移位数；用于拼出 4 bit 转移索引。 */
#define ENCODER_PREV_SHIFT          (2U)

/* ======== 类型定义 ======== */

/**
 * @brief 编码器状态枚举。
 */
typedef enum {
    ENCODER_STATE_STOP = 0,         /**< 本周期无位移。 */
    ENCODER_STATE_FORWARD,          /**< 本周期正向位移。 */
    ENCODER_STATE_BACKWARD,         /**< 本周期反向位移。 */
    ENCODER_STATE_ERROR             /**< 读取失败或状态异常。 */
} EncoderState_t;

/**
 * @brief 编码器硬件配置结构体。
 */
typedef struct {
    void *phase_a_port;             /**< A 相 GPIO 端口，本工程由 BSP 持有。 */
    bsp_gpio_pin_e phase_a_pin;     /**< A 相 GPIO 引脚。 */
    void *phase_b_port;             /**< B 相 GPIO 端口，本工程由 BSP 持有。 */
    bsp_gpio_pin_e phase_b_pin;     /**< B 相 GPIO 引脚。 */
} EncoderHW_t;

/**
 * @brief 编码器驱动实体结构体。
 */
typedef struct {
    EncoderHW_t hw;                 /**< 硬件配置。 */
    int32_t count;                  /**< 累计编码器计数。 */
    int16_t delta;                  /**< 本次刷新产生的计数增量。 */
    EncoderState_t state;           /**< 当前状态。 */
    uint8_t raw_state;              /**< 当前 A/B 相压缩状态。 */
    uint8_t last_state;             /**< 上一帧 A/B 相压缩状态。 */
    uint8_t has_state;              /**< 是否已经建立初始 A/B 相基准。 */
    uint8_t enable;                 /**< 使能标志。 */
    uint8_t reverse;                /**< 编码器安装方向，0 表示正装，1 表示反装。 */
    uint32_t sequence;              /**< 每次刷新递增的样本序号。 */
} Encoder_t;

/* ======== 公开 API ======== */

/**
 * @brief 初始化编码器驱动实体。
 * @param encoder 编码器实体。
 * @param hw 硬件配置。
 * @param reverse 安装方向，0 表示正装，1 表示反装。
 * @return 初始化成功返回 1，否则返回 0。
 */
uint8_t Encoder_Init(Encoder_t *encoder, const EncoderHW_t *hw,
    uint8_t reverse);

/**
 * @brief 使能或失能编码器。
 * @param encoder 编码器实体。
 * @param enable 1 表示使能，0 表示失能。
 * @return 设置成功返回 1，否则返回 0。
 */
uint8_t Encoder_Enable(Encoder_t *encoder, uint8_t enable);

/**
 * @brief 刷新一次正交编码器解码状态。
 * @param encoder 编码器实体。
 * @return 刷新成功返回 1，否则返回 0。
 */
uint8_t Encoder_Update(Encoder_t *encoder);

/**
 * @brief 清零编码器累计计数和运行状态。
 * @param encoder 编码器实体。
 * @return 清零成功返回 1，否则返回 0。
 */
uint8_t Encoder_Reset(Encoder_t *encoder);

#endif /* DRV_ENCODER_H */
