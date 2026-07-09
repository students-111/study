/**
 * @file drv_motor.h
 * @brief 简单有刷电机驱动库接口。
 */

#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H

/* ======== include ======== */

#include <stdint.h>

#include "bsp_gpio.h"
#include "bsp_pwm.h"

/* ======== 可调参数宏定义 ======== */

/* 电机速度命令最大值；100 表示 100% 输出。 */
#define MOTOR_SPEED_MAX                 (100)

/* 电机 PWM 占空比最大千分比；1000 表示 100%。 */
#define MOTOR_PWM_PERMILLE_MAX          (1000U)

/* 速度百分比换算 PWM 千分比的倍率。 */
#define MOTOR_SPEED_TO_PERMILLE_SCALE   (10U)

/* ======== 类型定义 ======== */

/**
 * @brief 电机状态枚举。
 */
typedef enum {
    MOTOR_STATE_STOP = 0,        /**< 停止。 */
    MOTOR_STATE_FORWARD,         /**< 正转。 */
    MOTOR_STATE_BACKWARD,        /**< 反转。 */
    MOTOR_STATE_ERROR            /**< 错误。 */
} MotorState_t;

/**
 * @brief 电机硬件配置结构体。
 */
typedef struct {
    void *htim;                  /**< PWM 定时器句柄，本工程由 BSP 持有。 */
    bsp_pwm_channel_e channel;   /**< PWM 通道。 */
    void *dir_port;              /**< 方向 GPIO 端口，本工程由 BSP 持有。 */
    bsp_gpio_pin_e dir_pin1;     /**< TB6612 IN1 方向引脚。 */
    bsp_gpio_pin_e dir_pin2;     /**< TB6612 IN2 方向引脚。 */
} MotorHW_t;

/**
 * @brief 电机驱动实体结构体。
 */
typedef struct {
    MotorHW_t hw;                /**< 硬件配置。 */
    int8_t speed;                /**< 当前速度，范围 -100 到 100。 */
    MotorState_t state;          /**< 当前状态。 */
    uint8_t enable;              /**< 使能标志。 */
    uint8_t reverse;             /**< 电机安装方向，0 表示正装，1 表示反装。 */
} Motor_t;

/* ======== 公开 API ======== */

/**
 * @brief 初始化电机驱动实体。
 * @param motor 电机实体。
 * @param hw 硬件配置。
 * @param reverse 安装方向，0 表示正装，1 表示反装。
 * @return 初始化成功返回 1，否则返回 0。
 */
uint8_t Motor_Init(Motor_t *motor, const MotorHW_t *hw, uint8_t reverse);

/**
 * @brief 使能或失能电机。
 * @param motor 电机实体。
 * @param enable 1 表示使能，0 表示失能。
 * @return 设置成功返回 1，否则返回 0。
 */
uint8_t Motor_Enable(Motor_t *motor, uint8_t enable);

/**
 * @brief 设置电机速度。
 * @param motor 电机实体。
 * @param speed 速度命令，范围 -100 到 100。
 * @return 设置成功返回 1，否则返回 0。
 */
uint8_t Motor_SetSpeed(Motor_t *motor, int8_t speed);

/**
 * @brief 停止电机。
 * @param motor 电机实体。
 * @return 停止成功返回 1，否则返回 0。
 */
uint8_t Motor_Stop(Motor_t *motor);

#endif /* DRV_MOTOR_H */
