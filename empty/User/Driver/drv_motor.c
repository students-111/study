/**
 * @file drv_motor.c
 * @brief 简单有刷电机驱动库实现。
 */

/* ======== include ======== */

#include "drv_motor.h"

#include <stdbool.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 内部函数 ======== */

/**
 * @brief 限制矢量占空比输出范围。
 * @param output_permille 输入矢量占空比。
 * @return 限幅后的矢量占空比。
 */
static int16_t Motor_LimitOutputPermille(int16_t output_permille)
{
    if (output_permille > MOTOR_OUTPUT_PERMILLE_MAX) {
        return MOTOR_OUTPUT_PERMILLE_MAX;
    }
    if (output_permille < -MOTOR_OUTPUT_PERMILLE_MAX) {
        return -MOTOR_OUTPUT_PERMILLE_MAX;
    }
    return output_permille;
}

/**
 * @brief 获取矢量占空比绝对值。
 * @param output_permille 矢量占空比。
 * @return 占空比绝对值，单位千分比。
 */
static uint16_t Motor_AbsOutputPermille(int16_t output_permille)
{
    return (output_permille < 0) ?
        (uint16_t)(-output_permille) : (uint16_t)output_permille;
}

/**
 * @brief 根据矢量占空比和安装方向判断电机状态。
 * @param output_permille 矢量占空比。
 * @param reverse 安装方向，0 表示正装，1 表示反装。
 * @return 电机状态。
 */
static MotorState_t Motor_GetState(int16_t output_permille, uint8_t reverse)
{
    if (output_permille == 0) {
        return MOTOR_STATE_STOP;
    }
    if (((output_permille > 0) && (reverse == 0U)) ||
        ((output_permille < 0) && (reverse != 0U))) {
        return MOTOR_STATE_FORWARD;
    }
    return MOTOR_STATE_BACKWARD;
}

/**
 * @brief 写入 TB6612 方向脚。
 * @param motor 电机实体。
 * @param state 电机目标状态。
 * @return 写入成功返回 1，否则返回 0。
 */
static uint8_t Motor_WriteDirection(Motor_t *motor, MotorState_t state)
{
    bool pin1_high = false;
    bool pin2_high = false;

    if (motor == NULL) {
        return 0U;
    }

    if (state == MOTOR_STATE_FORWARD) {
        pin1_high = true;
    } else if (state == MOTOR_STATE_BACKWARD) {
        pin2_high = true;
    }

    if (pin1_high) {
        DL_GPIO_setPins(motor->hw.dir_port1, motor->hw.dir_pin1);
    } else {
        DL_GPIO_clearPins(motor->hw.dir_port1, motor->hw.dir_pin1);
    }

    if (pin2_high) {
        DL_GPIO_setPins(motor->hw.dir_port2, motor->hw.dir_pin2);
    } else {
        DL_GPIO_clearPins(motor->hw.dir_port2, motor->hw.dir_pin2);
    }

    return 1U;
}

/* ======== 公开 API ======== */

uint8_t Motor_Init(Motor_t *motor, const MotorHW_t *hw, uint8_t reverse)
{
    if ((motor == NULL) || (hw == NULL)) {
        return 0U;
    }

    motor->hw = *hw;
    motor->output_permille = 0;
    motor->state = MOTOR_STATE_STOP;
    motor->enable = 1U;
    motor->reverse = (reverse != 0U) ? 1U : 0U;

    if (!Motor_WriteDirection(motor, MOTOR_STATE_STOP)) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }
    if (bsp_pwm_set_duty(motor->hw.channel, 0U) != BSP_OK) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }

    return 1U;
}

uint8_t Motor_Enable(Motor_t *motor, uint8_t enable)
{
    if (motor == NULL) {
        return 0U;
    }

    motor->enable = (enable != 0U) ? 1U : 0U;
    if (motor->enable == 0U) {
        return Motor_Stop(motor);
    }
    return 1U;
}

uint8_t Motor_SetOutput(Motor_t *motor, int16_t output_permille)
{
    int16_t limited_output;
    uint16_t duty_permille;
    MotorState_t next_state;

    if (motor == NULL) {
        return 0U;
    }
    if (motor->enable == 0U) {
        return Motor_Stop(motor);
    }

    limited_output = Motor_LimitOutputPermille(output_permille);
    next_state = Motor_GetState(limited_output, motor->reverse);
    duty_permille = Motor_AbsOutputPermille(limited_output);

    if (!Motor_WriteDirection(motor, next_state)) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }
    if (bsp_pwm_set_duty(motor->hw.channel, duty_permille) != BSP_OK) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }

    motor->output_permille = limited_output;
    motor->state = next_state;
    return 1U;
}

uint8_t Motor_Stop(Motor_t *motor)
{
    if (motor == NULL) {
        return 0U;
    }

    if (!Motor_WriteDirection(motor, MOTOR_STATE_STOP)) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }
    if (bsp_pwm_set_duty(motor->hw.channel, 0U) != BSP_OK) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }

    motor->output_permille = 0;
    motor->state = MOTOR_STATE_STOP;
    return 1U;
}
