/**
 * @file app_straight_drive.c
 * @brief Yaw 直线保持 APP 实现。
 */

/* ======== include ======== */

#include "app_straight_drive.h"

#include <stdbool.h>
#include <stdint.h>

#include "PID.h"
#include "dal_encoder.h"
#include "dal_motor.h"
#include "dal_mpu6050.h"
#include "dal_pid.h"

/* ======== 类型定义 ======== */

typedef struct {
    bool has_target_yaw;       /**< 是否已经锁定目标航向。 */
    int32_t target_yaw_mdeg;   /**< 目标航向角，单位 0.001 度。 */
} app_straight_drive_state_t;

/* ======== 内部变量 ======== */

static app_straight_drive_state_t g_app_straight_drive_state;

/* ======== 公开 API ======== */

void app_straight_drive_init(void)
{
    dal_pid_init();
    dal_motor_init();

    /* 停止电机。 */
    dal_motor_stop_all();
    /* 复位 PID。 */
    PID_Reset(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
    g_app_straight_drive_state.has_target_yaw = false;
    g_app_straight_drive_state.target_yaw_mdeg = 0;
}

void app_straight_drive_enter(void)
{
    app_straight_drive_enter_with_offset(0L);
}

void app_straight_drive_enter_with_offset(int32_t yaw_offset_mdeg)
{
    app_straight_drive_stop();

    if ((g_dal_mpu6050_sample.sequence != 0U) &&
        g_dal_mpu6050_sample.yaw_calibrated) {
        g_app_straight_drive_state.target_yaw_mdeg =
            g_dal_mpu6050_sample.yaw_mdeg + yaw_offset_mdeg;
        g_app_straight_drive_state.has_target_yaw = true;
    }
}

void app_straight_drive_stop(void)
{
    /* 停止电机。 */
    dal_motor_stop_all();
    /* 复位 PID。 */
    PID_Reset(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
    g_app_straight_drive_state.has_target_yaw = false;
    g_app_straight_drive_state.target_yaw_mdeg = 0;
}

void app_straight_drive_refresh(void)
{
    int16_t turn_cp;
    int16_t left_target_cp;
    int16_t right_target_cp;
    int16_t left_measured_cp;
    int16_t right_measured_cp;
    int16_t output_permille;
    int32_t error_mdeg;
    float angle_output;
    bool left_valid;
    bool right_valid;

    /* 如果没有刷新过或者 Yaw 未校准，就停止所有电机。 */
    if ((g_dal_mpu6050_sample.sequence == 0U) ||
        !g_dal_mpu6050_sample.yaw_calibrated) {
        dal_motor_stop_all();
        PID_Reset(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE]);
        PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
        PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
        return;
    }

    /* 如果没有目标航向，就用当前 Yaw 建立基准。 */
    if (!g_app_straight_drive_state.has_target_yaw) {
        g_app_straight_drive_state.target_yaw_mdeg =
            g_dal_mpu6050_sample.yaw_mdeg;
        g_app_straight_drive_state.has_target_yaw = true;
    }

    /* 计算最短 Yaw 角误差。 */
    error_mdeg = g_dal_mpu6050_sample.yaw_mdeg -
        g_app_straight_drive_state.target_yaw_mdeg;
    while (error_mdeg >= APP_STRAIGHT_DRIVE_YAW_HALF_TURN_MDEG) {
        error_mdeg -= APP_STRAIGHT_DRIVE_YAW_FULL_TURN_MDEG;
    }
    while (error_mdeg < -APP_STRAIGHT_DRIVE_YAW_HALF_TURN_MDEG) {
        error_mdeg += APP_STRAIGHT_DRIVE_YAW_FULL_TURN_MDEG;
    }

    /* 直行角度环 PID 计算。 */
    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE], 0.0f);
    angle_output = PID_Compute1_Rectangle(
        &pid_gather[DAL_PID_ID_STRAIGHT_ANGLE], (float)error_mdeg, 0U);
    turn_cp = (int16_t)(angle_output);

    /* 差速计算。 */
    left_target_cp = (int16_t)(APP_STRAIGHT_DRIVE_BASE_SPEED_CP - turn_cp);
    right_target_cp = (int16_t)(APP_STRAIGHT_DRIVE_BASE_SPEED_CP + turn_cp);

    /* 是否建立编码器测速基准。 */
    left_valid = g_dal_encoder_sample[DAL_ENCODER_M1].speed_valid;
    right_valid = g_dal_encoder_sample[DAL_ENCODER_M2].speed_valid;
    left_measured_cp = g_dal_encoder_sample[DAL_ENCODER_M1].speed_cp;
    right_measured_cp = g_dal_encoder_sample[DAL_ENCODER_M2].speed_cp;

    /* 如果没有建立测速基准，就停止所有电机。 */
    if (!left_valid || !right_valid) {
        dal_motor_stop_all();
        return;
    }

    /* 速度环 PID 计算。 */
    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_target_cp);
    output_permille = PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_measured_cp, 0U);

    (void)dal_motor_set_output(DAL_MOTOR_LEFT, output_permille);

    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_target_cp);
    output_permille = PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_measured_cp, 0U);
    (void)dal_motor_set_output(DAL_MOTOR_RIGHT, output_permille);
}
