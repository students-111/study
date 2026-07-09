/**
 * @file dal_gray.c
 * @brief 八路灰度传感器数据转换 DAL 实现。
 */

/* ======== include ======== */

#include "dal_gray.h"

#include <stddef.h>

#include "bsp_gpio.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 内部变量 ======== */

/*
 * D1~D8 从左到右排列；权重为左负右正，dal_gray_calc_position()
 * 对检测到线的通道取平均值，得到车身相对中心线的偏移方向和大小。
 */
static const float g_dal_gray_weights[DAL_GRAY_SENSOR_COUNT] = {
    DAL_GRAY_WEIGHT_VALUES
};

/*
 * 该表与 g_dal_gray_weights[] 使用同一索引：idx=0 对应 D1/bit0，
 * idx=7 对应 D8/bit7，保证 raw_mask、权重和实际 GPIO 一一对应。
 */
static const bsp_gpio_pin_e g_dal_gray_pins[DAL_GRAY_SENSOR_COUNT] = {
    BSP_GPIO_GRAY_D1,
    BSP_GPIO_GRAY_D2,
    BSP_GPIO_GRAY_D3,
    BSP_GPIO_GRAY_D4,
    BSP_GPIO_GRAY_D5,
    BSP_GPIO_GRAY_D6,
    BSP_GPIO_GRAY_D7,
    BSP_GPIO_GRAY_D8
};

dal_gray_sample_t g_dal_gray_sample;

/* ======== 内部函数 ======== */

/**
 * @brief 统计位图中置位的通道数量。
 * @param mask 待统计位图。
 * @return 置位数量。
 */
static uint8_t dal_gray_count_bits(uint8_t mask)
{
    uint8_t count = 0U;
    uint8_t idx;

    for (idx = 0U; idx < DAL_GRAY_SENSOR_COUNT; idx++) {
        if ((mask & (uint8_t)(1U << idx)) != 0U) {
            count++;
        }
    }
    return count;
}

/**
 * @brief 计算灰度有效通道的加权线位。
 * @param raw_mask 八路灰度检测位图，1 表示检测到线。
 * @param active_count 有效通道数量。
 * @return 加权线位，左负右正。
 */
static float dal_gray_calc_position(uint8_t raw_mask, uint8_t active_count)
{
    float sum = 0.0f;
    uint8_t idx;

    if (active_count == 0U) {
        return 0.0f;
    }

    for (idx = 0U; idx < DAL_GRAY_SENSOR_COUNT; idx++) {
        if ((raw_mask & (uint8_t)(1U << idx)) != 0U) {
            sum += g_dal_gray_weights[idx];
        }
    }

    return sum / (float)active_count;
}

/**
 * @brief 读取八路灰度传感器原始电平并压缩为 bit0~bit7。
 * @param void 无参数。
 * @return bit0~bit7 分别对应 D1~D8；读取失败的通道按低电平处理。
 */
static uint8_t dal_gray_read_raw(void)
{
    uint8_t raw = 0U;
    uint8_t idx;

    for (idx = 0U; idx < DAL_GRAY_SENSOR_COUNT; idx++) {
        bool high = false;

        if ((bsp_gpio_read(g_dal_gray_pins[idx], &high) == BSP_OK) && high) {
            raw |= (uint8_t)(1U << idx);
        }
    }

    return raw;
}

/* ======== 公开 API ======== */

void dal_gray_init(void)
{
    uint8_t idx;

    for (idx = 0U; idx < DAL_GRAY_SENSOR_COUNT; idx++) {
        /*
         * WHY: 灰度模块输出线可能在接线或模块未稳定时悬空，上拉和滞回能让
         * 软件采样获得明确默认电平。
         */
        (void)bsp_gpio_init_input(g_dal_gray_pins[idx], BSP_GPIO_PULL_UP, true);
    }

    g_dal_gray_sample.raw_mask = 0U;
    g_dal_gray_sample.active_count = 0U;
    g_dal_gray_sample.line_detected = false;
    g_dal_gray_sample.cross_detected = false;
    g_dal_gray_sample.position = 0.0f;
    g_dal_gray_sample.sequence = 0U;
}

void dal_gray_refresh(void)
{
    uint8_t raw;
    uint8_t count;

    raw = dal_gray_read_raw();
    count = dal_gray_count_bits(raw);

    g_dal_gray_sample.raw_mask = raw;
    g_dal_gray_sample.active_count = count;
    g_dal_gray_sample.line_detected = (count != 0U);
    g_dal_gray_sample.cross_detected = (count >= DAL_GRAY_CROSS_MIN_COUNT);
    g_dal_gray_sample.position = dal_gray_calc_position(raw, count);
    g_dal_gray_sample.sequence++;
}
