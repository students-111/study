/**
 * @file dal_motor.c
 * @brief TB6612 双电机控制 DAL 实现。
 */

/* ======== include ======== */

#include "dal_motor.h"

#include <stddef.h>

#include "../Driver/drv_motor.h"
#include "bsp_gpio.h"
#include "bsp_pwm.h"

/* ======== 可调参数宏定义 ======== */

/* DAL 输入速度为 PWM 千分比，驱动库速度为百分比。 */
#define DAL_MOTOR_PERMILLE_TO_PERCENT  (10)

/* ======== 内部变量 ======== */

static Motor_t g_dal_motor_instances[DAL_MOTOR_COUNT];

static const MotorHW_t g_dal_motor_hw[DAL_MOTOR_COUNT] = {
    {
        NULL,
        BSP_PWM_MOTOR_LEFT,
        NULL,
        BSP_GPIO_MOTOR_AIN1,
        BSP_GPIO_MOTOR_AIN2
    },
    {
        NULL,
        BSP_PWM_MOTOR_RIGHT,
        NULL,
        BSP_GPIO_MOTOR_BIN1,
        BSP_GPIO_MOTOR_BIN2
    }
};

/* ======== 内部函数 ======== */

/**
 * @brief 将 DAL 千分比速度转换为驱动库百分比速度。
 * @param speed_permille DAL 速度命令，单位 PWM 千分比。
 * @return 驱动库速度命令，范围 -100 到 100。
 */
static int8_t dal_motor_permille_to_percent(int16_t speed_permille)
{
    int16_t percent = speed_permille / DAL_MOTOR_PERMILLE_TO_PERCENT;

    if (percent > MOTOR_SPEED_MAX) {
        percent = MOTOR_SPEED_MAX;
    } else if (percent < -MOTOR_SPEED_MAX) {
        percent = -MOTOR_SPEED_MAX;
    }

    return (int8_t)percent;
}

/* ======== 公开 API ======== */

void dal_motor_init(void)
{
    (void)bsp_gpio_init_output(BSP_GPIO_MOTOR_AIN1);
    (void)bsp_gpio_init_output(BSP_GPIO_MOTOR_AIN2);
    (void)bsp_gpio_init_output(BSP_GPIO_MOTOR_BIN1);
    (void)bsp_gpio_init_output(BSP_GPIO_MOTOR_BIN2);
    (void)bsp_gpio_init_output(BSP_GPIO_MOTOR_STBY);

	//创建电机实例
    (void)Motor_Init(&g_dal_motor_instances[DAL_MOTOR_LEFT],
        &g_dal_motor_hw[DAL_MOTOR_LEFT], 0U);
    (void)Motor_Init(&g_dal_motor_instances[DAL_MOTOR_RIGHT],
        &g_dal_motor_hw[DAL_MOTOR_RIGHT], 0U);

	//停止所有电机
    dal_motor_stop_all();
	//初始化STBY为下拉不使能
    (void)bsp_gpio_write(BSP_GPIO_MOTOR_STBY, true);
	//初始化syscfig生成的PWM通道
    (void)bsp_pwm_start();
}

//设置电机速度
bool dal_motor_set_speed(dal_motor_id_e id, int16_t speed_permille)
{
    return Motor_SetSpeed(&g_dal_motor_instances[id],
        dal_motor_permille_to_percent(speed_permille)) != 0U;
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
