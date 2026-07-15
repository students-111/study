/**
 * @file dal_pid.h
 * @brief PID 控制器实例统一管理接口。
 */

#ifndef DAL_PID_H
#define DAL_PID_H

/* ======== include ======== */

#include "PID.h"

/* ======== 可调参数宏定义 ======== */

/* 灰度循迹外环比例系数；灰度位置单位约为 -4.0 到 4.0，输出单位为速度差 counts/测速周期。 */
#define DAL_PID_LINE_KP              (4.900f)

/* 灰度循迹外环积分系数；循迹外环暂不使用积分，避免低速找线时积分累积。 */
#define DAL_PID_LINE_KI              (0.0f)

/* 灰度循迹外环微分系数；当前先关闭微分，优先验证比例差速方向。 */
#define DAL_PID_LINE_KD              (0.01f)

/* 灰度循迹外环输出限幅，单位 counts/测速周期；限制差速不超过基础速度太多。 */
#define DAL_PID_LINE_OUTPUT_LIMIT    (18.0f)

/* 灰度循迹外环积分限幅；积分关闭时保持为 0。 */
#define DAL_PID_LINE_ITERM_LIMIT     (0.0f)

/* ======== 类型定义 ======== */

/**
 * @brief PID 控制器实例编号。
 */
typedef enum {
    DAL_PID_ID_SPEED_LEFT = 0,       /**< 左轮速度内环 PID。 */
    DAL_PID_ID_SPEED_RIGHT,          /**< 右轮速度内环 PID。 */
    DAL_PID_ID_LINE,                 /**< 灰度循迹外环 PID。 */
    DAL_PID_ID_STRAIGHT_ANGLE,       /**< 直线行驶角度外环 PID。 */
    DAL_PID_ID_COUNT                 /**< PID 实例数量。 */
} dal_pid_id_e;

/* ======== 公开 API ======== */

/**
 * @brief 全局 PID 控制器实例数组。
 *
 * `dal_pid_init()` 负责按本文件参数初始化；APP 层可直接使用对应实例调用
 * PID 算法库函数，不在其它 DAL 模块中重复保存 PID 状态。
 */
extern PID_TypeDef pid_gather[DAL_PID_ID_COUNT];

/**
 * @brief 初始化全部 PID 控制器实例。
 * @param void 无参数。
 * @return 无。
 */
void dal_pid_init(void);

#endif /* DAL_PID_H */
