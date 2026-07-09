/**
 * @file bsp_pwm.c
 * @brief 板载 PWM 输出 BSP 实现。
 */

/* ======== include ======== */

#include "bsp_pwm.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 内部函数 ======== */

/**
 * @brief 判断 PWM 通道编号是否合法。
 * @param channel PWM 输出通道。
 * @return 合法返回 true，否则返回 false。
 */
static bool bsp_pwm_channel_is_valid(bsp_pwm_channel_e channel)
{
    return ((unsigned int)channel < (unsigned int)BSP_PWM_COUNT);
}

/**
 * @brief 读取 SysConfig 已初始化的 PWM 周期计数。
 * @param void 无参数。
 * @return PWM 周期计数。
 */
static uint32_t bsp_pwm_get_period_counts(void)
{
    return DL_Timer_getLoadValue(MOTOR_PWM_INST) + 1U;
}

/**
 * @brief 将占空比千分比转换为 Timer compare 值。
 * @param duty_permille 占空比，单位千分比。
 * @return Timer compare 值。
 */
static uint32_t bsp_pwm_duty_to_compare(uint16_t duty_permille)
{
    uint32_t duty = (uint32_t)duty_permille;
    uint32_t pwm_period_counts = bsp_pwm_get_period_counts();

    /*
     * WHY: SysConfig 生成的 PWM 在 compare=period 时对应 0% duty，
     * 因此需要按 period - duty*period/1000 反向换算。
     */
    return pwm_period_counts -
           ((pwm_period_counts * duty) /
               (uint32_t)BSP_PWM_DUTY_PERMILLE_MAX);
}

/**
 * @brief 将 BSP PWM 通道转换为 SysConfig 生成的捕获比较通道。
 * @param channel PWM 输出通道。
 * @param out 输出捕获比较通道。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
static bsp_status_e bsp_pwm_to_compare_channel(
    bsp_pwm_channel_e channel, DL_TIMER_CC_INDEX *out)
{
    if (out == NULL) {
        return BSP_ERR_PARAM;
    }

    switch (channel) {
    case BSP_PWM_MOTOR_LEFT:
        *out = GPIO_MOTOR_PWM_C1_IDX;
        break;
    case BSP_PWM_MOTOR_RIGHT:
        *out = GPIO_MOTOR_PWM_C0_IDX;
        break;
    default:
        return BSP_ERR_PARAM;
    }

    return BSP_OK;
}

/* ======== 公开 API ======== */

bsp_status_e bsp_pwm_start(void)
{
    DL_TimerG_startCounter(MOTOR_PWM_INST);
    return BSP_OK;
}

bsp_status_e bsp_pwm_set_duty(
    bsp_pwm_channel_e channel, uint16_t duty_permille)
{
    DL_TIMER_CC_INDEX compare_channel;
    uint32_t compare;

    if (!bsp_pwm_channel_is_valid(channel) ||
        (duty_permille > BSP_PWM_DUTY_PERMILLE_MAX)) {
        return BSP_ERR_PARAM;
    }

    if (bsp_pwm_to_compare_channel(channel, &compare_channel) != BSP_OK) {
        return BSP_ERR_PARAM;
    }

    compare = bsp_pwm_duty_to_compare(duty_permille);
    DL_TimerG_setCaptureCompareValue(
        MOTOR_PWM_INST, compare, compare_channel);
    return BSP_OK;
}
