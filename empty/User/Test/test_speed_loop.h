/**
 * @file test_speed_loop.h
 * @brief 双轮速度闭环测试接口。
 */

#ifndef TEST_SPEED_LOOP_H
#define TEST_SPEED_LOOP_H

/* ======== include ======== */

#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* 双轮速度闭环测试周期，单位 ms；与测速快照刷新周期保持一致。 */
#define TEST_SPEED_LOOP_PERIOD_MS                (5U)

/* 目标速度初始值，单位 counts/speed-period；正值表示正转。 */
#define TEST_SPEED_LOOP_TARGET_INITIAL_CP        (15)

/* FireWater 串口波形帧周期，单位 ms；用于 PID 调参观察。 */
#define TEST_SPEED_LOOP_FIRE_PERIOD_MS           (5U)

/* 每个阶梯增加的目标速度，单位 counts/speed-period。 */
#define TEST_SPEED_LOOP_TARGET_STEP_CP           (3)

/* 目标速度上限，单位 counts/speed-period。 */
#define TEST_SPEED_LOOP_TARGET_MAX_CP            (30)

/* 目标速度阶梯更新周期，单位 ms。 */
#define TEST_SPEED_LOOP_TARGET_STEP_PERIOD_MS    (500U)

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

/**
 * @brief 以 FireWater 格式发送左右轮速度环 PID 波形数据。
 *
 * 通道 a、b、c 分别为左轮设定值、观测值、PID 输出；
 * 通道 d、e、f 分别为右轮设定值、观测值、PID 输出。
 * @param void 无参数。
 * @return 无。
 */
void test_speed_loop_fire_refresh(void);

/**
 * @brief 将双轮速度测试目标恢复为固定设定值。
 * @param void 无参数。
 * @return 无。
 */
void test_speed_loop_step_target(void);

#endif /* TEST_SPEED_LOOP_H */
