/**
 * @file app_task4.c
 * @brief 第四题四圈连续行驶状态机实现。
 */

/* ======== include ======== */

#include "app_task4.h"

#include <stdbool.h>
#include <stdint.h>

#include "PID.h"
#include "app_line_follow.h"
#include "app_straight_drive.h"
#include "bsp_time.h"
#include "dal_encoder.h"
#include "dal_gray.h"
#include "dal_key.h"
#include "dal_motor.h"
#include "dal_mpu6050.h"
#include "dal_pid.h"

/* ======== 可调参数宏定义 ======== */

/* 黑白边沿确认所需连续样本数，单位 次；抑制边缘抖动造成的错误切换。 */
#define APP_TASK4_GRAY_CONFIRM_SAMPLE_COUNT        (3U)

/* ======== 类型定义 ======== */

/**
 * @brief 第四题行驶状态。
 */
typedef enum {
    APP_TASK4_STATE_WAIT_KEY1 = 0,      /**< 等待 Key1 启动。 */
    APP_TASK4_STATE_TURN_TO_C,          /**< 低速转向至 A→C 航向。 */
    APP_TASK4_STATE_STRAIGHT_TO_C,      /**< 白底直行至 C 点。 */
    APP_TASK4_STATE_FIND_LINE_TO_B,     /**< 低速差速寻找 C→B 弧线。 */
    APP_TASK4_STATE_FOLLOW_TO_B,        /**< 沿右侧半圆循迹至 B 点。 */
    APP_TASK4_STATE_TURN_TO_D,          /**< 低速转向至 B→D 航向。 */
    APP_TASK4_STATE_STRAIGHT_TO_D,      /**< 白底直行至 D 点。 */
    APP_TASK4_STATE_FIND_LINE_TO_A,     /**< 低速差速寻找 D→A 弧线。 */
    APP_TASK4_STATE_FOLLOW_TO_A,        /**< 沿左侧半圆循迹至 A 点。 */
    APP_TASK4_STATE_FINISHED            /**< 已完成四圈并保持零速度。 */
} app_task4_state_e;

/**
 * @brief 第四题状态机运行数据。
 */
typedef struct {
    app_task4_state_e current_state;    /**< 当前第四题状态。 */
    app_task4_state_e turn_next_state;  /**< 低速航向转向完成后的目标状态。 */
    int32_t target_yaw_mdeg;            /**< 低速航向转向目标 Yaw，单位 0.001 度。 */
    uint32_t key1_handled_sequence;     /**< 已消费的 Key1 快照序号。 */
    uint32_t gray_handled_sequence;     /**< 已消费的灰度快照序号。 */
    uint8_t gray_confirm_count;         /**< 当前灰度状态连续确认次数。 */
    uint8_t completed_laps;             /**< 已完成圈数。 */
    uint32_t line_search_start_ms;      /**< 低速找线开始时刻，单位 ms。 */
    uint32_t arc_capture_start_ms;      /**< 当前弧线捕线开始时刻，单位 ms。 */
    bool arc_exit_armed;                /**< 已稳定循迹，允许离线判定为弧线终点。 */
} app_task4_state_data_t;

/* ======== 内部变量 ======== */

static app_task4_state_data_t g_app_task4_state;

/* ======== 内部函数声明 ======== */

/**
 * @brief 将 Yaw 归一化到正负半圈范围。
 * @param yaw_mdeg 待归一化的 Yaw，单位 0.001 度。
 * @return 归一化后的 Yaw，范围为 [-180000, 180000)。
 */
static int32_t app_task4_normalize_yaw_mdeg(int32_t yaw_mdeg);

/**
 * @brief 判断是否收到尚未消费的 Key1 按下事件。
 * @param void 无参数。
 * @return 收到新按下事件返回 true，否则返回 false。
 */
static bool app_task4_key1_start_requested(void);

/**
 * @brief 判断灰度快照是否连续满足指定黑线状态。
 * @param line_expected true 表示期望检测到黑线，false 表示期望离开黑线。
 * @return 连续确认达到阈值返回 true，否则返回 false。
 */
static bool app_task4_gray_is_confirmed(bool line_expected);

/**
 * @brief 使用左右轮速度 PID 设置目标速度。
 * @param left_target_cp 左轮目标速度，单位 counts/测速周期。
 * @param right_target_cp 右轮目标速度，单位 counts/测速周期。
 * @return 无。
 */
static void app_task4_set_speed_targets(int16_t left_target_cp,
    int16_t right_target_cp);

/**
 * @brief 通过速度 PID 保持左右轮零速度。
 * @param void 无参数。
 * @return 无。
 */
static void app_task4_hold_zero_speed(void);

/**
 * @brief 进入第四题低速航向转向状态。
 * @param state 目标低速航向转向状态。
 * @return 无。
 */
static void app_task4_enter_yaw_turn(app_task4_state_e state);

/**
 * @brief 刷新第四题低速航向转向控制。
 * @param void 无参数。
 * @return 无。
 */
static void app_task4_refresh_yaw_turn(void);

/**
 * @brief 进入第四题低速差速找线状态。
 * @param state 目标低速差速找线状态。
 * @return 无。
 */
static void app_task4_enter_line_search(app_task4_state_e state);

/**
 * @brief 刷新第四题低速差速找线控制。
 * @param turn_sign 差速方向，正值和负值对应相反转弯方向。
 * @param follow_state 检测到黑线后要进入的循迹状态。
 * @return 无。
 */
static void app_task4_refresh_line_search(int8_t turn_sign,
    app_task4_state_e follow_state);

/**
 * @brief 根据当前灰度状态进入第四题弧线循迹或低速找线状态。
 * @param follow_state 当前检测到黑线时要进入的循迹状态。
 * @param find_state 当前未检测到黑线时要进入的找线状态。
 * @return 无。
 */
static void app_task4_enter_arc_or_find(app_task4_state_e follow_state,
    app_task4_state_e find_state);

/**
 * @brief 进入第四题指定行驶状态。
 * @param state 目标第四题状态。
 * @return 无。
 */
static void app_task4_enter_state(app_task4_state_e state);

/* ======== 内部函数 ======== */

/**
 * @brief 将 Yaw 归一化到正负半圈范围。
 * @param yaw_mdeg 待归一化的 Yaw，单位 0.001 度。
 * @return 归一化后的 Yaw，范围为 [-180000, 180000)。
 */
static int32_t app_task4_normalize_yaw_mdeg(int32_t yaw_mdeg)
{
    while (yaw_mdeg >= DAL_MPU6050_YAW_HALF_TURN_MDEG) {
        yaw_mdeg -= DAL_MPU6050_YAW_FULL_TURN_MDEG;
    }

    while (yaw_mdeg < -DAL_MPU6050_YAW_HALF_TURN_MDEG) {
        yaw_mdeg += DAL_MPU6050_YAW_FULL_TURN_MDEG;
    }

    return yaw_mdeg;
}

/**
 * @brief 判断是否收到尚未消费的 Key1 按下事件。
 * @param void 无参数。
 * @return 收到新按下事件返回 true，否则返回 false。
 */
static bool app_task4_key1_start_requested(void)
{
    if (!g_dal_key_sample[DAL_KEY_KEY1].pressed_edge ||
        (g_dal_key_sample[DAL_KEY_KEY1].sequence ==
            g_app_task4_state.key1_handled_sequence)) {
        return false;
    }

    g_app_task4_state.key1_handled_sequence =
        g_dal_key_sample[DAL_KEY_KEY1].sequence;
    return true;
}

/**
 * @brief 判断灰度快照是否连续满足指定黑线状态。
 * @param line_expected true 表示期望检测到黑线，false 表示期望离开黑线。
 * @return 连续确认达到阈值返回 true，否则返回 false。
 */
static bool app_task4_gray_is_confirmed(bool line_expected)
{
    if (g_dal_gray_sample.sequence == 0U) {
        return false;
    }

    if (g_dal_gray_sample.sequence ==
        g_app_task4_state.gray_handled_sequence) {
        return (g_app_task4_state.gray_confirm_count >=
            APP_TASK4_GRAY_CONFIRM_SAMPLE_COUNT);
    }

    g_app_task4_state.gray_handled_sequence =
        g_dal_gray_sample.sequence;

    if (g_dal_gray_sample.line_detected == line_expected) {
        if (g_app_task4_state.gray_confirm_count <
            APP_TASK4_GRAY_CONFIRM_SAMPLE_COUNT) {
            g_app_task4_state.gray_confirm_count++;
        }
    } else {
        g_app_task4_state.gray_confirm_count = 0U;
    }

    return (g_app_task4_state.gray_confirm_count >=
        APP_TASK4_GRAY_CONFIRM_SAMPLE_COUNT);
}

/**
 * @brief 使用左右轮速度 PID 设置目标速度。
 * @param left_target_cp 左轮目标速度，单位 counts/测速周期。
 * @param right_target_cp 右轮目标速度，单位 counts/测速周期。
 * @return 无。
 */
static void app_task4_set_speed_targets(int16_t left_target_cp,
    int16_t right_target_cp)
{
    float left_pid_output;
    float right_pid_output;

    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_target_cp);
    left_pid_output = PID_Compute1_Rectangle(
        &pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)g_dal_encoder_sample[DAL_ENCODER_M1].speed_cp,
        bsp_time_get_ms());
    (void)dal_motor_set_output(DAL_MOTOR_LEFT, (int16_t)left_pid_output);

    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_target_cp);
    right_pid_output = PID_Compute1_Rectangle(
        &pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)g_dal_encoder_sample[DAL_ENCODER_M2].speed_cp,
        bsp_time_get_ms());
    (void)dal_motor_set_output(DAL_MOTOR_RIGHT, (int16_t)right_pid_output);
}

/**
 * @brief 通过速度 PID 保持左右轮零速度。
 * @param void 无参数。
 * @return 无。
 */
static void app_task4_hold_zero_speed(void)
{
    app_task4_set_speed_targets(0, 0);
}

/**
 * @brief 进入第四题低速航向转向状态。
 * @param state 目标低速航向转向状态。
 * @return 无。
 */
static void app_task4_enter_yaw_turn(app_task4_state_e state)
{
    int32_t delta_mdeg;

    if ((g_dal_mpu6050_sample.sequence == 0U) ||
        !g_dal_mpu6050_sample.yaw_calibrated) {
        g_app_task4_state.current_state = APP_TASK4_STATE_FINISHED;
        return;
    }

    delta_mdeg = APP_TASK4_TURN_A_TO_C_MDEG;
    g_app_task4_state.turn_next_state = APP_TASK4_STATE_STRAIGHT_TO_C;
    if (state == APP_TASK4_STATE_TURN_TO_D) {
        delta_mdeg = APP_TASK4_TURN_B_TO_D_MDEG;
        g_app_task4_state.turn_next_state = APP_TASK4_STATE_STRAIGHT_TO_D;
    }

    g_app_task4_state.current_state = state;
    g_app_task4_state.target_yaw_mdeg = app_task4_normalize_yaw_mdeg(
        g_dal_mpu6050_sample.yaw_mdeg + delta_mdeg);
    g_app_task4_state.gray_confirm_count = 0U;
}

/**
 * @brief 刷新第四题低速航向转向控制。
 * @param void 无参数。
 * @return 无。
 */
static void app_task4_refresh_yaw_turn(void)
{
    int32_t yaw_error_mdeg;
    int16_t turn_delta_cp = APP_TASK4_TRANSITION_TURN_DELTA_CP;

    if ((g_dal_mpu6050_sample.sequence == 0U) ||
        !g_dal_mpu6050_sample.yaw_calibrated) {
        g_app_task4_state.current_state = APP_TASK4_STATE_FINISHED;
        return;
    }

    yaw_error_mdeg = app_task4_normalize_yaw_mdeg(
        g_app_task4_state.target_yaw_mdeg -
        g_dal_mpu6050_sample.yaw_mdeg);
    if ((yaw_error_mdeg <= APP_TASK4_TURN_TOLERANCE_MDEG) &&
        (yaw_error_mdeg >= -APP_TASK4_TURN_TOLERANCE_MDEG)) {
        app_task4_enter_state(g_app_task4_state.turn_next_state);
        return;
    }

    if (yaw_error_mdeg < 0L) {
        turn_delta_cp = -turn_delta_cp;
    }
    turn_delta_cp = (int16_t)(turn_delta_cp *
        APP_TASK4_POSITIVE_YAW_TURN_SIGN);
    app_task4_set_speed_targets(
        (int16_t)(APP_TASK4_TRANSITION_BASE_SPEED_CP - turn_delta_cp),
        (int16_t)(APP_TASK4_TRANSITION_BASE_SPEED_CP + turn_delta_cp));
}

/**
 * @brief 进入第四题低速差速找线状态。
 * @param state 目标低速差速找线状态。
 * @return 无。
 */
static void app_task4_enter_line_search(app_task4_state_e state)
{
    g_app_task4_state.current_state = state;
    g_app_task4_state.line_search_start_ms = bsp_time_get_ms();
    g_app_task4_state.gray_confirm_count = 0U;
}

/**
 * @brief 刷新第四题低速差速找线控制。
 * @param turn_sign 差速方向，正值和负值对应相反转弯方向。
 * @param follow_state 检测到黑线后要进入的循迹状态。
 * @return 无。
 */
static void app_task4_refresh_line_search(int8_t turn_sign,
    app_task4_state_e follow_state)
{
    int16_t turn_delta_cp = APP_TASK4_TRANSITION_TURN_DELTA_CP;

    if ((uint32_t)(bsp_time_get_ms() -
        g_app_task4_state.line_search_start_ms) >=
        APP_TASK4_FIND_LINE_TIMEOUT_MS) {
        g_app_task4_state.current_state = APP_TASK4_STATE_FINISHED;
        return;
    }

    if (app_task4_gray_is_confirmed(true)) {
        app_task4_enter_state(follow_state);
        return;
    }

    if (turn_sign < 0) {
        turn_delta_cp = -turn_delta_cp;
    }
    app_task4_set_speed_targets(
        (int16_t)(APP_TASK4_TRANSITION_BASE_SPEED_CP - turn_delta_cp),
        (int16_t)(APP_TASK4_TRANSITION_BASE_SPEED_CP + turn_delta_cp));
}

/**
 * @brief 根据当前灰度状态进入第四题弧线循迹或低速找线状态。
 * @param follow_state 当前检测到黑线时要进入的循迹状态。
 * @param find_state 当前未检测到黑线时要进入的找线状态。
 * @return 无。
 */
static void app_task4_enter_arc_or_find(app_task4_state_e follow_state,
    app_task4_state_e find_state)
{
    if (g_dal_gray_sample.line_detected) {
        app_task4_enter_state(follow_state);
    } else {
        app_task4_enter_line_search(find_state);
    }
}

/**
 * @brief 进入第四题指定行驶状态。
 * @param state 目标第四题状态。
 * @return 无。
 */
static void app_task4_enter_state(app_task4_state_e state)
{
    g_app_task4_state.current_state = state;
    g_app_task4_state.gray_confirm_count = 0U;
    g_app_task4_state.gray_handled_sequence = g_dal_gray_sample.sequence;

    if ((state == APP_TASK4_STATE_STRAIGHT_TO_C) ||
        (state == APP_TASK4_STATE_STRAIGHT_TO_D)) {
        app_straight_drive_enter();
    } else if ((state == APP_TASK4_STATE_FOLLOW_TO_B) ||
        (state == APP_TASK4_STATE_FOLLOW_TO_A)) {
        g_app_task4_state.arc_capture_start_ms = bsp_time_get_ms();
        g_app_task4_state.arc_exit_armed = false;
    }
}

/* ======== 公开 API ======== */

void app_task4_init(void)
{
    app_line_follow_init();
    app_straight_drive_init();

    g_app_task4_state.current_state = APP_TASK4_STATE_WAIT_KEY1;
    g_app_task4_state.turn_next_state = APP_TASK4_STATE_FINISHED;
    g_app_task4_state.target_yaw_mdeg = 0L;
    g_app_task4_state.key1_handled_sequence =
        g_dal_key_sample[DAL_KEY_KEY1].sequence;
    g_app_task4_state.gray_handled_sequence = g_dal_gray_sample.sequence;
    g_app_task4_state.gray_confirm_count = 0U;
    g_app_task4_state.completed_laps = 0U;
    g_app_task4_state.line_search_start_ms = 0U;
    g_app_task4_state.arc_capture_start_ms = 0U;
    g_app_task4_state.arc_exit_armed = false;
    app_task4_hold_zero_speed();
}

void app_task4_refresh(void)
{
    switch (g_app_task4_state.current_state) {
    case APP_TASK4_STATE_WAIT_KEY1:
        app_task4_hold_zero_speed();
        if (app_task4_key1_start_requested()) {
            app_task4_enter_yaw_turn(APP_TASK4_STATE_TURN_TO_C);
        }
        break;

    case APP_TASK4_STATE_TURN_TO_C:
    case APP_TASK4_STATE_TURN_TO_D:
        app_task4_refresh_yaw_turn();
        break;

    case APP_TASK4_STATE_STRAIGHT_TO_C:
        app_straight_drive_refresh();
        if (app_task4_gray_is_confirmed(true)) {
            app_task4_enter_arc_or_find(APP_TASK4_STATE_FOLLOW_TO_B,
                APP_TASK4_STATE_FIND_LINE_TO_B);
        }
        break;

    case APP_TASK4_STATE_FIND_LINE_TO_B:
        app_task4_refresh_line_search(APP_TASK4_FIND_LINE_TO_B_TURN_SIGN,
            APP_TASK4_STATE_FOLLOW_TO_B);
        break;

    case APP_TASK4_STATE_FOLLOW_TO_B:
        app_line_follow_refresh();
        if (app_task4_gray_is_confirmed(false)) {
            if (g_app_task4_state.arc_exit_armed) {
                app_task4_enter_yaw_turn(APP_TASK4_STATE_TURN_TO_D);
            } else {
                app_task4_enter_line_search(APP_TASK4_STATE_FIND_LINE_TO_B);
            }
        } else if ((uint32_t)(bsp_time_get_ms() -
            g_app_task4_state.arc_capture_start_ms) >=
            APP_TASK4_ARC_CAPTURE_DURATION_MS) {
            g_app_task4_state.arc_exit_armed = true;
        }
        break;

    case APP_TASK4_STATE_STRAIGHT_TO_D:
        app_straight_drive_refresh();
        if (app_task4_gray_is_confirmed(true)) {
            app_task4_enter_arc_or_find(APP_TASK4_STATE_FOLLOW_TO_A,
                APP_TASK4_STATE_FIND_LINE_TO_A);
        }
        break;

    case APP_TASK4_STATE_FIND_LINE_TO_A:
        app_task4_refresh_line_search(APP_TASK4_FIND_LINE_TO_A_TURN_SIGN,
            APP_TASK4_STATE_FOLLOW_TO_A);
        break;

    case APP_TASK4_STATE_FOLLOW_TO_A:
        app_line_follow_refresh();
        if (app_task4_gray_is_confirmed(false)) {
            if (g_app_task4_state.arc_exit_armed) {
                g_app_task4_state.completed_laps++;
                if (g_app_task4_state.completed_laps >= APP_TASK4_LAP_COUNT) {
                    g_app_task4_state.current_state = APP_TASK4_STATE_FINISHED;
                } else {
                    app_task4_enter_yaw_turn(APP_TASK4_STATE_TURN_TO_C);
                }
            } else {
                app_task4_enter_line_search(APP_TASK4_STATE_FIND_LINE_TO_A);
            }
        } else if ((uint32_t)(bsp_time_get_ms() -
            g_app_task4_state.arc_capture_start_ms) >=
            APP_TASK4_ARC_CAPTURE_DURATION_MS) {
            g_app_task4_state.arc_exit_armed = true;
        }
        break;

    case APP_TASK4_STATE_FINISHED:
    default:
        app_task4_hold_zero_speed();
        break;
    }
}
