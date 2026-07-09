/**
 * @file bsp_pwm.h
 * @brief 板载 PWM 输出 BSP 接口。
 */

#ifndef BSP_PWM_H
#define BSP_PWM_H

/* ======== include ======== */

#include <stdint.h>

#include "bsp_status.h"

/* ======== 可调参数宏定义 ======== */

/* PWM 占空比千分比最大值；1000 表示 100%。 */
#define BSP_PWM_DUTY_PERMILLE_MAX      (1000U)

/* ======== 类型定义 ======== */

/**
 * @brief BSP PWM 输出通道。
 */
typedef enum {
    BSP_PWM_MOTOR_LEFT = 0,    /**< 左电机 PWM 输出。 */
    BSP_PWM_MOTOR_RIGHT,       /**< 右电机 PWM 输出。 */
    BSP_PWM_COUNT              /**< PWM 输出通道数量。 */
} bsp_pwm_channel_e;

/* ======== 公开 API ======== */

/**
 * @brief 启动 SysConfig 已初始化的 PWM 计数器。
 * @param void 无参数。
 * @return 成功返回 BSP_OK。
 */
bsp_status_e bsp_pwm_start(void);

/**
 * @brief 设置指定 PWM 通道占空比。
 * @param channel PWM 输出通道。
 * @param duty_permille 占空比，范围 0~1000，单位千分比。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
bsp_status_e bsp_pwm_set_duty(
    bsp_pwm_channel_e channel, uint16_t duty_permille);

#endif /* BSP_PWM_H */
