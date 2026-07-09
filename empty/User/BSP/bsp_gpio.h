/**
 * @file bsp_gpio.h
 * @brief 通用 GPIO BSP 接口。
 */

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

/* ======== include ======== */

#include <stdbool.h>

#include "bsp_status.h"

/* ======== 类型定义 ======== */

/**
 * @brief BSP 可操作的板级 GPIO 信号。
 *
 * 上层只能使用这个枚举描述板级信号，不直接接触 DriverLib 的 GPIO 端口、
 * PINCM、物理 PA/PB 编号或 DL_GPIO_PIN_x 宏。
 */
typedef enum {
    BSP_GPIO_KEY1 = 0,        /**< 板载功能按键 Key1。 */
    BSP_GPIO_MOTOR_AIN1,      /**< TB6612 左电机 AIN1 方向信号。 */
    BSP_GPIO_MOTOR_AIN2,      /**< TB6612 左电机 AIN2 方向信号。 */
    BSP_GPIO_MOTOR_BIN1,      /**< TB6612 右电机 BIN1 方向信号。 */
    BSP_GPIO_MOTOR_BIN2,      /**< TB6612 右电机 BIN2 方向信号。 */
    BSP_GPIO_MOTOR_STBY,      /**< TB6612 待机控制信号。 */
    BSP_GPIO_ENC_M1_A,        /**< M1 编码器 A 相信号。 */
    BSP_GPIO_ENC_M1_B,        /**< M1 编码器 B 相信号。 */
    BSP_GPIO_ENC_M2_A,        /**< M2 编码器 A 相信号。 */
    BSP_GPIO_ENC_M2_B,        /**< M2 编码器 B 相信号。 */
    BSP_GPIO_GRAY_D1,         /**< 灰度传感器 D1 信号。 */
    BSP_GPIO_GRAY_D2,         /**< 灰度传感器 D2 信号。 */
    BSP_GPIO_GRAY_D3,         /**< 灰度传感器 D3 信号。 */
    BSP_GPIO_GRAY_D4,         /**< 灰度传感器 D4 信号。 */
    BSP_GPIO_GRAY_D5,         /**< 灰度传感器 D5 信号。 */
    BSP_GPIO_GRAY_D6,         /**< 灰度传感器 D6 信号。 */
    BSP_GPIO_GRAY_D7,         /**< 灰度传感器 D7 信号。 */
    BSP_GPIO_GRAY_D8,         /**< 灰度传感器 D8 信号。 */
    BSP_GPIO_COUNT        /**< BSP GPIO 引脚数量。 */
} bsp_gpio_pin_e;

/**
 * @brief GPIO 输入上下拉配置。
 */
typedef enum {
    BSP_GPIO_PULL_NONE = 0,    /**< 无上下拉。 */
    BSP_GPIO_PULL_UP,          /**< 上拉。 */
    BSP_GPIO_PULL_DOWN         /**< 下拉。 */
} bsp_gpio_pull_e;

/* ======== 公开 API ======== */

/**
 * @brief 初始化 GPIO 输入。
 * @param pin BSP GPIO 引脚。
 * @param pull 输入上下拉配置。
 * @param hysteresis_enable true 表示开启输入滞回。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_gpio_init_input(bsp_gpio_pin_e pin, bsp_gpio_pull_e pull,
    bool hysteresis_enable);

/**
 * @brief 初始化 GPIO 输出，默认输出低电平并使能输出。
 * @param pin BSP GPIO 引脚。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_gpio_init_output(bsp_gpio_pin_e pin);

/**
 * @brief 读取 GPIO 当前电平。
 * @param pin BSP GPIO 引脚。
 * @param out 输出电平，true 表示高电平。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_gpio_read(bsp_gpio_pin_e pin, bool *out);

/**
 * @brief 写入 GPIO 输出电平。
 * @param pin BSP GPIO 引脚。
 * @param high true 表示输出高电平，false 表示输出低电平。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_gpio_write(bsp_gpio_pin_e pin, bool high);

#endif /* BSP_GPIO_H */
