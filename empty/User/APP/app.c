/**
 * @file app.c
 * @brief 第二题赛道行驶 APP 实现。
 */

/* ======== include ======== */

#include "app.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_line_follow.h"
#include "app_straight_drive.h"
#include "bsp_time.h"
#include "dal_gray.h"
#include "dal_key.h"

/* ======== 类型定义 ======== */

/**
 * @brief 第二题赛道行驶状态。
 */
typedef enum {
    APP_STATE_WAIT_KEY1 = 0,       /**< 等待 Key1 启动。 */
    APP_STATE_STRAIGHT_TO_B,       /**< 白底直行至 B 点。 */
    APP_STATE_FOLLOW_TO_C,         /**< 沿右侧半圆循迹至 C 点。 */
    APP_STATE_STRAIGHT_TO_D,       /**< 白底直行至 D 点。 */
    APP_STATE_FOLLOW_TO_A,         /**< 沿左侧半圆循迹至 A 点。 */
    APP_STATE_NODE_PAUSE,          /**< 节点停车提示。 */
    APP_STATE_FINISHED             /**< 已完成一圈并保持停车。 */
} app_state_e;

/**
 * @brief 第二题赛道状态机运行数据。
 */
typedef struct {
    app_state_e current_state;            /**< 当前赛道状态。 */
    app_state_e pause_next_state;         /**< 节点暂停结束后的目标状态。 */
    uint32_t pause_start_ms;              /**< 节点暂停开始时刻，单位 ms。 */
    uint32_t key1_handled_sequence;       /**< 已消费的 Key1 快照序号。 */
    uint8_t gray_confirm_count;           /**< 当前灰度状态连续确认次数。 */
} app_state_data_t;

/* ======== 内部变量 ======== */

static app_state_data_t g_app_state_data;

/* ======== 内部函数声明 ======== */

/**
 * @brief 停止两个复用行驶 APP 的电机输出并复位控制状态。
 * @param void 无参数。
 * @return 无。
 */
static void app_stop_motion(void);

/**
 * @brief 进入一个非暂停赛道状态，并按需建立直行航向基准。
 * @param state 目标赛道状态。
 * @return 无。
 */
static void app_enter_motion_state(app_state_e state);

/**
 * @brief 进入节点停车提示状态。
 * @param next_state 暂停结束后要进入的赛道状态。
 * @return 无。
 */
static void app_enter_node_pause(app_state_e next_state);

/**
 * @brief 判断灰度快照是否连续满足期望黑线状态。
 * @param line_expected true 表示期望检测到黑线，false 表示期望离开黑线。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_is_confirmed(bool line_expected);

/**
 * @brief 判断是否检测到尚未消费的 Key1 按下事件。
 * @param void 无参数。
 * @return 检测到新的按下事件返回 true，否则返回 false。
 */
static bool app_key1_start_requested(void);

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
 * @brief 进入一个非暂停赛道状态，并按需建立直行航向基准。
 * @param state 目标赛道状态。
 * @return 无。
 */
static void app_enter_motion_state(app_state_e state)
{
    app_stop_motion();
    g_app_state_data.current_state = state;
    g_app_state_data.gray_confirm_count = 0U;

    if ((state == APP_STATE_STRAIGHT_TO_B) ||
        (state == APP_STATE_STRAIGHT_TO_D)) {
        app_straight_drive_enter();
    }
}

/**
 * @brief 进入节点停车提示状态。
 * @param next_state 暂停结束后要进入的赛道状态。
 * @return 无。
 */
static void app_enter_node_pause(app_state_e next_state)
{
    app_stop_motion();
    g_app_state_data.current_state = APP_STATE_NODE_PAUSE;
    g_app_state_data.pause_next_state = next_state;
    g_app_state_data.pause_start_ms = bsp_time_get_ms();
    g_app_state_data.gray_confirm_count = 0U;
}

/**
 * @brief 判断灰度快照是否连续满足期望黑线状态。
 * @param line_expected true 表示期望检测到黑线，false 表示期望离开黑线。
 * @return 连续确认次数达到阈值返回 true，否则返回 false。
 */
static bool app_gray_is_confirmed(bool line_expected)
{
    if (g_dal_gray_sample.sequence == 0U) {
        return false;
    }

    if (g_dal_gray_sample.line_detected == line_expected) {
        if (g_app_state_data.gray_confirm_count <
            APP_GRAY_CONFIRM_SAMPLE_COUNT) {
            g_app_state_data.gray_confirm_count++;
        }
    } else {
        g_app_state_data.gray_confirm_count = 0U;
    }

    return (g_app_state_data.gray_confirm_count >=
        APP_GRAY_CONFIRM_SAMPLE_COUNT);
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
            g_app_state_data.key1_handled_sequence)) {
        return false;
    }

    g_app_state_data.key1_handled_sequence =
        g_dal_key_sample[DAL_KEY_KEY1].sequence;
    return true;
}

/* ======== 公开 API ======== */

void app_init(void)
{
    app_line_follow_init();
    app_straight_drive_init();

    g_app_state_data.current_state = APP_STATE_WAIT_KEY1;
    g_app_state_data.pause_next_state = APP_STATE_WAIT_KEY1;
    g_app_state_data.pause_start_ms = 0U;
    g_app_state_data.key1_handled_sequence =
        g_dal_key_sample[DAL_KEY_KEY1].sequence;
    g_app_state_data.gray_confirm_count = 0U;
    app_stop_motion();
}

void app_refresh(void)
{
    switch (g_app_state_data.current_state) {
    case APP_STATE_WAIT_KEY1:
        if (app_key1_start_requested()) {
            app_enter_motion_state(APP_STATE_STRAIGHT_TO_B);
        }
        break;

    case APP_STATE_STRAIGHT_TO_B:
        app_straight_drive_refresh();
        if (app_gray_is_confirmed(true)) {
            app_enter_node_pause(APP_STATE_FOLLOW_TO_C);
        }
        break;

    case APP_STATE_FOLLOW_TO_C:
        app_line_follow_refresh();
        if (app_gray_is_confirmed(false)) {
            app_enter_node_pause(APP_STATE_STRAIGHT_TO_D);
        }
        break;

    case APP_STATE_STRAIGHT_TO_D:
        app_straight_drive_refresh();
        if (app_gray_is_confirmed(true)) {
            app_enter_node_pause(APP_STATE_FOLLOW_TO_A);
        }
        break;

    case APP_STATE_FOLLOW_TO_A:
        app_line_follow_refresh();
        if (app_gray_is_confirmed(false)) {
            app_enter_node_pause(APP_STATE_FINISHED);
        }
        break;

    case APP_STATE_NODE_PAUSE:
        if ((uint32_t)(bsp_time_get_ms() - g_app_state_data.pause_start_ms) >=
            APP_NODE_PAUSE_DURATION_MS) {
            app_enter_motion_state(g_app_state_data.pause_next_state);
        }
        break;

    case APP_STATE_FINISHED:
    default:
        app_stop_motion();
        break;
    }
}
