/**
 * @file test_motor_pwm.h
 * @brief 双轮电机 PWM 测试接口。
 */

#ifndef TEST_MOTOR_PWM_H
#define TEST_MOTOR_PWM_H

/* ======== include ======== */

/* ======== 可调参数宏定义 ======== */

/* 双轮 PWM 测试任务周期，单位 ms；每 10s 递增一次占空比。 */
#define TEST_MOTOR_PWM_PERIOD_MS             (10000ULL)

/* 双轮 PWM 测试起始占空比，单位千分比；0 表示从停止开始。 */
#define TEST_MOTOR_PWM_START_PERMILLE        (0)

/* 双轮 PWM 测试单次递增占空比，单位千分比。 */
#define TEST_MOTOR_PWM_STEP_PERMILLE         (100)

/* 双轮 PWM 测试最大占空比，单位千分比；达到后保持。 */
#define TEST_MOTOR_PWM_MAX_PERMILLE          (1000)

/* ======== 公开 API ======== */

/**
 * @brief 初始化双轮 PWM 测试。
 * @param void 无参数。
 * @return 无。
 */
void test_motor_pwm_init(void);

/**
 * @brief 周期刷新双轮 PWM 测试输出。
 * @param void 无参数。
 * @return 无。
 */
void test_motor_pwm_refresh(void);

#endif /* TEST_MOTOR_PWM_H */
