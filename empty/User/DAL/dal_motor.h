/**
 * @file dal_motor.h
 * @brief TB6612 双电机控制 DAL 接口。
 */

#ifndef DAL_MOTOR_H
#define DAL_MOTOR_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* 电机矢量占空比最大千分比；1000 表示 100% 输出。 */
#define DAL_MOTOR_OUTPUT_PERMILLE_MAX  (1000)

/* 电机矢量占空比最小千分比；负值表示反转。 */
#define DAL_MOTOR_OUTPUT_PERMILLE_MIN  (-DAL_MOTOR_OUTPUT_PERMILLE_MAX)

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
 * @brief 设置单个电机矢量占空比输出。
 * @param id 电机编号。
 * @param output_permille 矢量占空比，范围 -1000 到 1000，单位千分比。
 * @return 设置成功返回 true，否则返回 false。
 */
bool dal_motor_set_output(dal_motor_id_e id, int16_t output_permille);

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
