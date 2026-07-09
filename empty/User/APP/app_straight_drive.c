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
    bool running;              /**< 当前是否已经使能速度内环输出。 */
    bool has_target_yaw;       /**< 是否已经锁定目标航向。 */
    bool has_left_encoder_count;  /**< 左轮是否已建立编码器计数基准。 */
    bool has_right_encoder_count; /**< 右轮是否已建立编码器计数基准。 */
    int32_t target_yaw_mdeg;   /**< 目标航向角，单位 0.001 度。 */
    int32_t last_left_encoder_count;  /**< 左轮上一控制周期编码器累计值。 */
    int32_t last_right_encoder_count; /**< 右轮上一控制周期编码器累计值。 */
} app_straight_drive_state_t;

/* ======== 内部变量 ======== */

static app_straight_drive_state_t g_app_straight_drive_state;

/* ======== 内部函数 ======== */

/**
 * @brief 将 PID 浮点输出转换为 PWM 千分比命令。
 * @param value PID 浮点输出。
 * @return 四舍五入后的 PWM 千分比命令。
 */
static int16_t app_straight_drive_pid_to_permille(float value)
{
    if (value >= 0.0f) {
        return (int16_t)(value + APP_STRAIGHT_DRIVE_ROUND_OFFSET);
    }
    return (int16_t)(value - APP_STRAIGHT_DRIVE_ROUND_OFFSET);
}

/**
 * @brief 复位直行模式使用的速度内环运行态。
 * @param void 无参数。
 * @return 无。
 */
static void app_straight_drive_reset_speed_runtime(void)
{
    g_app_straight_drive_state.running = false;
    g_app_straight_drive_state.has_left_encoder_count = false;
    g_app_straight_drive_state.has_right_encoder_count = false;
    g_app_straight_drive_state.last_left_encoder_count = 0;
    g_app_straight_drive_state.last_right_encoder_count = 0;
    PID_Reset(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
}

/**
 * @brief 根据编码器累计值计算单轮本周期速度。
 * @param encoder_sample 编码器快照。
 * @param has_last_count 是否已经建立上一周期计数基准。
 * @param last_count 上一周期计数基准。
 * @param out_counts_per_period 输出本周期速度，单位 counts/control-period。
 * @return 速度有效返回 true，否则返回 false。
 */
static bool app_straight_drive_calc_wheel_speed(
    const dal_encoder_sample_t *encoder_sample,
    bool *has_last_count,
    int32_t *last_count,
    int16_t *out_counts_per_period)
{
    int32_t count_delta;

    if ((encoder_sample->sequence == 0U) ||
        (encoder_sample->direction == DAL_ENCODER_DIR_ERROR)) {
        *has_last_count = false;
        *out_counts_per_period = 0;
        return false;
    }

    if (!*has_last_count) {
        *last_count = encoder_sample->count;
        *has_last_count = true;
        *out_counts_per_period = 0;
        return false;
    }

    count_delta = encoder_sample->count - *last_count;
    *last_count = encoder_sample->count;

    if (count_delta > INT16_MAX) {
        count_delta = INT16_MAX;
    } else if (count_delta < INT16_MIN) {
        count_delta = INT16_MIN;
    }

    *out_counts_per_period = (int16_t)count_delta;
    return true;
}

/**
 * @brief 计算两个 Yaw 角之间的最短角差。
 * @param current_mdeg 当前 Yaw 角，单位 0.001 度。
 * @param target_mdeg 目标 Yaw 角，单位 0.001 度。
 * @return 最短角差，单位 0.001 度。
 */
static int32_t app_straight_drive_calc_error(int32_t current_mdeg,
    int32_t target_mdeg)
{
    int32_t error = current_mdeg - target_mdeg;

    while (error >= APP_STRAIGHT_DRIVE_YAW_HALF_TURN_MDEG) {
        error -= APP_STRAIGHT_DRIVE_YAW_FULL_TURN_MDEG;
    }
    while (error < -APP_STRAIGHT_DRIVE_YAW_HALF_TURN_MDEG) {
        error += APP_STRAIGHT_DRIVE_YAW_FULL_TURN_MDEG;
    }
    return error;
}

/**
 * @brief 下发左右轮目标速度并确保速度内环使能。
 * @param left_target 左轮目标速度。
 * @param right_target 右轮目标速度。
 * @return 无。
 */
static void app_straight_drive_apply_targets(int16_t left_target,
    int16_t right_target)
{
    int16_t left_measured;
    int16_t right_measured;
    float output;
    bool left_valid;
    bool right_valid;

    left_valid = app_straight_drive_calc_wheel_speed(
        &g_dal_encoder_sample[DAL_ENCODER_M1],
        &g_app_straight_drive_state.has_left_encoder_count,
        &g_app_straight_drive_state.last_left_encoder_count,
        &left_measured);
    right_valid = app_straight_drive_calc_wheel_speed(
        &g_dal_encoder_sample[DAL_ENCODER_M2],
        &g_app_straight_drive_state.has_right_encoder_count,
        &g_app_straight_drive_state.last_right_encoder_count,
        &right_measured);

    if (!left_valid || !right_valid) {
        dal_motor_stop_all();
        return;
    }

    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_target);
    output = PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_measured, 0U);
    (void)dal_motor_set_speed(DAL_MOTOR_LEFT,
        app_straight_drive_pid_to_permille(output));

    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_target);
    output = PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_measured, 0U);
    (void)dal_motor_set_speed(DAL_MOTOR_RIGHT,
        app_straight_drive_pid_to_permille(output));

    g_app_straight_drive_state.running = true;
}

/* ======== 公开 API ======== */

void app_straight_drive_init(void)
{
    dal_pid_init();
    dal_motor_init();
    app_straight_drive_stop();
}

void app_straight_drive_enter(void)
{
    app_straight_drive_stop();

    if ((g_dal_mpu6050_sample.sequence != 0U) &&
        g_dal_mpu6050_sample.yaw_calibrated) {
        g_app_straight_drive_state.target_yaw_mdeg =
            g_dal_mpu6050_sample.yaw_mdeg;
        g_app_straight_drive_state.has_target_yaw = true;
    }
}

void app_straight_drive_stop(void)
{
    dal_motor_stop_all();
    app_straight_drive_reset_speed_runtime();
    g_app_straight_drive_state.has_target_yaw = false;
    g_app_straight_drive_state.target_yaw_mdeg = 0;
}

void app_straight_drive_refresh(void)
{
    int32_t error_mdeg;
    int16_t turn;
    int16_t left_target;
    int16_t right_target;
    float angle_output;

    if ((g_dal_mpu6050_sample.sequence == 0U) ||
        !g_dal_mpu6050_sample.yaw_calibrated) {
        dal_motor_stop_all();
        app_straight_drive_reset_speed_runtime();
        return;
    }

    if (!g_app_straight_drive_state.has_target_yaw) {
        g_app_straight_drive_state.target_yaw_mdeg =
            g_dal_mpu6050_sample.yaw_mdeg;
        g_app_straight_drive_state.has_target_yaw = true;
    }

    error_mdeg = app_straight_drive_calc_error(
        g_dal_mpu6050_sample.yaw_mdeg,
        g_app_straight_drive_state.target_yaw_mdeg);
    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE], 0.0f);
    angle_output = PID_Compute1_Rectangle(
        &pid_gather[DAL_PID_ID_STRAIGHT_ANGLE], (float)error_mdeg, 0U);
    turn = app_straight_drive_pid_to_permille(angle_output);

    left_target = (int16_t)(APP_STRAIGHT_DRIVE_BASE_SPEED_CP - turn);
    right_target = (int16_t)(APP_STRAIGHT_DRIVE_BASE_SPEED_CP + turn);

    app_straight_drive_apply_targets(left_target, right_target);
}
