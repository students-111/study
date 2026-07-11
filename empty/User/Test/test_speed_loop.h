/**
 * @file test_speed_loop.h
 * @brief 双轮速度闭环测试接口。
 */

#ifndef TEST_SPEED_LOOP_H
#define TEST_SPEED_LOOP_H

/* ======== include ======== */

#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* 双轮速度闭环测试周期，单位 ms；读取最新编码器测速快照。 */
#define TEST_SPEED_LOOP_PERIOD_MS        (20ULL)

/* 左轮目标速度，单位 counts/speed-period；正值表示正转。 */
#define TEST_SPEED_LOOP_LEFT_TARGET_CP   (6)

/* 右轮目标速度，单位 counts/speed-period；正值表示正转。 */
#define TEST_SPEED_LOOP_RIGHT_TARGET_CP  (6)

/* ======== 类型定义 ======== */

/**
 * @brief 单轮速度闭环测试快照。
 */
typedef struct {
    int16_t target_cp;       /**< 目标速度，单位 counts/speed-period。 */
    int16_t measured_cp;     /**< 实测速度，单位 counts/speed-period。 */
    int16_t output_permille; /**< PID 输出后的电机矢量占空比命令，单位千分比。 */
    uint8_t valid;           /**< 本周期速度数据是否有效。 */
} test_speed_loop_wheel_sample_t;

/**
 * @brief 双轮速度闭环测试快照。
 */
typedef struct {
    test_speed_loop_wheel_sample_t left;  /**< 左轮测试快照。 */
    test_speed_loop_wheel_sample_t right; /**< 右轮测试快照。 */
    uint32_t sequence;                    /**< 每次速度环刷新递增的序号。 */
} test_speed_loop_sample_t;

/* ======== 全局实例 ======== */

/**
 * @brief 双轮速度闭环测试最新快照，供调试器只读观察。
 */
extern test_speed_loop_sample_t g_test_speed_loop_sample;

/* ======== 公开 API ======== */

/**
 * @brief 初始化双轮速度闭环测试。
 * @param void 无参数。
 * @return 无。
 */
void test_speed_loop_init(void);

/**
 * @brief 周期刷新双轮速度闭环测试。
 * @param void 无参数。
 * @return 无。
 */
void test_speed_loop_refresh(void);

#endif /* TEST_SPEED_LOOP_H */
