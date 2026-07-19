/**
 * @file app.c
 * @brief 循迹赛题行驶 APP 实现。
 */

/* ======== include ======== */

#include "app.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_line_follow.h"
#include "app_straight_drive.h"
#include "app_task4.h"
#include "bsp_time.h"
#include "dal_encoder.h"
#include "dal_gray.h"
#include "dal_key.h"
#include "dal_motor.h"
#include "dal_jy901p.h"

/* ======== 类型定义 ======== */

/**
 * @brief 第二题赛道行驶状态。
 */
typedef enum {
    APP_TASK2_STATE_WAIT_KEY1 = 0,       /**< 等待 Key1 启动。 */
    APP_TASK2_STATE_WAIT_MOTION_READY,   /**< 等待直行传感器数据就绪。 */
    APP_TASK2_STATE_STRAIGHT_TO_B,       /**< 白底直行至 B 点。 */
    APP_TASK2_STATE_FOLLOW_TO_C,         /**< 沿右侧半圆循迹至 C 点。 */
    APP_TASK2_STATE_STRAIGHT_TO_D,       /**< 白底直行至 D 点。 */
    APP_TASK2_STATE_FOLLOW_TO_A,         /**< 沿左侧半圆循迹至 A 点。 */
    APP_TASK2_STATE_NODE_PAUSE,          /**< 节点停车提示。 */
    APP_TASK2_STATE_FINISHED             /**< 已完成一圈并保持停车。 */
} app_task2_state_e;

/**
 * @brief 第三题赛道行驶状态。
 */
typedef enum {
    APP_TASK3_STATE_WAIT_KEY1 = 0,       /**< 等待 Key1 启动。 */
    APP_TASK3_STATE_TURN_TO_C,           /**< 原地转向至 A→C 航向。 */
    APP_TASK3_STATE_STRAIGHT_TO_C,       /**< 白底直行至 C 点。 */
    APP_TASK3_STATE_NODE_PAUSE,          /**< C、B、D 节点停车提示。 */
    APP_TASK3_STATE_FIND_LINE_TO_B,      /**< 原地寻找 C→B 右侧半圆。 */
    APP_TASK3_STATE_FOLLOW_TO_B,         /**< 沿右侧半圆循迹至 B 点。 */
    APP_TASK3_STATE_TURN_TO_D,           /**< 原地转向至 B→D 航向。 */
    APP_TASK3_STATE_STRAIGHT_TO_D,       /**< 白底直行至 D 点。 */
    APP_TASK3_STATE_FIND_LINE_TO_A,      /**< 原地寻找 D→A 左侧半圆。 */
    APP_TASK3_STATE_FOLLOW_TO_A,         /**< 沿左侧半圆循迹至 A 点。 */
    APP_TASK3_STATE_FINISHED             /**< 已完成一圈并保持停车。 */
} app_task3_state_e;

/**
 * @brief 赛题共用运行数据。
 */
typedef struct {
    uint32_t key1_handled_sequence;      /**< 已消费的 Key1 快照序号。 */
    uint32_t gray_handled_sequence;      /**< 已消费的灰度快照序号。 */
    uint8_t gray_confirm_count;          /**< 当前灰度状态连续确认次数。 */
} app_common_state_t;

/**
 * @brief 第二题状态机运行数据。
 */
typedef struct {
    app_task2_state_e current_state;     /**< 当前第二题赛道状态。 */
    app_task2_state_e pause_next_state;  /**< 节点暂停结束后的目标状态。 */
    uint32_t pause_start_ms;             /**< 节点暂停开始时刻，单位 ms。 */
    bool node_detect_armed;              /**< 已离开上一个节点，可检测下一个节点。 */
} app_task2_state_data_t;

/**
 * @brief 第三题状态机运行数据。
 */
typedef struct {
    app_task3_state_e current_state;     /**< 当前第三题赛道状态。 */
    app_task3_state_e pause_next_state;  /**< 节点暂停结束后的目标状态。 */
    app_task3_state_e turn_next_state;   /**< 原地转向完成后的目标状态。 */
    uint32_t pause_start_ms;             /**< 节点暂停开始时刻，单位 ms。 */
    uint32_t line_search_start_ms;       /**< 寻找弧线开始时刻，单位 ms。 */
    int32_t target_yaw_raw;             /**< 原地转向目标 Yaw，单位原始角度 LSB。 */
    bool line_search_armed;              /**< 已离开端点黑线，可接受重新检测到的弧线。 */
} app_task3_state_data_t;

/* ======== 内部变量 ======== */

static app_common_state_t g_app_common_state;

app_task2_state_data_t g_app_task2_state;

app_task3_state_data_t g_app_task3_state;

static bool g_app_straight_test_running;

/* ======== 内部函数声明 ======== */

/**
 * @brief 停止两个复用行驶 APP 的电机输出并复位控制状态。
 * @param void 无参数。
 * @return 无。
 */
static void app_stop_motion(void);

/**
 * @brief 使用左右轮速度 PID 将小车保持在零速度。
 * @param void 无参数。
 * @return 无。
 */
static void app_hold_motion_zero(void);

/**
 * @brief 判断灰度快照是否连续满足期望黑线状态。
 * @param line_expected true 表示期望检测到黑线，false 表示期望离开黑线。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_is_confirmed(bool line_expected);

/**
 * @brief 判断当前灰度条件是否连续成立。
 * @param condition 当前灰度条件。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_condition_is_confirmed(bool condition);

/**
 * @brief 判断当前灰度条件是否按指定次数连续成立。
 * @param condition 当前灰度条件。
 * @param confirm_count 需要连续确认的样本数，单位 次。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_condition_is_confirmed_by_count(
    bool condition,
    uint8_t confirm_count);

/**
 * @brief 判断第二题直行段是否已检测到有效节点线。
 * @param void 无参数。
 * @return 中间灰度通道连续检测到黑线时返回 true，否则返回 false。
 */
static bool app_task2_straight_node_is_confirmed(void);

/**
 * @brief 判断第二题半圆循迹段是否已到达出口节点。
 * @param void 无参数。
 * @return 灰度检测到出口宽线时返回 true，否则返回 false。
 */
static bool app_task2_follow_exit_is_confirmed(void);

/**
 * @brief 判断是否检测到尚未消费的 Key1 按下事件。
 * @param void 无参数。
 * @return 检测到新的按下事件返回 true，否则返回 false。
 */
static bool app_key1_start_requested(void);

/**
 * @brief 判断第二题直行所需的姿态和测速数据是否已经就绪。
 * @param void 无参数。
 * @return 角度参考已就绪且左右编码器测速有效时返回 true，否则返回 false。
 */
static bool app_task2_motion_ready(void);

/**
 * @brief 将 Yaw 角度归一化到半圈范围。
 * @param yaw_raw 待归一化的 Yaw，单位原始角度 LSB。
 * @return 归一化后的 Yaw，范围为 [-32768, 32768)。
 */
static int32_t app_normalize_yaw_raw(int32_t yaw_raw);

/**
 * @brief 进入第二题非暂停行驶状态。
 * @param state 目标第二题状态。
 * @return 无。
 */
static void app_task2_enter_motion_state(app_task2_state_e state);

/**
 * @brief 进入第二题启动前的传感器就绪等待状态。
 * @param void 无参数。
 * @return 无。
 */
static void app_task2_enter_motion_ready_wait(void);

/**
 * @brief 进入第二题节点停车提示状态。
 * @param next_state 暂停结束后要进入的第二题状态。
 * @return 无。
 */
static void app_task2_enter_node_pause(app_task2_state_e next_state);

/**
 * @brief 刷新第二题赛道状态机。
 * @param void 无参数。
 * @return 无。
 */
static void app_task2_refresh(void);

/**
 * @brief 获取第三题指定原地转向状态的相对目标角。
 * @param state 第三题原地转向状态。
 * @return 相对转角，单位原始角度 LSB。
 */
static int32_t app_task3_get_turn_delta_raw(app_task3_state_e state);

/**
 * @brief 获取第三题指定原地转向状态完成后的目标状态。
 * @param state 第三题原地转向状态。
 * @return 原地转向完成后要进入的第三题状态。
 */
static app_task3_state_e app_task3_get_turn_next_state(
    app_task3_state_e state);

/**
 * @brief 进入第三题运行状态。
 * @param state 目标第三题状态。
 * @return 无。
 */
static void app_task3_enter_state(app_task3_state_e state);

/**
 * @brief 进入第三题节点停车提示状态。
 * @param next_state 暂停结束后要进入的第三题状态。
 * @return 无。
 */
static void app_task3_enter_node_pause(app_task3_state_e next_state);

/**
 * @brief 刷新第三题原地转向闭环。
 * @param void 无参数。
 * @return 无。
 */
static void app_task3_refresh_turn(void);

/**
 * @brief 按指定 Yaw 方向输出原地转向电机命令。
 * @param yaw_direction 目标 Yaw 方向，正值表示正 Yaw，负值表示负 Yaw。
 * @return 无。
 */
static void app_task3_apply_turn_output(int8_t yaw_direction);

/**
 * @brief 刷新第三题端点离线后重新寻找弧线的转向状态。
 * @param yaw_direction 寻找弧线时的目标 Yaw 方向，正值表示正 Yaw，负值表示负 Yaw。
 * @param follow_state 重新检测到弧线后要进入的循迹状态。
 * @return 无。
 */
static void app_task3_refresh_line_search(int8_t yaw_direction,
    app_task3_state_e follow_state);

/**
 * @brief 刷新第三题赛道状态机。
 * @param void 无参数。
 * @return 无。
 */
static void app_task3_refresh(void);

/**
 * @brief 刷新直行测试逻辑。
 * @param void 无参数。
 * @return 无。
 */
static void app_straight_test_refresh(void);

/* ======== 内部函数 ======== */

/**
 * @brief 停止两个复用行驶 APP 的电机输出并复位控制状态。
 * @param void 无参数。
 * @return 无。
 */
static void app_stop_motion(void)
{
    app_straight_drive_stop();
    app_line_follow_stop();
}

/**
 * @brief 使用左右轮速度 PID 将小车保持在零速度。
 * @param void 无参数。
 * @return 无。
 */
static void app_hold_motion_zero(void)
{
    app_line_follow_hold_stop();
}

/**
 * @brief 判断灰度快照是否连续满足期望黑线状态。
 * @param line_expected true 表示期望检测到黑线，false 表示期望离开黑线。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_is_confirmed(bool line_expected)
{
    return app_gray_condition_is_confirmed(
        g_dal_gray_sample.line_detected == line_expected);
}

/**
 * @brief 判断当前灰度条件是否连续成立。
 * @param condition 当前灰度条件。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_condition_is_confirmed(bool condition)
{
    return app_gray_condition_is_confirmed_by_count(
        condition,
        APP_GRAY_CONFIRM_SAMPLE_COUNT);
}

/**
 * @brief 判断当前灰度条件是否按指定次数连续成立。
 * @param condition 当前灰度条件。
 * @param confirm_count 需要连续确认的样本数，单位 次。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_condition_is_confirmed_by_count(
    bool condition,
    uint8_t confirm_count)
{
    if (g_dal_gray_sample.sequence == 0U) {
        return false;
    }

    if (g_dal_gray_sample.sequence ==
        g_app_common_state.gray_handled_sequence) {
        return (g_app_common_state.gray_confirm_count >=
            confirm_count);
    }

    g_app_common_state.gray_handled_sequence = g_dal_gray_sample.sequence;

    if (condition) {
        if (g_app_common_state.gray_confirm_count <
            confirm_count) {
            g_app_common_state.gray_confirm_count++;
        }
    } else {
        g_app_common_state.gray_confirm_count = 0U;
    }

    return (g_app_common_state.gray_confirm_count >=
        confirm_count);
}

/**
 * @brief 判断是否检测到尚未消费的 Key1 按下事件。
 * @param void 无参数。
 * @return 检测到新的按下事件返回 true，否则返回 false。
 */
static bool app_key1_start_requested(void)
{
    if (!g_dal_key_sample[DAL_KEY_KEY1].pressed_edge ||
        (g_dal_key_sample[DAL_KEY_KEY1].sequence ==
            g_app_common_state.key1_handled_sequence)) {
        return false;
    }

    g_app_common_state.key1_handled_sequence =
        g_dal_key_sample[DAL_KEY_KEY1].sequence;
    return true;
}

/**
 * @brief 判断第二题直行所需的姿态和测速数据是否已经就绪。
 * @param void 无参数。
 * @return 角度参考已就绪且左右编码器测速有效时返回 true，否则返回 false。
 */
static bool app_task2_motion_ready(void)
{
    return ((g_dal_jy901p_sample.sequence != 0U) &&
        g_dal_jy901p_sample.angle_reference_ready &&
        g_dal_encoder_sample[DAL_ENCODER_M1].speed_valid &&
        g_dal_encoder_sample[DAL_ENCODER_M2].speed_valid);
}

/**
 * @brief 判断第二题直行段是否已检测到有效节点线。
 * @param void 无参数。
 * @return 中间灰度通道连续检测到黑线时返回 true，否则返回 false。
 */
static bool app_task2_straight_node_is_confirmed(void)
{
    if (!g_app_task2_state.node_detect_armed) {
        if (app_gray_condition_is_confirmed(!g_dal_gray_sample.line_detected)) {
            g_app_task2_state.node_detect_armed = true;
            g_app_common_state.gray_confirm_count = 0U;
        }

        return false;
    }

    return app_gray_condition_is_confirmed_by_count(
        g_dal_gray_sample.line_detected,
        APP_TASK2_STRAIGHT_NODE_CONFIRM_COUNT);
}

/**
 * @brief 判断第二题半圆循迹段是否已到达出口节点。
 * @param void 无参数。
 * @return 循迹后检测到离开黑线时返回 true，否则返回 false。
 */
static bool app_task2_follow_exit_is_confirmed(void)
{
    if (!g_app_task2_state.node_detect_armed) {
        if (app_gray_condition_is_confirmed(g_dal_gray_sample.line_detected)) {
            g_app_task2_state.node_detect_armed = true;
            g_app_common_state.gray_confirm_count = 0U;
        }

        return false;
    }

    return app_gray_condition_is_confirmed(!g_dal_gray_sample.line_detected);
}

/**
 * @brief 将 Yaw 角度归一化到半圈范围。
 * @param yaw_raw 待归一化的 Yaw，单位原始角度 LSB。
 * @return 归一化后的 Yaw，范围为 [-32768, 32768)。
 */
static int32_t app_normalize_yaw_raw(int32_t yaw_raw)
{
    while (yaw_raw >= DAL_JY901P_YAW_HALF_TURN_RAW) {
        yaw_raw -= DAL_JY901P_YAW_FULL_TURN_RAW;
    }

    while (yaw_raw < -DAL_JY901P_YAW_HALF_TURN_RAW) {
        yaw_raw += DAL_JY901P_YAW_FULL_TURN_RAW;
    }

    return yaw_raw;
}

/**
 * @brief 进入第二题非暂停行驶状态。
 * @param state 目标第二题状态。
 * @return 无。
 */
static void app_task2_enter_motion_state(app_task2_state_e state)
{
    app_stop_motion();
    g_app_task2_state.current_state = state;
    g_app_common_state.gray_confirm_count = 0U;
    g_app_common_state.gray_handled_sequence = g_dal_gray_sample.sequence;
    g_app_task2_state.node_detect_armed = false;

    if (state == APP_TASK2_STATE_STRAIGHT_TO_D) {
        app_straight_drive_enter_with_offset(APP_TASK2_C_TO_D_YAW_OFFSET_RAW);
    } else if (state == APP_TASK2_STATE_STRAIGHT_TO_B) {
        app_straight_drive_enter();
    }
}

/**
 * @brief 进入第二题启动前的传感器就绪等待状态。
 * @param void 无参数。
 * @return 无。
 */
static void app_task2_enter_motion_ready_wait(void)
{
    app_stop_motion();
    g_app_task2_state.current_state = APP_TASK2_STATE_WAIT_MOTION_READY;
    g_app_common_state.gray_confirm_count = 0U;
    g_app_common_state.gray_handled_sequence = g_dal_gray_sample.sequence;
}

/**
 * @brief 进入第二题节点停车提示状态。
 * @param next_state 暂停结束后要进入的第二题状态。
 * @return 无。
 */
static void app_task2_enter_node_pause(app_task2_state_e next_state)
{
    app_hold_motion_zero();
    g_app_task2_state.current_state = APP_TASK2_STATE_NODE_PAUSE;
    g_app_task2_state.pause_next_state = next_state;
    g_app_task2_state.pause_start_ms = bsp_time_get_ms();
    g_app_common_state.gray_confirm_count = 0U;
    g_app_common_state.gray_handled_sequence = g_dal_gray_sample.sequence;
}

/**
 * @brief 刷新第二题赛道状态机。
 * @param void 无参数。
 * @return 无。
 */
static void app_task2_refresh(void)
{
    switch (g_app_task2_state.current_state) {
    case APP_TASK2_STATE_WAIT_KEY1:
        app_hold_motion_zero();
        if (app_key1_start_requested()) {
            app_task2_enter_motion_ready_wait();
        }
        break;

    case APP_TASK2_STATE_WAIT_MOTION_READY:
        app_hold_motion_zero();
        if (app_task2_motion_ready()) {
            app_task2_enter_motion_state(APP_TASK2_STATE_STRAIGHT_TO_B);
        }
        break;

    case APP_TASK2_STATE_STRAIGHT_TO_B:
        app_straight_drive_refresh();
        if (app_task2_straight_node_is_confirmed()) {
            app_task2_enter_node_pause(APP_TASK2_STATE_FOLLOW_TO_C);
        }
        break;

    case APP_TASK2_STATE_FOLLOW_TO_C:
        app_line_follow_refresh();
        if (app_task2_follow_exit_is_confirmed()) {
            app_task2_enter_node_pause(APP_TASK2_STATE_STRAIGHT_TO_D);
        }
        break;

    case APP_TASK2_STATE_STRAIGHT_TO_D:
        app_straight_drive_refresh();
        if (app_task2_straight_node_is_confirmed()) {
            app_task2_enter_node_pause(APP_TASK2_STATE_FOLLOW_TO_A);
        }
        break;

    case APP_TASK2_STATE_FOLLOW_TO_A:
        app_line_follow_refresh();
        if (app_task2_follow_exit_is_confirmed()) {
            app_task2_enter_node_pause(APP_TASK2_STATE_FINISHED);
        }
        break;

    case APP_TASK2_STATE_NODE_PAUSE:
        app_hold_motion_zero();
        if ((uint32_t)(bsp_time_get_ms() -
            g_app_task2_state.pause_start_ms) >= APP_NODE_PAUSE_DURATION_MS) {
            app_task2_enter_motion_state(g_app_task2_state.pause_next_state);
        }
        break;

    case APP_TASK2_STATE_FINISHED:
    default:
        app_hold_motion_zero();
        break;
    }
}

/**
 * @brief 获取第三题指定原地转向状态的相对目标角。
 * @param state 第三题原地转向状态。
 * @return 相对转角，单位原始角度 LSB。
 */
static int32_t app_task3_get_turn_delta_raw(app_task3_state_e state)
{
    switch (state) {
    case APP_TASK3_STATE_TURN_TO_C:
        return APP_TASK3_TURN_A_TO_C_RAW;
    case APP_TASK3_STATE_TURN_TO_D:
        return APP_TASK3_TURN_B_TO_D_RAW;
    default:
        return 0L;
    }
}

/**
 * @brief 获取第三题指定原地转向状态完成后的目标状态。
 * @param state 第三题原地转向状态。
 * @return 原地转向完成后要进入的第三题状态。
 */
static app_task3_state_e app_task3_get_turn_next_state(
    app_task3_state_e state)
{
    switch (state) {
    case APP_TASK3_STATE_TURN_TO_C:
        return APP_TASK3_STATE_STRAIGHT_TO_C;
    case APP_TASK3_STATE_TURN_TO_D:
        return APP_TASK3_STATE_STRAIGHT_TO_D;
    default:
        return APP_TASK3_STATE_FINISHED;
    }
}

/**
 * @brief 进入第三题运行状态。
 * @param state 目标第三题状态。
 * @return 无。
 */
static void app_task3_enter_state(app_task3_state_e state)
{
    app_stop_motion();
    g_app_task3_state.current_state = state;
    g_app_common_state.gray_confirm_count = 0U;
    g_app_common_state.gray_handled_sequence = g_dal_gray_sample.sequence;

    if ((state == APP_TASK3_STATE_STRAIGHT_TO_C) ||
        (state == APP_TASK3_STATE_STRAIGHT_TO_D)) {
        app_straight_drive_enter();
    } else if ((state == APP_TASK3_STATE_TURN_TO_C) ||
        (state == APP_TASK3_STATE_TURN_TO_D)) {
        if ((g_dal_jy901p_sample.sequence == 0U) ||
            !g_dal_jy901p_sample.angle_reference_ready) {
            g_app_task3_state.current_state = APP_TASK3_STATE_FINISHED;
            return;
        }

        g_app_task3_state.target_yaw_raw = app_normalize_yaw_raw(
            g_dal_jy901p_sample.yaw_raw +
            app_task3_get_turn_delta_raw(state));
        g_app_task3_state.turn_next_state =
            app_task3_get_turn_next_state(state);
    } else if ((state == APP_TASK3_STATE_FIND_LINE_TO_B) ||
        (state == APP_TASK3_STATE_FIND_LINE_TO_A)) {
        g_app_task3_state.line_search_start_ms = bsp_time_get_ms();
        g_app_task3_state.line_search_armed = false;
    }
}

/**
 * @brief 进入第三题节点停车提示状态。
 * @param next_state 暂停结束后要进入的第三题状态。
 * @return 无。
 */
static void app_task3_enter_node_pause(app_task3_state_e next_state)
{
    app_hold_motion_zero();
    g_app_task3_state.current_state = APP_TASK3_STATE_NODE_PAUSE;
    g_app_task3_state.pause_next_state = next_state;
    g_app_task3_state.pause_start_ms = bsp_time_get_ms();
    g_app_common_state.gray_confirm_count = 0U;
    g_app_common_state.gray_handled_sequence = g_dal_gray_sample.sequence;
}

/**
 * @brief 刷新第三题原地转向闭环。
 * @param void 无参数。
 * @return 无。
 */
static void app_task3_refresh_turn(void)
{
    int32_t yaw_error_raw;
    int16_t left_output_permille;

    if ((g_dal_jy901p_sample.sequence == 0U) ||
        !g_dal_jy901p_sample.angle_reference_ready) {
        app_stop_motion();
        g_app_task3_state.current_state = APP_TASK3_STATE_FINISHED;
        return;
    }

    yaw_error_raw = app_normalize_yaw_raw(
        g_app_task3_state.target_yaw_raw -
        g_dal_jy901p_sample.yaw_raw);

    if ((yaw_error_raw <= APP_TASK3_TURN_TOLERANCE_RAW) &&
        (yaw_error_raw >= -APP_TASK3_TURN_TOLERANCE_RAW)) {
        app_stop_motion();
        app_task3_enter_state(g_app_task3_state.turn_next_state);
        return;
    }

    left_output_permille = APP_TASK3_TURN_OUTPUT_PERMILLE;
    if (yaw_error_raw < 0L) {
        left_output_permille = -left_output_permille;
    }

    app_task3_apply_turn_output((int8_t)(left_output_permille /
        APP_TASK3_TURN_OUTPUT_PERMILLE));
}

/**
 * @brief 按指定 Yaw 方向输出原地转向电机命令。
 * @param yaw_direction 目标 Yaw 方向，正值表示正 Yaw，负值表示负 Yaw。
 * @return 无。
 */
static void app_task3_apply_turn_output(int8_t yaw_direction)
{
    int16_t left_output_permille = APP_TASK3_TURN_OUTPUT_PERMILLE;

    if (yaw_direction < 0) {
        left_output_permille = -left_output_permille;
    }

    left_output_permille = (int16_t)(left_output_permille *
        APP_TASK3_POSITIVE_YAW_LEFT_OUTPUT_SIGN);
    (void)dal_motor_set_output(DAL_MOTOR_LEFT, left_output_permille);
    (void)dal_motor_set_output(DAL_MOTOR_RIGHT, -left_output_permille);
}

/**
 * @brief 刷新第三题端点离线后重新寻找弧线的转向状态。
 * @param yaw_direction 寻找弧线时的目标 Yaw 方向，正值表示正 Yaw，负值表示负 Yaw。
 * @param follow_state 重新检测到弧线后要进入的循迹状态。
 * @return 无。
 */
static void app_task3_refresh_line_search(int8_t yaw_direction,
    app_task3_state_e follow_state)
{
    if ((uint32_t)(bsp_time_get_ms() -
        g_app_task3_state.line_search_start_ms) >=
        APP_TASK3_FIND_LINE_TIMEOUT_MS) {
        app_stop_motion();
        g_app_task3_state.current_state = APP_TASK3_STATE_FINISHED;
        return;
    }

    if (!g_app_task3_state.line_search_armed) {
        if (app_gray_is_confirmed(false)) {
            g_app_task3_state.line_search_armed = true;
            g_app_common_state.gray_confirm_count = 0U;
        }
    } else if (app_gray_is_confirmed(true)) {
        app_stop_motion();
        app_task3_enter_state(follow_state);
        return;
    }

    app_task3_apply_turn_output(yaw_direction);
}

/**
 * @brief 刷新第三题赛道状态机。
 * @param void 无参数。
 * @return 无。
 */
static void app_task3_refresh(void)
{
    switch (g_app_task3_state.current_state) {
    case APP_TASK3_STATE_WAIT_KEY1:
        app_hold_motion_zero();
        if ((g_dal_jy901p_sample.sequence != 0U) &&
            g_dal_jy901p_sample.angle_reference_ready &&
            app_key1_start_requested()) {
            app_task3_enter_state(APP_TASK3_STATE_TURN_TO_C);
        }
        break;

    case APP_TASK3_STATE_TURN_TO_C:
    case APP_TASK3_STATE_TURN_TO_D:
        app_task3_refresh_turn();
        break;

    case APP_TASK3_STATE_STRAIGHT_TO_C:
        app_straight_drive_refresh();
        if (app_gray_is_confirmed(true)) {
            app_task3_enter_node_pause(APP_TASK3_STATE_FIND_LINE_TO_B);
        }
        break;

    case APP_TASK3_STATE_FIND_LINE_TO_B:
        app_task3_refresh_line_search(
            APP_TASK3_FIND_LINE_TO_B_YAW_DIRECTION,
            APP_TASK3_STATE_FOLLOW_TO_B);
        break;

    case APP_TASK3_STATE_FOLLOW_TO_B:
        app_line_follow_refresh();
        if (app_gray_is_confirmed(false)) {
            app_task3_enter_node_pause(APP_TASK3_STATE_TURN_TO_D);
        }
        break;

    case APP_TASK3_STATE_STRAIGHT_TO_D:
        app_straight_drive_refresh();
        if (app_gray_is_confirmed(true)) {
            app_task3_enter_node_pause(APP_TASK3_STATE_FIND_LINE_TO_A);
        }
        break;

    case APP_TASK3_STATE_FIND_LINE_TO_A:
        app_task3_refresh_line_search(
            APP_TASK3_FIND_LINE_TO_A_YAW_DIRECTION,
            APP_TASK3_STATE_FOLLOW_TO_A);
        break;

    case APP_TASK3_STATE_FOLLOW_TO_A:
        app_line_follow_refresh();
        if (app_gray_is_confirmed(false)) {
            app_hold_motion_zero();
            g_app_task3_state.current_state = APP_TASK3_STATE_FINISHED;
        }
        break;

    case APP_TASK3_STATE_NODE_PAUSE:
        app_hold_motion_zero();
        if ((uint32_t)(bsp_time_get_ms() -
            g_app_task3_state.pause_start_ms) >= APP_NODE_PAUSE_DURATION_MS) {
            if ((g_app_task3_state.pause_next_state ==
                APP_TASK3_STATE_FIND_LINE_TO_B) &&
                g_dal_gray_sample.line_detected) {
                app_task3_enter_state(APP_TASK3_STATE_FOLLOW_TO_B);
            } else if ((g_app_task3_state.pause_next_state ==
                APP_TASK3_STATE_FIND_LINE_TO_A) &&
                g_dal_gray_sample.line_detected) {
                app_task3_enter_state(APP_TASK3_STATE_FOLLOW_TO_A);
            } else {
                app_task3_enter_state(g_app_task3_state.pause_next_state);
            }
        }
        break;

    case APP_TASK3_STATE_FINISHED:
    default:
        app_hold_motion_zero();
        break;
    }
}

/**
 * @brief 刷新直行测试逻辑。
 * @param void 无参数。
 * @return 无。
 */
static void app_straight_test_refresh(void)
{
    if (!g_app_straight_test_running) {
        app_hold_motion_zero();
        if (app_key1_start_requested() && app_task2_motion_ready()) {
            g_app_straight_test_running = true;
            app_straight_drive_enter();
        }
        return;
    }

    app_straight_drive_refresh();
}

/* ======== 公开 API ======== */

void app_init(void)
{
    if (APP_ACTIVE_TASK == APP_TASK_4) {
        app_task4_init();
        return;
    }

    app_line_follow_init();
    app_straight_drive_init();

    g_app_common_state.key1_handled_sequence =
        g_dal_key_sample[DAL_KEY_KEY1].sequence;
    g_app_common_state.gray_handled_sequence = g_dal_gray_sample.sequence;
    g_app_common_state.gray_confirm_count = 0U;

    g_app_task2_state.current_state = APP_TASK2_STATE_WAIT_KEY1;
    g_app_task2_state.pause_next_state = APP_TASK2_STATE_WAIT_KEY1;
    g_app_task2_state.pause_start_ms = 0U;
    g_app_task2_state.node_detect_armed = false;

    g_app_task3_state.current_state = APP_TASK3_STATE_WAIT_KEY1;
    g_app_task3_state.pause_next_state = APP_TASK3_STATE_WAIT_KEY1;
    g_app_task3_state.turn_next_state = APP_TASK3_STATE_FINISHED;
    g_app_task3_state.pause_start_ms = 0U;
    g_app_task3_state.line_search_start_ms = 0U;
    g_app_task3_state.target_yaw_raw = 0L;
    g_app_task3_state.line_search_armed = false;
    g_app_straight_test_running = false;

    app_stop_motion();
}

void app_refresh(void)
{
    if (APP_ACTIVE_TASK == APP_TASK_2) {
        app_task2_refresh();
    } else if (APP_ACTIVE_TASK == APP_TASK_3) {
        app_task3_refresh();
    } else if (APP_ACTIVE_TASK == APP_TASK_STRAIGHT_TEST) {
        app_straight_test_refresh();
    } else {
        app_task4_refresh();
    }
}
