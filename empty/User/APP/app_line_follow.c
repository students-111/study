/**
 * @file app_line_follow.c
 * @brief 灰度循迹 APP 实现。
 */

/* ======== include ======== */

#include "app_line_follow.h"

#include <stdbool.h>
#include <stdint.h>

#include "PID.h"
#include "dal_encoder.h"
#include "dal_gray.h"
#include "dal_motor.h"
#include "dal_pid.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

typedef struct {
    bool running;                    /**< 当前是否正在输出循迹控制。 */
    bool has_left_encoder_count;     /**< 左轮是否已建立编码器计数基准。 */
    bool has_right_encoder_count;    /**< 右轮是否已建立编码器计数基准。 */
    int32_t last_left_encoder_count; /**< 左轮上一控制周期编码器累计值。 */
    int32_t last_right_encoder_count;/**< 右轮上一控制周期编码器累计值。 */
} app_line_follow_state_t;

/* ======== 内部变量 ======== */

static app_line_follow_state_t g_app_line_follow_state;

/* ======== 内部函数 ======== */

/**
 * @brief 将 PID 浮点输出转换为 int16_t 控制量。
 * @param value PID 浮点输出。
 * @return 四舍五入后的 int16_t 控制量。
 */
static int16_t app_line_follow_pid_to_i16(float value)
{
    if (value >= 0.0f) {
        return (int16_t)(value + APP_LINE_FOLLOW_ROUND_OFFSET);
    }
    return (int16_t)(value - APP_LINE_FOLLOW_ROUND_OFFSET);
}

/**
 * @brief 复位循迹 APP 的编码器基准和 PID 运行态。
 * @param void 无参数。
 * @return 无。
 */
static void app_line_follow_reset_runtime(void)
{
    g_app_line_follow_state.running = false;
    g_app_line_follow_state.has_left_encoder_count = false;
    g_app_line_follow_state.has_right_encoder_count = false;
    g_app_line_follow_state.last_left_encoder_count = 0;
    g_app_line_follow_state.last_right_encoder_count = 0;

    PID_Reset(&pid_gather[DAL_PID_ID_LINE]);
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
static bool app_line_follow_calc_wheel_speed(
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
 * @brief 停止全部循迹电机输出。
 * @param void 无参数。
 * @return 无。
 */
static void app_line_follow_stop_motors(void)
{
    dal_motor_stop_all();
}

/* ======== 公开 API ======== */

void app_line_follow_stop(void)
{
    app_line_follow_stop_motors();
    app_line_follow_reset_runtime();
}

void app_line_follow_init(void)
{
    dal_pid_init();
    dal_motor_init();
    app_line_follow_stop();
}

void app_line_follow_refresh(void)
{
    int16_t turn_cp;
    int16_t left_target_cp;
    int16_t right_target_cp;
    int16_t left_measured_cp;
    int16_t right_measured_cp;
    int16_t output_permille;
    float pid_output;
    bool left_valid;
    bool right_valid;

    if ((g_dal_gray_sample.sequence == 0U) ||
        !g_dal_gray_sample.line_detected) {
//        app_line_follow_stop();
        return;
    }

    /*循迹环PID计算*/
    pid_output = PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_LINE],
        g_dal_gray_sample.position, 0U);
    turn_cp = app_line_follow_pid_to_i16(pid_output);
    
    /*差速计算*/
    left_target_cp = (int16_t)(APP_LINE_FOLLOW_BASE_SPEED_CP - turn_cp);
    right_target_cp = (int16_t)(APP_LINE_FOLLOW_BASE_SPEED_CP + turn_cp);

    /*获取速度 返回值是是否有效*/
    left_valid = app_line_follow_calc_wheel_speed(
        &g_dal_encoder_sample[DAL_ENCODER_M1],
        &g_app_line_follow_state.has_left_encoder_count,
        &g_app_line_follow_state.last_left_encoder_count,
        &left_measured_cp);
    right_valid = app_line_follow_calc_wheel_speed(
        &g_dal_encoder_sample[DAL_ENCODER_M2],
        &g_app_line_follow_state.has_right_encoder_count,
        &g_app_line_follow_state.last_right_encoder_count,
        &right_measured_cp);

    /*编码器返回异常停止电机 */
    if (!left_valid || !right_valid) {
        app_line_follow_stop_motors();
        return;
    }

    /* 速度环 PID 计算。 */
    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_target_cp);
    output_permille = app_line_follow_pid_to_i16(
        PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_SPEED_LEFT],
            (float)left_measured_cp, 0U));
    (void)dal_motor_set_speed(DAL_MOTOR_LEFT, output_permille);

    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_target_cp);
    output_permille = app_line_follow_pid_to_i16(
        PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
            (float)right_measured_cp, 0U));
    (void)dal_motor_set_speed(DAL_MOTOR_RIGHT, output_permille);

    g_app_line_follow_state.running = true;
}
