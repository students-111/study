/**
 * @file app.h
 * @brief 第二题赛道行驶 APP 接口。
 */

#ifndef APP_H
#define APP_H

/* ======== 可调参数宏定义 ======== */

/* 赛道状态机任务周期，单位 ms；需与灰度和按键刷新周期保持一致。 */
#define APP_PERIOD_MS                    (10U)

/* 黑白边沿确认所需的连续灰度样本数，单位 次；用于抑制赛道边缘抖动。 */
#define APP_GRAY_CONFIRM_SAMPLE_COUNT    (3U)

/* 节点提示暂停时长，单位 ms；当前以停车替代尚未具备的声光提示。 */
#define APP_NODE_PAUSE_DURATION_MS       (1000U)

/* ======== 公开 API ======== */

/**
 * @brief 初始化第二题赛道状态机及其复用的行驶 APP。
 * @param void 无参数。
 * @return 无。
 */
void app_init(void);

/**
 * @brief 刷新第二题赛道状态机。
 * @param void 无参数。
 * @return 无。
 */
void app_refresh(void);

#endif /* APP_H */
