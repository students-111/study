/**
 * @file dal_pid.c
 * @brief PID 控制器实例统一管理实现。
 */

/* ======== include ======== */

#include "dal_pid.h"

/* ======== 全局实例 ======== */

PID_TypeDef pid_gather[DAL_PID_ID_COUNT];

/* ======== 内部函数 ======== */

/**
 * @brief 初始化单个 PID 控制器实例。
 * @param pid PID 控制器实例。
 * @param kp 比例系数。
 * @param ki 积分系数。
 * @param kd 微分系数。
 * @param output_limit 输出正负限幅。
 * @param iterm_limit 积分项正负限幅。
 * @return 无。
 */
static void dal_pid_config_one(PID_TypeDef *pid, float kp, float ki, float kd,
    float output_limit, float iterm_limit)
{
    PID_InitTypeDef init = {
        .Kp = kp,
        .Ki = ki,
        .Kd = kd,
        .Setpoint = 0.0f,
        .OutputUpperLimit = output_limit,
        .OutputLowerLimit = -output_limit,
        .ITermUpperLimit = iterm_limit,
        .ITermLowerLimit = -iterm_limit,
        .DefaultOutput = 0.0f,
    };

    PID_Init(pid, &init);
}

/* ======== 公开 API ======== */

void dal_pid_init(void)
{
    /* 左轮速度内环：Kp, Ki, Kd, 输出限幅, 积分限幅。 */
    dal_pid_config_one(&pid_gather[DAL_PID_ID_SPEED_LEFT],
        8.0f, 0.8f, 0.0f, 1000.0f, 600.0f);

    /* 右轮速度内环：Kp, Ki, Kd, 输出限幅, 积分限幅。 */
    dal_pid_config_one(&pid_gather[DAL_PID_ID_SPEED_RIGHT],
        8.0f, 0.8f, 0.00f, 1000.0f, 600.0f);

    /* 灰度循迹外环：Kp, Ki, Kd, 输出限幅, 积分限幅。 */
    dal_pid_config_one(&pid_gather[DAL_PID_ID_LINE],
        DAL_PID_LINE_KP, DAL_PID_LINE_KI, DAL_PID_LINE_KD,
        DAL_PID_LINE_OUTPUT_LIMIT, DAL_PID_LINE_ITERM_LIMIT);

    /* 直线角度外环：Kp, Ki, Kd, 输出限幅, 积分限幅。 */
    dal_pid_config_one(&pid_gather[DAL_PID_ID_STRAIGHT_ANGLE],
        0.000125f, 0.0f, 0.0f, 4.0f, 0.0f);
}
