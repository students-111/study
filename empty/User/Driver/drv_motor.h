/**
 * @file drv_motor.h
 * @brief 简单有刷电机驱动库接口。
 */

#ifndef DRV_MOTOR_H
#define DRV_MOTOR_H

/* ======== include ======== */

#include <stdint.h>

#include "bsp_pwm.h"

/* ======== 可调参数宏定义 ======== */

/* 电机矢量占空比最大值；1000 表示 100% 输出。 */
#define MOTOR_OUTPUT_PERMILLE_MAX       (1000)

/* 电机 PWM 占空比最大千分比；1000 表示 100%。 */
#define MOTOR_PWM_PERMILLE_MAX          (1000U)

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
    void *dir_port1;             /**< TB6612 IN1 GPIO 端口。 */
    uint32_t dir_pin1;           /**< TB6612 IN1 GPIO pin mask。 */
    void *dir_port2;             /**< TB6612 IN2 GPIO 端口。 */
    uint32_t dir_pin2;           /**< TB6612 IN2 GPIO pin mask。 */
} MotorHW_t;

/**
 * @brief 电机驱动实体结构体。
 */
typedef struct {
    MotorHW_t hw;                /**< 硬件配置。 */
    int16_t output_permille;     /**< 当前矢量占空比，范围 -1000 到 1000。 */
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
 * @brief 设置电机矢量占空比输出。
 * @param motor 电机实体。
 * @param output_permille 矢量占空比，范围 -1000 到 1000。
 * @return 设置成功返回 1，否则返回 0。
 */
uint8_t Motor_SetOutput(Motor_t *motor, int16_t output_permille);

/**
 * @brief 停止电机。
 * @param motor 电机实体。
 * @return 停止成功返回 1，否则返回 0。
 */
uint8_t Motor_Stop(Motor_t *motor);

#endif /* DRV_MOTOR_H */
