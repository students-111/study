/**
 * @file dal_motor.c
 * @brief TB6612 双电机控制 DAL 实现。
 */

/* ======== include ======== */

#include "dal_motor.h"

#include <stddef.h>

#include "../Driver/drv_motor.h"
#include "bsp_pwm.h"
#include "ti_msp_dl_config.h"

/* ======== 内部变量 ======== */

Motor_t g_dal_motor_instances[DAL_MOTOR_COUNT];

static const MotorHW_t g_dal_motor_hw[DAL_MOTOR_COUNT] = {
    {
        NULL,
        BSP_PWM_MOTOR_LEFT,
        BOARD_GPIO_PORT,
        BOARD_GPIO_MOTOR_AIN1_PIN,
        BOARD_GPIO_PORT,
        BOARD_GPIO_MOTOR_AIN2_PIN
    },
    {
        NULL,
        BSP_PWM_MOTOR_RIGHT,
        BOARD_GPIO_PORT,
        BOARD_GPIO_MOTOR_BIN1_PIN,
        BOARD_GPIO_PORT,
        BOARD_GPIO_MOTOR_BIN2_PIN
    }
};

/* ======== 公开 API ======== */

void dal_motor_init(void)
{
    /* 创建电机实例。 */
    (void)Motor_Init(&g_dal_motor_instances[DAL_MOTOR_LEFT],
        &g_dal_motor_hw[DAL_MOTOR_LEFT], 0U);
    (void)Motor_Init(&g_dal_motor_instances[DAL_MOTOR_RIGHT],
        &g_dal_motor_hw[DAL_MOTOR_RIGHT], 0U);

    /* 停止所有电机。 */
    dal_motor_stop_all();
    /* 初始化 STBY 为使能状态。 */
    DL_GPIO_setPins(BOARD_GPIO_PORT, BOARD_GPIO_MOTOR_STBY_PIN);
    /* 初始化 SysConfig 生成的 PWM 通道。 */
    (void)bsp_pwm_start();
}

//发送指定电机占空比
bool dal_motor_set_output(dal_motor_id_e id, int16_t output_permille)
{
    return Motor_SetOutput(&g_dal_motor_instances[id], output_permille) != 0U;
}

//停止指定电机
void dal_motor_stop(dal_motor_id_e id)
{
    (void)Motor_Stop(&g_dal_motor_instances[id]);
}

//停止所有电机
void dal_motor_stop_all(void)
{
    dal_motor_stop(DAL_MOTOR_LEFT);
    dal_motor_stop(DAL_MOTOR_RIGHT);
}
