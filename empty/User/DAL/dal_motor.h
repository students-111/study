/**
 * @file dal_motor.h
 * @brief TB6612 双电机控制 DAL 接口。
 */

#ifndef DAL_MOTOR_H
#define DAL_MOTOR_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

/* ======== 类型定义 ======== */

/**
 * @brief 电机逻辑编号。
 */
typedef enum {
    DAL_MOTOR_LEFT = 0,    /**< 左电机。 */
    DAL_MOTOR_RIGHT,       /**< 右电机。 */
    DAL_MOTOR_COUNT        /**< 电机数量。 */
} dal_motor_id_e;

/* ======== 公开 API ======== */

/**
 * @brief 初始化 TB6612FNG 双电机控制接口。
 * @param void 无参数。
 * @return 无。
 */
void dal_motor_init(void);

/**
 * @brief 设置单个电机速度。
 * @param id 电机编号。
 * @param speed_permille 速度命令，单位 PWM 千分比。
 * @return 电机编号合法返回 true，否则返回 false。
 */
bool dal_motor_set_speed(dal_motor_id_e id, int16_t speed_permille);

/**
 * @brief 停止单个电机。
 * @param id 电机编号。
 * @return 无。
 */
void dal_motor_stop(dal_motor_id_e id);

/**
 * @brief 停止全部电机。
 * @param void 无参数。
 * @return 无。
 */
void dal_motor_stop_all(void);

#endif /* DAL_MOTOR_H */
