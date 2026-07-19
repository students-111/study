/**
 * @file app_line_follow.h
 * @brief 灰度循迹 APP 接口。
 */

#ifndef APP_LINE_FOLLOW_H
#define APP_LINE_FOLLOW_H

/* ======== include ======== */

/* ======== 可调参数宏定义 ======== */

/* 循迹任务周期，单位 ms；读取最新编码器测速快照进行速度内环控制。 */
#define APP_LINE_FOLLOW_PERIOD_MS      (10ULL)

/* 基础目标速度，单位 counts/speed-period；循迹阶段使用。 */
#define APP_LINE_FOLLOW_BASE_SPEED_CP  (40)

/* PID 调试打印周期，单位 ms；限制串口占用，避免影响循迹控制实时性。 */
#define APP_LINE_FOLLOW_DEBUG_PRINT_PERIOD_MS  (100U)

/* 循迹环调试量定点倍率；整数值除以 100 还原原始浮点值。 */
#define APP_LINE_FOLLOW_DEBUG_LINE_SCALE        (100.0f)

/* 速度环调试量定点倍率；整数值除以 10 还原原始浮点值。 */
#define APP_LINE_FOLLOW_DEBUG_SPEED_SCALE       (10.0f)

/* PID 增益调试量定点倍率；整数值除以 1000000 还原原始浮点值。 */
#define APP_LINE_FOLLOW_DEBUG_GAIN_SCALE        (1000000.0f)

/* PID 增益简化调试倍率；整数值除以 100 还原原始浮点值。 */
#define APP_LINE_FOLLOW_DEBUG_GAIN_SIMPLE_SCALE (100.0f)

/* 调试定点整数最大值；异常浮点值会被夹紧到该范围。 */
#define APP_LINE_FOLLOW_DEBUG_INT_MAX           (32767)

/* 调试定点整数最小值；异常浮点值会被夹紧到该范围。 */
#define APP_LINE_FOLLOW_DEBUG_INT_MIN           (-32768)

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
 * @brief 使用左右轮速度 PID 将小车保持在零速度。
 * @param void 无参数。
 * @return 无。
 */
void app_line_follow_hold_stop(void);

/**
 * @brief 刷新灰度循迹控制逻辑。
 * @param void 无参数。
 * @return 无。
 */
void app_line_follow_refresh(void);

#endif /* APP_LINE_FOLLOW_H */
