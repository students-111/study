/**
 * @file app_line_follow.h
 * @brief 灰度循迹 APP 接口。
 */

#ifndef APP_LINE_FOLLOW_H
#define APP_LINE_FOLLOW_H

/* ======== include ======== */

/* ======== 可调参数宏定义 ======== */

/* 循迹任务周期，单位 ms；应与速度闭环控制周期保持一致。 */
#define APP_LINE_FOLLOW_PERIOD_MS      (1ULL)

/* 基础目标速度，单位 counts/control-period；高速循迹阶段使用的直线目标速度。 */
#define APP_LINE_FOLLOW_BASE_SPEED_CP  (14)

/* PID 浮点输出转整数时的四舍五入补偿。 */
#define APP_LINE_FOLLOW_ROUND_OFFSET   (0.5f)

/* ======== 公开 API ======== */

/**
 * @brief 初始化灰度循迹任务状态。
 * @param void 无参数。
 * @return 无。
 */
void app_line_follow_init(void);

/**
 * @brief 停止灰度循迹输出并清除 APP 运行状态。
 * @param void 无参数。
 * @return 无。
 */
void app_line_follow_stop(void);

/**
 * @brief 刷新灰度循迹控制逻辑。
 * @param void 无参数。
 * @return 无。
 */
void app_line_follow_refresh(void);

#endif /* APP_LINE_FOLLOW_H */
