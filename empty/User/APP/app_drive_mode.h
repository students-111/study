/**
 * @file app_drive_mode.h
 * @brief 行车模式管理 APP 接口。
 */

#ifndef APP_DRIVE_MODE_H
#define APP_DRIVE_MODE_H

/* ======== include ======== */

/* ======== 可调参数宏定义 ======== */

/* 行车模式管理任务周期，单位 ms；需与控制外环周期保持一致。 */
#define APP_DRIVE_MODE_PERIOD_MS       (1ULL)

/* ======== 公开 API ======== */

/**
 * @brief 初始化行车模式管理状态和子模式。
 * @param void 无参数。
 * @return 无。
 */
void app_drive_mode_init(void);

/**
 * @brief 刷新 Key1 启动、灰线自动切换和当前行车模式。
 * @param void 无参数。
 * @return 无。
 */
void app_drive_mode_refresh(void);

#endif /* APP_DRIVE_MODE_H */
