/**
 * @file test_speed_loop.c
 * @brief 双轮速度闭环测试实现。
 */

/* ======== include ======== */

#include "test_speed_loop.h"

#include <stdbool.h>
#include <stdint.h>

#include "PID.h"
#include "bsp_uart.h"
#include "dal_encoder.h"
#include "dal_motor.h"
#include "dal_pid.h"

/* ======== 全局实例 ======== */

test_speed_loop_sample_t g_test_speed_loop_sample;

/* ======== 内部变量 ======== */

static int16_t g_test_speed_loop_target_cp;

/* ======== 内部函数 ======== */

/**
 * @brief 将 PID 浮点输出转换为 int16_t 控制量。
 * @param value PID 浮点输出。
 * @return 截断后的 int16_t 控制量。
 */
static int16_t test_speed_loop_pid_to_i16(float value)
{
    return (int16_t)value;
}

/**
 * @brief 将 PID 浮点输出转换为电机矢量占空比命令。
 * @param value PID 浮点输出。
 * @return 限幅后的电机矢量占空比命令，单位千分比。
 */
static int16_t test_speed_loop_pid_to_permille(float value)
{
    int16_t output_permille = test_speed_loop_pid_to_i16(value);

    if (output_permille > DAL_MOTOR_OUTPUT_PERMILLE_MAX) {
        output_permille = DAL_MOTOR_OUTPUT_PERMILLE_MAX;
    } else if (output_permille < DAL_MOTOR_OUTPUT_PERMILLE_MIN) {
        output_permille = DAL_MOTOR_OUTPUT_PERMILLE_MIN;
    }

    return output_permille;
}

/**
 * @brief 复位速度闭环测试运行态。
 * @param void 无参数。
 * @return 无。
 */
static void test_speed_loop_reset_runtime(void)
{
    g_test_speed_loop_target_cp = (int16_t)TEST_SPEED_LOOP_TARGET_INITIAL_CP;
    g_test_speed_loop_sample.left.target_cp =
        g_test_speed_loop_target_cp;
    g_test_speed_loop_sample.left.measured_cp = 0;
    g_test_speed_loop_sample.left.output_permille = 0;
    g_test_speed_loop_sample.left.valid = 0U;
    g_test_speed_loop_sample.right.target_cp =
        g_test_speed_loop_target_cp;
    g_test_speed_loop_sample.right.measured_cp = 0;
    g_test_speed_loop_sample.right.output_permille = 0;
    g_test_speed_loop_sample.right.valid = 0U;
    g_test_speed_loop_sample.sequence = 0U;

    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_LEFT]);
    PID_Reset(&pid_gather[DAL_PID_ID_SPEED_RIGHT]);
}

/**
 * @brief 刷新单轮速度闭环。
 * @param pid_id PID 实例编号。
 * @param motor_id 电机编号。
 * @param target_cp 目标速度，单位 counts/speed-period。
 * @param measured_cp 实测速度，单位 counts/speed-period。
 * @param sample 单轮测试快照。
 * @return 无。
 */
static void test_speed_loop_apply_wheel(dal_pid_id_e pid_id,
    dal_motor_id_e motor_id,
    int16_t target_cp,
    int16_t measured_cp,
    test_speed_loop_wheel_sample_t *sample)
{
    int16_t output_permille;

    PID_ChangeSetpoint(&pid_gather[pid_id], (float)target_cp);
    output_permille = test_speed_loop_pid_to_permille(
        PID_Compute1_Rectangle(&pid_gather[pid_id], (float)measured_cp, 0U));
    (void)dal_motor_set_output(motor_id, output_permille);

    sample->target_cp = target_cp;
    sample->measured_cp = measured_cp;
    sample->output_permille = output_permille;
    sample->valid = 1U;
}

/* ======== 公开 API ======== */

void test_speed_loop_init(void)
{
    dal_pid_init();
    dal_motor_init();
    dal_encoder_init();
    dal_encoder_reset(DAL_ENCODER_M1);
    dal_encoder_reset(DAL_ENCODER_M2);
    dal_motor_stop_all();
    test_speed_loop_reset_runtime();
}

void test_speed_loop_refresh(void)
{
    int16_t left_measured_cp;
    int16_t right_measured_cp;
    bool left_valid;
    bool right_valid;

    left_valid = g_dal_encoder_sample[DAL_ENCODER_M1].speed_valid;
    right_valid = g_dal_encoder_sample[DAL_ENCODER_M2].speed_valid;
    left_measured_cp = g_dal_encoder_sample[DAL_ENCODER_M1].speed_cp;
    right_measured_cp = g_dal_encoder_sample[DAL_ENCODER_M2].speed_cp;

    g_test_speed_loop_sample.sequence++;
    g_test_speed_loop_sample.left.target_cp =
        g_test_speed_loop_target_cp;
    g_test_speed_loop_sample.left.measured_cp = left_measured_cp;
    g_test_speed_loop_sample.left.valid = left_valid ? 1U : 0U;
    g_test_speed_loop_sample.right.target_cp =
        g_test_speed_loop_target_cp;
    g_test_speed_loop_sample.right.measured_cp = right_measured_cp;
    g_test_speed_loop_sample.right.valid = right_valid ? 1U : 0U;

    if (!left_valid || !right_valid) {
        dal_motor_stop_all();
        g_test_speed_loop_sample.left.output_permille = 0;
        g_test_speed_loop_sample.right.output_permille = 0;
        return;
    }

    test_speed_loop_apply_wheel(DAL_PID_ID_SPEED_LEFT, DAL_MOTOR_LEFT,
        g_test_speed_loop_target_cp, left_measured_cp,
        &g_test_speed_loop_sample.left);
    test_speed_loop_apply_wheel(DAL_PID_ID_SPEED_RIGHT, DAL_MOTOR_RIGHT,
        g_test_speed_loop_target_cp, right_measured_cp,
        &g_test_speed_loop_sample.right);
}

void test_speed_loop_fire_refresh(void)
{
    (void)bsp_uart_try_printf(UART_ID_UART0,
        "a:%d,b:%d,c:%d,d:%d,e:%d,f:%d\r\n",
        (int)g_test_speed_loop_sample.left.target_cp,
        (int)g_test_speed_loop_sample.left.measured_cp,
        (int)g_test_speed_loop_sample.left.output_permille,
        (int)g_test_speed_loop_sample.right.target_cp,
        (int)g_test_speed_loop_sample.right.measured_cp,
        (int)g_test_speed_loop_sample.right.output_permille);
}

void test_speed_loop_step_target(void)
{
    g_test_speed_loop_target_cp = (int16_t)TEST_SPEED_LOOP_TARGET_INITIAL_CP;
}
