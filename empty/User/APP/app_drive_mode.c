/**
 * @file app_drive_mode.c
 * @brief 行车模式管理 APP 实现。
 */

/* ======== include ======== */

#include "app_drive_mode.h"

#include <stdint.h>

#include "app_line_follow.h"
#include "app_straight_drive.h"
#include "dal_gray.h"
#include "dal_key.h"
#include "dal_motor.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

typedef enum {
    APP_DRIVE_MODE_STOP = 0,       /**< 停止模式。 */
    APP_DRIVE_MODE_LINE_FOLLOW,    /**< 沿线循迹模式。 */
    APP_DRIVE_MODE_STRAIGHT        /**< Yaw 直线保持模式。 */
} app_drive_mode_e;

/* ======== 内部变量 ======== */

static app_drive_mode_e g_app_drive_mode_current;

static uint32_t g_app_drive_mode_key1_handled_sequence;

/* ======== 内部函数 ======== */

/**
 * @brief 停止所有行车子模式输出。
 * @param void 无参数。
 * @return 无。
 */
static void app_drive_mode_stop_outputs(void)
{
    app_straight_drive_stop();
    app_line_follow_stop();
    dal_motor_stop_all();
}

/**
 * @brief 切换到指定行车模式。
 * @param mode 目标行车模式。
 * @return 无。
 */
static void app_drive_mode_set(app_drive_mode_e mode)
{
    app_drive_mode_stop_outputs();
    g_app_drive_mode_current = mode;

    if (mode == APP_DRIVE_MODE_STRAIGHT) {
        app_straight_drive_enter();
    }
}

/**
 * @brief 根据 Key1 请求从停止模式进入直线找线模式。
 * @param void 无参数。
 * @return 无。
 */
static void app_drive_mode_handle_start_key(void)
{
    if (g_app_drive_mode_current == APP_DRIVE_MODE_STOP) {
        app_drive_mode_set(APP_DRIVE_MODE_STRAIGHT);
    }
}

/**
 * @brief 直线找线阶段检测到灰线后切入循迹模式。
 * @param void 无参数。
 * @return 无。
 */
static void app_drive_mode_update_line_transition(void)
{
    if ((g_app_drive_mode_current == APP_DRIVE_MODE_STRAIGHT) &&
        (g_dal_gray_sample.sequence != 0U) &&
        g_dal_gray_sample.line_detected) {
        app_drive_mode_set(APP_DRIVE_MODE_LINE_FOLLOW);
    }
}

/**
 * @brief 刷新当前行车模式执行体。
 * @param void 无参数。
 * @return 无。
 */
static void app_drive_mode_refresh_current(void)
{
    switch (g_app_drive_mode_current) {
    case APP_DRIVE_MODE_STRAIGHT:
        app_straight_drive_refresh();
        break;
    case APP_DRIVE_MODE_LINE_FOLLOW:
        app_line_follow_refresh();
        break;
    case APP_DRIVE_MODE_STOP:
    default:
        dal_motor_stop_all();
        break;
    }
}

/* ======== 公开 API ======== */

void app_drive_mode_init(void)
{
    app_straight_drive_init();
    app_line_follow_init();
    g_app_drive_mode_key1_handled_sequence = g_dal_key_sample[DAL_KEY_KEY1].sequence;
    app_drive_mode_set(APP_DRIVE_MODE_STOP);
}

void app_drive_mode_refresh(void)
{
    if (g_dal_key_sample[DAL_KEY_KEY1].pressed_edge &&
        (g_dal_key_sample[DAL_KEY_KEY1].sequence !=
            g_app_drive_mode_key1_handled_sequence)) {
        g_app_drive_mode_key1_handled_sequence =
            g_dal_key_sample[DAL_KEY_KEY1].sequence;
        app_drive_mode_handle_start_key();
    }

    app_drive_mode_update_line_transition();
    app_drive_mode_refresh_current();
}
