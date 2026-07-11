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
#include "bsp_time.h"
#include "bsp_uart.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 内部变量 ======== */

static uint32_t g_app_line_follow_debug_last_print_ms;

static bool g_app_line_follow_debug_config_printed;

/* ======== 内部函数 ======== */


/**
 * @brief 将 PID 浮点输出转换为电机矢量占空比命令。
 * @param value PID 浮点输出。
 * @return 限幅后的电机矢量占空比命令，单位千分比。
 */
static int16_t app_line_follow_pid_to_permille(float value)
{
    int16_t output_permille = (int16_t)(value);

    if (output_permille > DAL_MOTOR_OUTPUT_PERMILLE_MAX) {
        output_permille = DAL_MOTOR_OUTPUT_PERMILLE_MAX;
    } else if (output_permille < DAL_MOTOR_OUTPUT_PERMILLE_MIN) {
        output_permille = DAL_MOTOR_OUTPUT_PERMILLE_MIN;
    }

    return output_permille;
}

/**
 * @brief 判断 PID 调试数据是否到达打印周期。
 * @param void 无参数。
 * @return 到达打印周期返回 true，否则返回 false。
 */
static bool app_line_follow_debug_is_due(void)
{
    uint32_t now_ms = bsp_time_get_ms();

    if ((uint32_t)(now_ms - g_app_line_follow_debug_last_print_ms) <
        APP_LINE_FOLLOW_DEBUG_PRINT_PERIOD_MS) {
        return false;
    }

    g_app_line_follow_debug_last_print_ms = now_ms;
    return true;
}

/**
 * @brief 将 PID 浮点调试量转换为受限的定点整数。
 * @param value 待转换的浮点调试量。
 * @param scale 定点缩放倍率。
 * @return 缩放后的有符号整数。
 */
static int app_line_follow_debug_scale(float value, float scale)
{
    float scaled_value = value * scale;

    if (scaled_value != scaled_value) {
        return 0;
    }
    if (scaled_value > (float)APP_LINE_FOLLOW_DEBUG_INT_MAX) {
        return APP_LINE_FOLLOW_DEBUG_INT_MAX;
    }
    if (scaled_value < (float)APP_LINE_FOLLOW_DEBUG_INT_MIN) {
        return APP_LINE_FOLLOW_DEBUG_INT_MIN;
    }

    return (int)scaled_value;
}

/**
 * @brief 按 VOFA CSV 格式打印循迹和左右速度环调试量。
 * @param pid_output 循迹外环输出，单位 counts/测速周期。
 * @param turn_cp 差速命令，单位 counts/测速周期。
 * @param left_target_cp 左轮目标速度，单位 counts/测速周期。
 * @param left_measured_cp 左轮实测速度，单位 counts/测速周期。
 * @param left_output_permille 左轮速度环输出，单位千分比。
 * @param right_target_cp 右轮目标速度，单位 counts/测速周期。
 * @param right_measured_cp 右轮实测速度，单位 counts/测速周期。
 * @param right_output_permille 右轮速度环输出，单位千分比。
 * @return 无。
 */
//static void app_line_follow_debug_print(float pid_output, int16_t turn_cp,
//    int16_t left_target_cp, int16_t left_measured_cp,
//    int16_t left_output_permille, int16_t right_target_cp,
//    int16_t right_measured_cp, int16_t right_output_permille)
//{
//    (void)bsp_uart_printf(UART_ID_UART0,
//        "%u,%u,%u,%d,%d,%d,%d,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
//        (unsigned int)g_dal_gray_sample.sequence,
//        (unsigned int)g_dal_gray_sample.raw_mask,
//        (unsigned int)g_dal_gray_sample.active_count,
//        app_line_follow_debug_scale(pid_gather[DAL_PID_ID_LINE].Setpoint,
//            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//        app_line_follow_debug_scale(g_dal_gray_sample.position,
//            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//        app_line_follow_debug_scale(pid_gather[DAL_PID_ID_LINE].LastError,
//            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//        app_line_follow_debug_scale(pid_gather[DAL_PID_ID_LINE].Kp,
//            APP_LINE_FOLLOW_DEBUG_GAIN_SIMPLE_SCALE),
//        (unsigned int)pid_gather[DAL_PID_ID_LINE].Manual,
//        app_line_follow_debug_scale(pid_gather[DAL_PID_ID_LINE].OutputLowerLimit,
//            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//        app_line_follow_debug_scale(pid_gather[DAL_PID_ID_LINE].OutputUpperLimit,
//            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//        app_line_follow_debug_scale(pid_output,
//            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//        turn_cp,
//        left_target_cp, left_measured_cp, left_output_permille,
//        right_target_cp, right_measured_cp, right_output_permille);
//}


/**
 * @brief 非阻塞打印循迹调试短帧。
 * @param pid_output 循迹外环输出，单位 counts/测速周期。
 * @param turn_cp 差速命令，单位 counts/测速周期。
 * @param left_target_cp 左轮目标速度，单位 counts/测速周期。
 * @param left_measured_cp 左轮实测速度，单位 counts/测速周期。
 * @param left_output_permille 左轮速度环输出，单位千分比。
 * @param right_target_cp 右轮目标速度，单位 counts/测速周期。
 * @param right_measured_cp 右轮实测速度，单位 counts/测速周期。
 * @param right_output_permille 右轮速度环输出，单位千分比。
 * @return 无。
 */
static void app_line_follow_debug_try_print(float pid_output, int16_t turn_cp,
    int16_t left_target_cp, int16_t left_measured_cp,
    int16_t left_output_permille, int16_t right_target_cp,
    int16_t right_measured_cp, int16_t right_output_permille)
{
    (void)bsp_uart_try_printf(UART_ID_UART0,
        "%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
        (unsigned int)g_dal_gray_sample.sequence,
        (unsigned int)g_dal_gray_sample.raw_mask,
        (unsigned int)g_dal_gray_sample.active_count,
        app_line_follow_debug_scale(g_dal_gray_sample.position,
            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
        app_line_follow_debug_scale(pid_output,
            APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
        turn_cp,
        left_target_cp,
        left_measured_cp,
        left_output_permille,
        right_target_cp,
        right_measured_cp,
        right_output_permille);
}

/* ======== 公开 API ======== */

void app_line_follow_stop(void)
{
	//停止电机
    dal_motor_stop_all();
	//复位PID
    PID_Reset(&pid_gather[DAL_PID_ID_LINE]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
    g_app_line_follow_debug_last_print_ms = bsp_time_get_ms();
    g_app_line_follow_debug_config_printed = false;
}




void app_line_follow_init(void)
{
    dal_pid_init();
    dal_motor_init();

	//停止电机
    dal_motor_stop_all();
	//复位PID
    PID_Reset(&pid_gather[DAL_PID_ID_LINE]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
    g_app_line_follow_debug_last_print_ms = bsp_time_get_ms();
    g_app_line_follow_debug_config_printed = false;
}

void app_line_follow_refresh(void)
{
    int16_t turn_cp;
    int16_t left_target_cp;
    int16_t right_target_cp;
    int16_t left_measured_cp;
    int16_t right_measured_cp;
    int16_t left_output_permille;
    int16_t right_output_permille;
    float pid_output;
    float left_pid_output;
    float right_pid_output;
    bool left_valid;
    bool right_valid;

	/*如果没有刷新过 或者 没检测到线 就停止所有电机*/
    if ((g_dal_gray_sample.sequence == 0U) ||
        !g_dal_gray_sample.line_detected) {
        dal_motor_stop_all();
        if (app_line_follow_debug_is_due()) {
            app_line_follow_debug_try_print(0.0f, 0, 0, 0, 0, 0, 0, 0);
        }
        return;
    }

    /*循迹环PID计算*/
    pid_output = PID_Compute1_Rectangle(&pid_gather[DAL_PID_ID_LINE],
        g_dal_gray_sample.position, 0U);
    turn_cp = (int16_t)(pid_output);
    
    /*差速计算*/
    left_target_cp = (int16_t)(APP_LINE_FOLLOW_BASE_SPEED_CP - turn_cp);
    right_target_cp = (int16_t)(APP_LINE_FOLLOW_BASE_SPEED_CP + turn_cp);

	/*是否建立基准*/
    left_valid = g_dal_encoder_sample[DAL_ENCODER_M1].speed_valid;
    right_valid = g_dal_encoder_sample[DAL_ENCODER_M2].speed_valid;
	
    left_measured_cp = g_dal_encoder_sample[DAL_ENCODER_M1].speed_cp;
    right_measured_cp = g_dal_encoder_sample[DAL_ENCODER_M2].speed_cp;

	/*如果没有建立 停止所有电机*/
    if (!left_valid || !right_valid) {
        dal_motor_stop_all();
        if (app_line_follow_debug_is_due()) {
            app_line_follow_debug_try_print(pid_output, turn_cp,
                left_target_cp, left_measured_cp, 0,
                right_target_cp, right_measured_cp, 0);
        }
        return;
    }

    /* 速度环 PID 计算。 */
    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_target_cp);

    left_pid_output = PID_Compute1_Rectangle(
        &pid_gather[DAL_PID_ID_SPEED_LEFT],
        (float)left_measured_cp, 0U);
    left_output_permille = app_line_follow_pid_to_permille(left_pid_output);
    (void)dal_motor_set_output(DAL_MOTOR_LEFT, left_output_permille);//设置占空比

    PID_ChangeSetpoint(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_target_cp);
    right_pid_output = PID_Compute1_Rectangle(
        &pid_gather[DAL_PID_ID_SPEED_RIGHT],
        (float)right_measured_cp, 0U);
    right_output_permille = app_line_follow_pid_to_permille(right_pid_output);
    (void)dal_motor_set_output(DAL_MOTOR_RIGHT, right_output_permille);

//    if (app_line_follow_debug_is_due()) {
//        if (!g_app_line_follow_debug_config_printed) {
//            int print_result;

//            print_result = bsp_uart_printf(UART_ID_UART0,
//                "PIDCFG,kz=%ld,vz=%ld,base=%d,log=%lums;"
//                "LN,k=%ld/%ld/%ld,ol=%ld/%ld,il=%ld/%ld;"
//                "SL,k=%ld/%ld/%ld,ol=%ld/%ld,il=%ld/%ld;"
//                "SR,k=%ld/%ld/%ld,ol=%ld/%ld,il=%ld/%ld\r\n",
//                (long)APP_LINE_FOLLOW_DEBUG_GAIN_SCALE,
//                (long)APP_LINE_FOLLOW_DEBUG_SPEED_SCALE,
//                APP_LINE_FOLLOW_BASE_SPEED_CP,
//                (unsigned long)APP_LINE_FOLLOW_DEBUG_PRINT_PERIOD_MS,
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].Kp,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].Ki,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].Kd,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].OutputLowerLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].OutputUpperLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].ITermLowerLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].ITermUpperLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].Kp,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].Ki,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].Kd,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].OutputLowerLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].OutputUpperLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].ITermLowerLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].ITermUpperLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].Kp,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].Ki,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].Kd,
//                    APP_LINE_FOLLOW_DEBUG_GAIN_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].OutputLowerLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].OutputUpperLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].ITermLowerLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].ITermUpperLimit,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE));
//            if (print_result > 0) {
//                g_app_line_follow_debug_config_printed = true;
//            }
//        } else {
//            (void)bsp_uart_printf(UART_ID_UART0,
//                "PIDD,ms=%lu,g=%02X/%u;"
//                "LN100,s=%ld,v=%ld,e=%ld,p=%ld,i=%ld,d=%ld,o=%ld,t=%d;"
//                "SL10,s=%d,v=%d,e=%ld,p=%ld,i=%ld,d=%ld,o=%ld,c=%d;"
//                "SR10,s=%d,v=%d,e=%ld,p=%ld,i=%ld,d=%ld,o=%ld,c=%d\r\n",
//                (unsigned long)bsp_time_get_ms(),
//                (unsigned int)g_dal_gray_sample.raw_mask,
//                (unsigned int)g_dal_gray_sample.active_count,
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].Setpoint,
//                    APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//                (long)app_line_follow_debug_scale(
//                    g_dal_gray_sample.position,
//                    APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].LastError,
//                    APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].Kp *
//                        pid_gather[DAL_PID_ID_LINE].LastError,
//                    APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].Ki *
//                        pid_gather[DAL_PID_ID_LINE].ITerm,
//                    APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_LINE].Kd *
//                        (pid_gather[DAL_PID_ID_LINE].LastError -
//                         pid_gather[DAL_PID_ID_LINE].PreviousError),
//                    APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//                (long)app_line_follow_debug_scale(pid_output,
//                    APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//                turn_cp,
//                left_target_cp, left_measured_cp,
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].LastError,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].Kp *
//                        pid_gather[DAL_PID_ID_SPEED_LEFT].LastError,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].Ki *
//                        pid_gather[DAL_PID_ID_SPEED_LEFT].ITerm,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_LEFT].Kd *
//                        (pid_gather[DAL_PID_ID_SPEED_LEFT].LastError -
//                         pid_gather[DAL_PID_ID_SPEED_LEFT].PreviousError),
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(left_pid_output,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                left_output_permille,
//                right_target_cp, right_measured_cp,
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].LastError,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].Kp *
//                        pid_gather[DAL_PID_ID_SPEED_RIGHT].LastError,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].Ki *
//                        pid_gather[DAL_PID_ID_SPEED_RIGHT].ITerm,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(
//                    pid_gather[DAL_PID_ID_SPEED_RIGHT].Kd *
//                        (pid_gather[DAL_PID_ID_SPEED_RIGHT].LastError -
//                         pid_gather[DAL_PID_ID_SPEED_RIGHT].PreviousError),
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                (long)app_line_follow_debug_scale(right_pid_output,
//                    APP_LINE_FOLLOW_DEBUG_SPEED_SCALE),
//                right_output_permille);
//        }
//    }
//bsp_uart_printf(UART_ID_UART0,"Hello");
    if (app_line_follow_debug_is_due()) {
        app_line_follow_debug_try_print(pid_output, turn_cp,
            left_target_cp, left_measured_cp, left_output_permille,
            right_target_cp, right_measured_cp, right_output_permille);
    }

//    if (app_line_follow_debug_is_due()) {
//        (void)bsp_uart_printf(UART_ID_UART0,
//            "%ld,%ld,%ld,%d,%d,%d,%d,%d,%d\r\n",
//            (long)app_line_follow_debug_scale(
//                pid_gather[DAL_PID_ID_LINE].Setpoint,
//                APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//            (long)app_line_follow_debug_scale(
//                g_dal_gray_sample.position,
//                APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//            (long)app_line_follow_debug_scale(pid_output,
//                APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//            left_target_cp, left_measured_cp, left_output_permille,
//            right_target_cp, right_measured_cp, right_output_permille);
//    }

//	bsp_uart_printf(UART_ID_UART0,"%ld,%ld,%ld\r\n",(long)app_line_follow_debug_scale(
//                pid_gather[DAL_PID_ID_LINE].Setpoint,
//                APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//            (long)app_line_follow_debug_scale(
//                g_dal_gray_sample.position,
//                APP_LINE_FOLLOW_DEBUG_LINE_SCALE),
//            (long)app_line_follow_debug_scale(pid_output,
//                APP_LINE_FOLLOW_DEBUG_LINE_SCALE));
}
