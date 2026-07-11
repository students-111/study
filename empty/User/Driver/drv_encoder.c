/**
 * @file drv_encoder.c
 * @brief 简单正交编码器驱动库实现。
 */

/* ======== include ======== */

#include "drv_encoder.h"

#include <stddef.h>

/* ======== 可调参数宏定义 ======== */

/* ======== 内部变量 ======== */

/*
 * 四相正交解码表，索引为 (previous << 2) | current。
 * 相邻合法状态只移动一步；跳变到对角状态通常来自抖动或漏采，按 0 处理。
 */
static const int8_t g_encoder_transition_table[ENCODER_TRANSITION_COUNT] = {
    0,  1, -1,  0,
   -1,  0,  0,  1,
    1,  0,  0, -1,
    0, -1,  1,  0
};

/* ======== 内部函数 ======== */

/**
 * @brief 将 A/B 相电平压缩为 2 bit 状态。
 * @param phase_a A 相电平。
 * @param phase_b B 相电平。
 * @return 压缩后的 A/B 相状态。
 * 人话 ：把编码器引脚的四位测速00 01 10 11 变成 0 1 2 3
 */
static uint8_t Encoder_PackState(bool phase_a, bool phase_b)
{
    uint8_t state = 0U;

    if (phase_a) {
        state |= ENCODER_PHASE_A_BIT;
    }
    if (phase_b) {
        state |= ENCODER_PHASE_B_BIT;
    }

    return state;
}

/**
 * @brief 根据上一状态和当前状态解码单步位移。
 * @param previous 上一帧 A/B 相压缩状态。
 * @param current 当前 A/B 相压缩状态。
 * @return 正向返回 1，反向返回 -1，无效或未移动返回 0。
 * 根据压缩过后的2bit位推导出目前是正转还是反转
 */
static int8_t Encoder_DecodeStep(uint8_t previous, uint8_t current)
{
    return g_encoder_transition_table[
        ((previous & ENCODER_STATE_MASK) << ENCODER_PREV_SHIFT) |
        (current & ENCODER_STATE_MASK)];
}

/**
 * @brief 根据单步位移更新编码器状态。
 * @param encoder 编码器实体。
 * @param step 本次解码位移。
 * @return 无。
 */
static void Encoder_UpdateState(Encoder_t *encoder, int8_t step)
{
    if (step > 0) {
        encoder->state = ENCODER_STATE_FORWARD;
    } else if (step < 0) {
        encoder->state = ENCODER_STATE_BACKWARD;
    } else {
        encoder->state = ENCODER_STATE_STOP;
    }
}

/* ======== 公开 API ======== */

uint8_t Encoder_Init(Encoder_t *encoder, uint8_t reverse)
{
    if (encoder == NULL) {
        return 0U;
    }

    encoder->reverse = (reverse != 0U) ? 1U : 0U;
    encoder->enable = 1U;

    return Encoder_Reset(encoder);
}

uint8_t Encoder_Enable(Encoder_t *encoder, uint8_t enable)
{
    if (encoder == NULL) {
        return 0U;
    }

    encoder->enable = (enable != 0U) ? 1U : 0U;
    if (encoder->enable == 0U) {
        encoder->delta = 0;
        encoder->state = ENCODER_STATE_STOP;
    }
    return 1U;
}

uint8_t Encoder_Update(Encoder_t *encoder, bool phase_a, bool phase_b)
{
    uint8_t current_state;
    int8_t step;

    if (encoder == NULL) {
        return 0U;
    }
    if (encoder->enable == 0U) {
        /* 如果编码器已经失能，只刷新本次增量和停止状态。 */
        encoder->delta = 0;
        encoder->state = ENCODER_STATE_STOP;
        return 1U;
    }

    current_state = Encoder_PackState(phase_a, phase_b);
    encoder->raw_state = current_state;

    if (encoder->has_state == 0U) {
        encoder->last_state = current_state;
        encoder->delta = 0;
        encoder->state = ENCODER_STATE_STOP;
        encoder->has_state = 1U;
        encoder->sequence++;
        return 1U;
    }

    step = Encoder_DecodeStep(encoder->last_state, current_state);
    if (encoder->reverse != 0U) {
        step = (int8_t)(-step);
    }

    encoder->last_state = current_state;
    encoder->delta = (int16_t)step;
    encoder->count += (int32_t)step;
    Encoder_UpdateState(encoder, step);
    encoder->sequence++;

    return 1U;
}

uint8_t Encoder_Reset(Encoder_t *encoder)
{
    if (encoder == NULL) {
        return 0U;
    }

    encoder->count = 0;
    encoder->delta = 0;
    encoder->state = ENCODER_STATE_STOP;
    encoder->raw_state = 0U;
    encoder->last_state = 0U;
    encoder->has_state = 0U;
    encoder->sequence = 0U;

    return 1U;
}
