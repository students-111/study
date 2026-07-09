/**
 * @file dal_pid.h
 * @brief PID 控制器实例统一管理接口。
 */

#ifndef DAL_PID_H
#define DAL_PID_H

/* ======== include ======== */

#include "PID.h"

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
