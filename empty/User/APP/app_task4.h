/**
 * @file app_task4.h
 * @brief 第四题四圈连续行驶状态机接口。
 */

#ifndef APP_TASK4_H
#define APP_TASK4_H

/* ======== 可调参数宏定义 ======== */

/* 第四题完成圈数；仅完成该次数的 D→A 弧线后停车。 */
#define APP_TASK4_LAP_COUNT                       (4U)

/* 节点低速转向基础速度，单位 counts/测速周期；初始值用于经验调参。 */
#define APP_TASK4_TRANSITION_BASE_SPEED_CP         (45)

/* 节点低速转向差速量，单位 counts/测速周期；与基础速度共同决定转弯曲率。 */
#define APP_TASK4_TRANSITION_TURN_DELTA_CP         (25)

/* 低速航向转向到位容差，单位 0.001 度；进入该范围后切换到下一行驶段。 */
#define APP_TASK4_TURN_TOLERANCE_MDEG              (2000L)

/* 正 Yaw 转向时左右轮差速符号；设为 1 或 -1，按实车 Yaw 与电机方向关系调节。 */
#define APP_TASK4_POSITIVE_YAW_TURN_SIGN           (1)

/* A→C 低速航向转向相对角，单位 0.001 度；需按实车调参。 */
#define APP_TASK4_FIRST_TURN_A_TO_C_MDEG           (-36500L)

/* 后续圈 A→C 低速航向转向相对角，单位 0.001 度；从 D→A 回到 A 后使用。 */
#define APP_TASK4_REPEAT_TURN_A_TO_C_MDEG          (-54000L)

/* B→D 低速航向转向相对角，单位 0.001 度；需按实车调参。 */
#define APP_TASK4_TURN_B_TO_D_MDEG                 (60000L)

/* C 点找右侧半圆的差速方向；设为 1 或 -1，按实车调节。 */
#define APP_TASK4_FIND_LINE_TO_B_TURN_SIGN         (1)

/* D 点找左侧半圆的差速方向；设为 1 或 -1，按实车调节。 */
#define APP_TASK4_FIND_LINE_TO_A_TURN_SIGN         (-1)

/* 低速找线的最长持续时间，单位 ms；超时后进入最终零速度状态。 */
#define APP_TASK4_FIND_LINE_TIMEOUT_MS              (5000U)

/* 首次切入弧线后所需的最短稳定循迹时间，单位 ms；期间丢线只允许重新找弧线。 */
#define APP_TASK4_ARC_CAPTURE_DURATION_MS           (300U)

/* ======== 公开 API ======== */

/**
 * @brief 初始化第四题四圈连续行驶状态机。
 * @param void 无参数。
 * @return 无。
 */
void app_task4_init(void);

/**
 * @brief 刷新第四题四圈连续行驶状态机。
 * @param void 无参数。
 * @return 无。
 */
void app_task4_refresh(void);

#endif /* APP_TASK4_H */
