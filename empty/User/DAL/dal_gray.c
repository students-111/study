/**
 * @file dal_gray.c
 * @brief 八路灰度传感器数据转换 DAL 实现。
 */

/* ======== include ======== */

#include "dal_gray.h"

#include <stddef.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

/**
 * @brief 灰度传感器 GPIO 描述。
 */
typedef struct {
    GPIO_Regs *port;       /**< GPIO 端口。 */
    uint32_t pin;          /**< GPIO pin mask。 */
} dal_gray_gpio_t;


//对外暴露的灰度传感器数据接口
dal_gray_sample_t g_dal_gray_sample;


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
static const dal_gray_gpio_t g_dal_gray_pins[DAL_GRAY_SENSOR_COUNT] = {
    {
        BOARD_GPIO_PORT,//GPIOB
        BOARD_GPIO_GRAY_D1_PIN  //PIN18
    },
    {
        BOARD_GPIO_PORT,
        BOARD_GPIO_GRAY_D2_PIN
    },
    {
        BOARD_GPIO_PORT,
        BOARD_GPIO_GRAY_D3_PIN
    },
    {
        BOARD_GPIO_PORT,
        BOARD_GPIO_GRAY_D4_PIN
    },
    {
        BOARD_GPIO_PORT,
        BOARD_GPIO_GRAY_D5_PIN
    },
    {
        BOARD_GPIO_PORT,
        BOARD_GPIO_GRAY_D6_PIN
    },
    {
        BOARD_GPIO_PORT,
        BOARD_GPIO_GRAY_D7_PIN
    },
    {
        BOARD_GPIO_PORT,
        BOARD_GPIO_GRAY_D8_PIN
    }
};



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
        const dal_gray_gpio_t *gpio = &g_dal_gray_pins[idx];

        if (DAL_GRAY_ACTIVE_LEVEL_HIGH != 0U) {
            if ((DL_GPIO_readPins(gpio->port, gpio->pin) & gpio->pin) != 0U) {
                raw |= (uint8_t)(1U << idx);
            }
        } else if ((DL_GPIO_readPins(gpio->port, gpio->pin) & gpio->pin) == 0U) {
            raw |= (uint8_t)(1U << idx);
        }
    }

    return raw;
}

/* ======== 公开 API ======== */

void dal_gray_init(void)
{
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
    count = dal_gray_count_bits(raw);//看看灰度传感器有多少置位了

    g_dal_gray_sample.raw_mask = raw;//刷新对应置位
    g_dal_gray_sample.active_count = count;//刷新置位数
    g_dal_gray_sample.line_detected = (count != 0U);//如果有置位就检测到了线
    g_dal_gray_sample.cross_detected = (count >= DAL_GRAY_CROSS_MIN_COUNT);//判断是否直角转弯
    g_dal_gray_sample.position = dal_gray_calc_position(raw, count);//刷新加权数 
    g_dal_gray_sample.sequence++;//调试用变量
}
