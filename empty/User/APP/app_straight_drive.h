/**
 * @file app_straight_drive.h
 * @brief Yaw 直线保持 APP 接口。
 */

#ifndef APP_STRAIGHT_DRIVE_H
#define APP_STRAIGHT_DRIVE_H

/* ======== include ======== */

/* ======== 可调参数宏定义 ======== */

/* 直线保持任务周期，单位 ms；需与速度内环控制周期保持一致。 */
#define APP_STRAIGHT_DRIVE_PERIOD_MS       (1ULL)

/* 直线保持基础目标速度，单位 counts/control-period。 */
#define APP_STRAIGHT_DRIVE_BASE_SPEED_CP   (20)

/* PID 浮点输出转整数时的四舍五入补偿。 */
#define APP_STRAIGHT_DRIVE_ROUND_OFFSET    (0.5f)

/* Yaw 半圈角度，单位 0.001 度；用于计算最短角差。 */
#define APP_STRAIGHT_DRIVE_YAW_HALF_TURN_MDEG  (180000L)

/* Yaw 一圈角度，单位 0.001 度；用于计算最短角差。 */
#define APP_STRAIGHT_DRIVE_YAW_FULL_TURN_MDEG  (360000L)

/* ======== 公开 API ======== */

/**
 * @brief 初始化 Yaw 直线保持状态。
 * @param void 无参数。
 * @return 无。
 */
void app_straight_drive_init(void);

/**
 * @brief 进入 Yaw 直线保持模式并锁定当前航向。
 * @param void 无参数。
 * @return 无。
 */
void app_straight_drive_enter(void);

/**
 * @brief 停止 Yaw 直线保持输出并清除运行状态。
 * @param void 无参数。
 * @return 无。
 */
void app_straight_drive_stop(void);

/**
 * @brief 刷新 Yaw 直线保持控制逻辑。
 * @param void 无参数。
 * @return 无。
 */
void app_straight_drive_refresh(void);

#endif /* APP_STRAIGHT_DRIVE_H */
