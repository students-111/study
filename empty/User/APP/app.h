/**
 * @file app.h
 * @brief 循迹赛题行驶 APP 接口。
 */

#ifndef APP_H
#define APP_H

/* ======== 可调参数宏定义 ======== */

/* 赛题状态机任务周期，单位 ms；需与灰度、按键和姿态刷新周期保持一致。 */
#define APP_PERIOD_MS                            (10U)

/* 黑白边沿确认所需的连续灰度样本数，单位 次；用于抑制赛道边缘抖动。 */
#define APP_GRAY_CONFIRM_SAMPLE_COUNT            (3U)

/* 第二题直行遇黑线停车确认次数，单位 次；数值越小越容易停车。 */
#define APP_TASK2_STRAIGHT_NODE_CONFIRM_COUNT    (1U)

/* 第二题 C 点出弯后直行航向补偿，单位 0.001 度；方向反了就改成正数。 */
#define APP_TASK2_C_TO_D_YAW_OFFSET_MDEG         (-5000L)

/* 节点提示暂停时长，单位 ms；当前以停车替代尚未具备的声光提示。 */
#define APP_NODE_PAUSE_DURATION_MS               (1000U)

/* task3 的 A→C 相对转角，单位 0.001 度；需按实车 Yaw 正方向调参。 */
#define APP_TASK3_TURN_A_TO_C_MDEG               (-32000L)

/* task3 的 B→D 相对转角，单位 0.001 度；需按实车 Yaw 正方向调参。 */
#define APP_TASK3_TURN_B_TO_D_MDEG               (34700L)

/* task3 原地转向输出，单位千分比；过大易过冲，过小可能无法克服静摩擦。 */
#define APP_TASK3_TURN_OUTPUT_PERMILLE           (300)

/* task3 转向到位容差，单位 0.001 度；进入该范围即停止原地转向。 */
#define APP_TASK3_TURN_TOLERANCE_MDEG            (2000L)

/* 正 Yaw 转向时左轮输出符号；设为 1 或 -1，按实车 Yaw 与电机方向对应关系调节。 */
#define APP_TASK3_POSITIVE_YAW_LEFT_OUTPUT_SIGN  (-1)

/* task3 在 C 点寻找右侧半圆时的转向 Yaw 方向；设为 1 或 -1，需按实车调节。 */
#define APP_TASK3_FIND_LINE_TO_B_YAW_DIRECTION   (1)

/* task3 在 D 点寻找左侧半圆时的转向 Yaw 方向；设为 1 或 -1，需按实车调节。 */
#define APP_TASK3_FIND_LINE_TO_A_YAW_DIRECTION   (-1)

/* task3 寻找弧线的最长转向时长，单位 ms；超时后停车，避免脱线时持续原地旋转。 */
#define APP_TASK3_FIND_LINE_TIMEOUT_MS            (5000U)

/* ======== 类型定义 ======== */

/**
 * @brief 可执行的循迹赛题编号。
 */
typedef enum {
    APP_TASK_2 = 0,    /**< 第二题：A→B→C→D→A。 */
    APP_TASK_3,        /**< 第三题：A→C→B→D→A。 */
    APP_TASK_4,        /**< 第四题：A→C→B→D→A 连续四圈。 */
    APP_TASK_COUNT     /**< 可执行赛题数量。 */
} app_task_e;

/* 当前编译启用的赛题；需要切回时改为对应的 APP_TASK_x。 */
#define APP_ACTIVE_TASK                         (APP_TASK_2)

/* ======== 公开 API ======== */

/**
 * @brief 初始化当前启用赛题的状态机及其复用行驶 APP。
 * @param void 无参数。
 * @return 无。
 */
void app_init(void);

/**
 * @brief 刷新当前启用赛题的状态机。
 * @param void 无参数。
 * @return 无。
 */
void app_refresh(void);

#endif /* APP_H */
