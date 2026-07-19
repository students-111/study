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
#include "dal_jy901p.h"
#include "dal_pid.h"

/* ======== 类型定义 ======== */

typedef struct {
    bool has_target_yaw;       /**< 是否已经锁定目标航向。 */
    bool yaw_correction_enabled; /**< 当前直行段是否启用 Yaw 修正。 */
    int32_t target_yaw_raw;   /**< 目标航向角，单位原始角度 LSB。 */
    float last_turn_cp;        /**< 上一次角度差速输出，单位 counts/speed-period。 */
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
    g_app_straight_drive_state.yaw_correction_enabled = false;
    g_app_straight_drive_state.target_yaw_raw = 0;
    g_app_straight_drive_state.last_turn_cp = 0.0f;
}

void app_straight_drive_enter(void)
{
    app_straight_drive_enter_with_offset(0L);
}

void app_straight_drive_enter_with_offset(int32_t yaw_offset_raw)
{
    app_straight_drive_stop();
    g_app_straight_drive_state.yaw_correction_enabled =
        (APP_STRAIGHT_DRIVE_YAW_CORRECTION_ENABLE != 0U) ||
        (yaw_offset_raw != 0L);

    if ((g_dal_jy901p_sample.sequence != 0U) &&
        g_dal_jy901p_sample.angle_reference_ready) {
        g_app_straight_drive_state.target_yaw_raw =
            g_dal_jy901p_sample.yaw_raw + yaw_offset_raw;
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
    g_app_straight_drive_state.yaw_correction_enabled = false;
    g_app_straight_drive_state.target_yaw_raw = 0;
    g_app_straight_drive_state.last_turn_cp = 0.0f;
}

void app_straight_drive_refresh(void)
{
    float turn_cp;
    float turn_delta_cp;
    float left_target_cp;
    float right_target_cp;
    int16_t left_measured_cp;
    int16_t right_measured_cp;
    int16_t output_permille;
    int32_t error_raw;
    float angle_output;
    bool left_valid;
    bool right_valid;

    /* 如果角度快照未刷新或参考未就绪，就停止所有电机。 */
    if (g_app_straight_drive_state.yaw_correction_enabled &&
        ((g_dal_jy901p_sample.sequence == 0U) ||
        !g_dal_jy901p_sample.angle_reference_ready)) {
        dal_motor_stop_all();
        PID_Reset(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE]);
        PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
        PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
        return;
    }

    /* 如果没有目标航向，就用当前 Yaw 建立基准。 */
    if (g_app_straight_drive_state.yaw_correction_enabled &&
        !g_app_straight_drive_state.has_target_yaw) {
        g_app_straight_drive_state.target_yaw_raw =
            g_dal_jy901p_sample.yaw_raw;
        g_app_straight_drive_state.has_target_yaw = true;
    }

    /* 计算目标航向相对当前航向的最短角差。 */
    error_raw = 0L;
    if (g_app_straight_drive_state.yaw_correction_enabled) {
        error_raw = g_app_straight_drive_state.target_yaw_raw -
            g_dal_jy901p_sample.yaw_raw;
    }
    while (error_raw >= APP_STRAIGHT_DRIVE_YAW_HALF_TURN_RAW) {
        error_raw -= APP_STRAIGHT_DRIVE_YAW_FULL_TURN_RAW;
    }
    while (error_raw < -APP_STRAIGHT_DRIVE_YAW_HALF_TURN_RAW) {
        error_raw += APP_STRAIGHT_DRIVE_YAW_FULL_TURN_RAW;
    }

    /* 直行角度环 PID 计算。 */
    if (!g_app_straight_drive_state.yaw_correction_enabled) {
        angle_output = 0.0f;
    } else if ((error_raw <= APP_STRAIGHT_DRIVE_YAW_DEADBAND_RAW) &&
        (error_raw >= -APP_STRAIGHT_DRIVE_YAW_DEADBAND_RAW)) {
        angle_output = 0.0f;
    } else {
        angle_output = PID_ComputeError(
            &pid_gather[DAL_PID_ID_STRAIGHT_ANGLE], (float)error_raw);
        angle_output *= APP_STRAIGHT_DRIVE_POSITIVE_YAW_TURN_SIGN;
    }

    turn_delta_cp = angle_output - g_app_straight_drive_state.last_turn_cp;
    if (turn_delta_cp > APP_STRAIGHT_DRIVE_TURN_STEP_LIMIT_CP) {
        turn_delta_cp = APP_STRAIGHT_DRIVE_TURN_STEP_LIMIT_CP;
    } else if (turn_delta_cp < -APP_STRAIGHT_DRIVE_TURN_STEP_LIMIT_CP) {
        turn_delta_cp = -APP_STRAIGHT_DRIVE_TURN_STEP_LIMIT_CP;
    }
    turn_cp = g_app_straight_drive_state.last_turn_cp + turn_delta_cp;
    g_app_straight_drive_state.last_turn_cp = turn_cp;

    /* 差速计算。 */
    left_target_cp = (float)APP_STRAIGHT_DRIVE_BASE_SPEED_CP + turn_cp -
        APP_STRAIGHT_DRIVE_LEFT_TRIM_CP;
    right_target_cp = (float)APP_STRAIGHT_DRIVE_BASE_SPEED_CP - turn_cp +
        APP_STRAIGHT_DRIVE_LEFT_TRIM_CP;

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
