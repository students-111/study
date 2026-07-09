/**
 * @file dal_mpu6050.c
 * @brief MPU6050 姿态传感器 DAL 实现。
 */

/* ======== include ======== */

#include "dal_mpu6050.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#include "bsp_i2c.h"
#include "bsp_time.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

typedef enum {
    DAL_MPU6050_OK = 0,            /**< 操作完成。 */
    DAL_MPU6050_BUSY,              /**< I2C 或状态机正忙。 */
    DAL_MPU6050_NOT_READY,         /**< 设备尚未完成配置。 */
    DAL_MPU6050_ERROR              /**< 设备探测或事务失败。 */
} dal_mpu6050_status_e;

typedef enum {
    DAL_MPU6050_STATE_IDLE = 0,            /**< 状态机空闲。 */
    DAL_MPU6050_STATE_PROBE_START,         /**< 准备发起 WHO_AM_I 探测。 */
    DAL_MPU6050_STATE_PROBE_WAIT,          /**< 等待 WHO_AM_I 探测完成。 */
    DAL_MPU6050_STATE_CONFIG_START,        /**< 准备写入配置寄存器。 */
    DAL_MPU6050_STATE_CONFIG_WAIT,         /**< 等待配置写入完成。 */
    DAL_MPU6050_STATE_READY,               /**< 配置完成，可发起采样。 */
    DAL_MPU6050_STATE_SAMPLE_WAIT,         /**< 等待采样帧读取完成。 */
    DAL_MPU6050_STATE_ERROR                /**< 状态机进入错误态。 */
} dal_mpu6050_state_e;

typedef struct {
    uint8_t reg;                   /**< 待写寄存器地址。 */
    uint8_t val;                   /**< 待写寄存器值。 */
} dal_mpu6050_reg_cfg_t;

typedef struct {
    double q_angle;                /**< 角度过程噪声。 */
    double q_bias;                 /**< 零偏过程噪声。 */
    double r_measure;              /**< 测量噪声。 */
    double angle;                  /**< 当前角度估计。 */
    double bias;                   /**< 当前零偏估计。 */
    double p[DAL_MPU6050_KALMAN_DIM][DAL_MPU6050_KALMAN_DIM]; /**< 误差协方差矩阵。 */
} dal_mpu6050_kalman_t;

/* ======== 全局实例 ======== */

dal_mpu6050_sample_t g_dal_mpu6050_sample;

/* ======== 内部变量 ======== */

static const dal_mpu6050_reg_cfg_t g_dal_mpu6050_cfg[DAL_MPU6050_CFG_COUNT] = {
    {DAL_MPU6050_REG_PWR_MGMT_1, DAL_MPU6050_PWR_WAKE_VALUE},
    {DAL_MPU6050_REG_SMPLRT_DIV, DAL_MPU6050_SMPLRT_DIV_VALUE},
    {DAL_MPU6050_REG_ACCEL_CONFIG, DAL_MPU6050_ACCEL_CONFIG_VALUE},
    {DAL_MPU6050_REG_GYRO_CONFIG, DAL_MPU6050_GYRO_CONFIG_VALUE}
};

static dal_mpu6050_state_e g_dal_mpu6050_state =
    DAL_MPU6050_STATE_IDLE;
static uint8_t g_dal_mpu6050_addr = DAL_MPU6050_ADDR_LOW;
static uint8_t g_dal_mpu6050_who;
static uint8_t g_dal_mpu6050_frame[DAL_MPU6050_FRAME_LEN];
static uint8_t g_dal_mpu6050_cfg_idx;
static bool g_dal_mpu6050_has_sample = false;
static uint32_t g_dal_mpu6050_last_ms;
static uint32_t g_dal_mpu6050_sample_due_ms;
static uint32_t g_dal_mpu6050_error_retry_ms;
static int64_t g_dal_mpu6050_yaw_mdeg;
static int32_t g_dal_mpu6050_gyro_z_bias_mdps;
static uint8_t g_dal_mpu6050_yaw_bias_skip_count;
static dal_mpu6050_kalman_t g_dal_mpu6050_kalman_x;
static dal_mpu6050_kalman_t g_dal_mpu6050_kalman_y;

/* ======== 内部函数声明 ======== */

/**
 * @brief 启动 MPU6050 探测与配置状态机。
 * @param void 无参数。
 * @return 启动结果。
 */
static dal_mpu6050_status_e dal_mpu6050_init_start(void);

/**
 * @brief 推进 MPU6050 异步状态机。
 * @param now_ms 当前毫秒计数。
 * @return 无。
 */
static void dal_mpu6050_process(uint32_t now_ms);

/**
 * @brief 向 MPU6050 发起一次异步采样帧读取。
 * @param void 无参数。
 * @return 请求结果。
 */
static dal_mpu6050_status_e dal_mpu6050_request_sample(void);

/**
 * @brief 判断 MPU6050 是否已可接受采样调度。
 * @param void 无参数。
 * @return 可采样或正在采样返回 true，否则返回 false。
 */
static bool dal_mpu6050_is_ready(void);

/**
 * @brief 从错误态重新启动 MPU6050 地址探测流程。
 * @param void 无参数。
 * @return 无。
 */
static void dal_mpu6050_retry_probe(void);

/* ======== 内部函数 ======== */

/**
 * @brief 复位 Kalman 滤波器状态。
 * @param kalman Kalman 滤波器实例。
 * @return 无。
 */
static void dal_mpu6050_kalman_reset(dal_mpu6050_kalman_t *kalman)
{
    kalman->q_angle = DAL_MPU6050_KALMAN_Q_ANGLE;
    kalman->q_bias = DAL_MPU6050_KALMAN_Q_BIAS;
    kalman->r_measure = DAL_MPU6050_KALMAN_R_MEASURE;
    kalman->angle = 0.0;
    kalman->bias = 0.0;
    kalman->p[0][0] = 0.0;
    kalman->p[0][1] = 0.0;
    kalman->p[1][0] = 0.0;
    kalman->p[1][1] = 0.0;
}

/**
 * @brief 根据新角度和角速度更新 Kalman 角度估计。
 * @param kalman Kalman 滤波器实例。
 * @param new_angle 加速度解算角度。
 * @param new_rate 陀螺仪角速度。
 * @param dt 采样间隔，单位 s。
 * @return 更新后的角度估计。
 */
static double dal_mpu6050_kalman_get_angle(
    dal_mpu6050_kalman_t *kalman, double new_angle, double new_rate, double dt)
{
    double rate = new_rate - kalman->bias;
    double s;
    double k[DAL_MPU6050_KALMAN_DIM];
    double y;
    double p00_temp;
    double p01_temp;

    kalman->angle += dt * rate;

    kalman->p[0][0] += dt * (dt * kalman->p[1][1] -
        kalman->p[0][1] - kalman->p[1][0] + kalman->q_angle);
    kalman->p[0][1] -= dt * kalman->p[1][1];
    kalman->p[1][0] -= dt * kalman->p[1][1];
    kalman->p[1][1] += kalman->q_bias * dt;

    s = kalman->p[0][0] + kalman->r_measure;
    k[0] = kalman->p[0][0] / s;
    k[1] = kalman->p[1][0] / s;

    y = new_angle - kalman->angle;
    kalman->angle += k[0] * y;
    kalman->bias += k[1] * y;

    p00_temp = kalman->p[0][0];
    p01_temp = kalman->p[0][1];

    kalman->p[0][0] -= k[0] * p00_temp;
    kalman->p[0][1] -= k[0] * p01_temp;
    kalman->p[1][0] -= k[1] * p00_temp;
    kalman->p[1][1] -= k[1] * p01_temp;

    return kalman->angle;
}

/**
 * @brief 将大端 16 位寄存器数据转换为 int16_t。
 * @param buf 指向高字节的缓冲区。
 * @return 转换后的有符号 16 位数值。
 */
static int16_t dal_mpu6050_be16(const uint8_t *buf)
{
    return (int16_t)(((uint16_t)buf[0] << DAL_MPU6050_HIGH_BYTE_SHIFT) |
        buf[1]);
}

/**
 * @brief 将 double 工程量转换为 0.001 单位整数。
 * @param val 待转换工程量。
 * @return 0.001 单位整数。
 */
static int32_t dal_mpu6050_double_to_milli(double val)
{
    return (int32_t)((val >= 0.0) ?
        (val * (double)DAL_MPU6050_MILLI_SCALE + DAL_MPU6050_ROUND_OFFSET) :
        (val * (double)DAL_MPU6050_MILLI_SCALE - DAL_MPU6050_ROUND_OFFSET));
}

/**
 * @brief 将 Yaw 角归一化到正负半圈范围内。
 * @param yaw_mdeg 待归一化的 Yaw 角，单位 0.001 度。
 * @return 归一化后的 Yaw 角，单位 0.001 度。
 */
static int64_t dal_mpu6050_normalize_yaw(int64_t yaw_mdeg)
{
    while (yaw_mdeg >= (int64_t)DAL_MPU6050_YAW_HALF_TURN_MDEG) {
        yaw_mdeg -= (int64_t)DAL_MPU6050_YAW_FULL_TURN_MDEG;
    }
    while (yaw_mdeg < -(int64_t)DAL_MPU6050_YAW_HALF_TURN_MDEG) {
        yaw_mdeg += (int64_t)DAL_MPU6050_YAW_FULL_TURN_MDEG;
    }
    return yaw_mdeg;
}

/**
 * @brief 根据 Z 轴角速度更新相对 Yaw 角。
 * @param dt 当前采样间隔，单位 s。
 * @return 无。
 */
static void dal_mpu6050_solve_yaw(double dt)
{
    int32_t gyro_z_mdps =
        g_dal_mpu6050_sample.gyro_mdps[DAL_MPU6050_AXIS_Z];

    if (!g_dal_mpu6050_sample.yaw_calibrated) {
        if (g_dal_mpu6050_yaw_bias_skip_count <
            DAL_MPU6050_YAW_BIAS_SKIP_SAMPLES) {
            g_dal_mpu6050_yaw_bias_skip_count++;
            g_dal_mpu6050_yaw_mdeg = 0;
            g_dal_mpu6050_sample.yaw_mdeg = 0;
            return;
        }
        g_dal_mpu6050_gyro_z_bias_mdps = gyro_z_mdps;
        g_dal_mpu6050_yaw_mdeg = 0;
        g_dal_mpu6050_sample.yaw_mdeg = 0;
        g_dal_mpu6050_sample.yaw_calibrated = true;
        return;
    }

    g_dal_mpu6050_yaw_mdeg += (int64_t)(
        ((double)(gyro_z_mdps - g_dal_mpu6050_gyro_z_bias_mdps) * dt) +
        ((gyro_z_mdps >= g_dal_mpu6050_gyro_z_bias_mdps) ?
            DAL_MPU6050_ROUND_OFFSET : -DAL_MPU6050_ROUND_OFFSET));
    g_dal_mpu6050_yaw_mdeg =
        dal_mpu6050_normalize_yaw(g_dal_mpu6050_yaw_mdeg);
    g_dal_mpu6050_sample.yaw_mdeg = (int32_t)g_dal_mpu6050_yaw_mdeg;
}

/**
 * @brief 根据最新原始数据解算 pitch/roll 并更新 Kalman 滤波器。
 * @param now_ms 当前毫秒计数。
 * @return 无。
 */
static void dal_mpu6050_solve_kalman(uint32_t now_ms)
{
    double dt = DAL_MPU6050_DEFAULT_DT_S;
    double roll;
    double roll_sqrt;
    double pitch;
    double gx_dps = (double)g_dal_mpu6050_sample.gyro_mdps[0] /
        (double)DAL_MPU6050_MILLI_SCALE;
    double gy_dps = (double)g_dal_mpu6050_sample.gyro_mdps[1] /
        (double)DAL_MPU6050_MILLI_SCALE;

    if (g_dal_mpu6050_last_ms != 0U) {
        dt = (double)(uint32_t)(now_ms - g_dal_mpu6050_last_ms) /
            (double)DAL_MPU6050_MILLI_SCALE;
        if (dt <= 0.0) {
            dt = DAL_MPU6050_DEFAULT_DT_S;
        }
    }
    g_dal_mpu6050_last_ms = now_ms;

    roll_sqrt = sqrt(
        (double)g_dal_mpu6050_sample.accel_raw[0] *
        (double)g_dal_mpu6050_sample.accel_raw[0] +
        (double)g_dal_mpu6050_sample.accel_raw[DAL_MPU6050_AXIS_Z] *
        (double)g_dal_mpu6050_sample.accel_raw[DAL_MPU6050_AXIS_Z]);

    roll = (roll_sqrt != 0.0) ?
        atan((double)g_dal_mpu6050_sample.accel_raw[1] / roll_sqrt) *
            DAL_MPU6050_RAD_TO_DEG : 0.0;
    pitch = atan2(-(double)g_dal_mpu6050_sample.accel_raw[0],
        (double)g_dal_mpu6050_sample.accel_raw[DAL_MPU6050_AXIS_Z]) *
        DAL_MPU6050_RAD_TO_DEG;

    dal_mpu6050_solve_yaw(dt);

    if (!g_dal_mpu6050_has_sample) {
        g_dal_mpu6050_kalman_x.angle = roll;
        g_dal_mpu6050_kalman_y.angle = pitch;
        g_dal_mpu6050_sample.roll_mdeg = dal_mpu6050_double_to_milli(roll);
        g_dal_mpu6050_sample.pitch_mdeg = dal_mpu6050_double_to_milli(pitch);
        return;
    }

    if (((pitch < -DAL_MPU6050_CROSS_LIMIT_DEG) &&
         (g_dal_mpu6050_kalman_y.angle > DAL_MPU6050_CROSS_LIMIT_DEG)) ||
        ((pitch > DAL_MPU6050_CROSS_LIMIT_DEG) &&
         (g_dal_mpu6050_kalman_y.angle < -DAL_MPU6050_CROSS_LIMIT_DEG))) {
        g_dal_mpu6050_kalman_y.angle = pitch;
        g_dal_mpu6050_sample.pitch_mdeg = dal_mpu6050_double_to_milli(pitch);
    } else {
        g_dal_mpu6050_sample.pitch_mdeg = dal_mpu6050_double_to_milli(
            dal_mpu6050_kalman_get_angle(&g_dal_mpu6050_kalman_y,
                pitch, gy_dps, dt));
    }

    /*
     * WHY: MPU6050_STD 算法约定 pitch 超过 90 度时 X 轴角速度反向。
     */
    if (fabs((double)g_dal_mpu6050_sample.pitch_mdeg /
        (double)DAL_MPU6050_MILLI_SCALE) > DAL_MPU6050_CROSS_LIMIT_DEG) {
        gx_dps = -gx_dps;
    }

    g_dal_mpu6050_sample.roll_mdeg = dal_mpu6050_double_to_milli(
        dal_mpu6050_kalman_get_angle(&g_dal_mpu6050_kalman_x,
            roll, gx_dps, dt));
}

/**
 * @brief 解码 MPU6050 原始数据帧并更新公开样本。
 * @param now_ms 当前毫秒计数。
 * @return 无。
 */
static void dal_mpu6050_decode_frame(uint32_t now_ms)
{
    uint8_t idx;

    for (idx = 0U; idx < DAL_MPU6050_AXIS_COUNT; idx++) {
        g_dal_mpu6050_sample.accel_raw[idx] =
            dal_mpu6050_be16(&g_dal_mpu6050_frame[
                idx * DAL_MPU6050_WORD_BYTES]);
        g_dal_mpu6050_sample.gyro_raw[idx] =
            dal_mpu6050_be16(&g_dal_mpu6050_frame[
                DAL_MPU6050_GYRO_FRAME_OFFSET +
                idx * DAL_MPU6050_WORD_BYTES]);
        g_dal_mpu6050_sample.accel_mg[idx] =
            ((int32_t)g_dal_mpu6050_sample.accel_raw[idx] *
             DAL_MPU6050_MILLI_SCALE) / DAL_MPU6050_ACCEL_LSB_PER_G;
        g_dal_mpu6050_sample.gyro_mdps[idx] =
            ((int32_t)g_dal_mpu6050_sample.gyro_raw[idx] *
             DAL_MPU6050_MILLI_SCALE) / DAL_MPU6050_GYRO_LSB_PER_DPS;
    }

    g_dal_mpu6050_sample.accel_mg[DAL_MPU6050_AXIS_Z] =
        dal_mpu6050_double_to_milli(
            (double)g_dal_mpu6050_sample.accel_raw[DAL_MPU6050_AXIS_Z] /
            DAL_MPU6050_ACCEL_Z_CORRECTOR);
    g_dal_mpu6050_sample.temp_raw = dal_mpu6050_be16(
        &g_dal_mpu6050_frame[DAL_MPU6050_TEMP_FRAME_OFFSET]);
    g_dal_mpu6050_sample.temp_mc = DAL_MPU6050_TEMP_OFFSET_MC +
        ((int32_t)g_dal_mpu6050_sample.temp_raw *
         DAL_MPU6050_MILLI_SCALE) / DAL_MPU6050_TEMP_SLOPE_DEN;
    g_dal_mpu6050_sample.address = g_dal_mpu6050_addr;

    dal_mpu6050_solve_kalman(now_ms);

    g_dal_mpu6050_sample.sequence++;
    g_dal_mpu6050_has_sample = true;
}

/**
 * @brief 启动 MPU6050 探测与配置状态机。
 * @param void 无参数。
 * @return 启动结果。
 */
static dal_mpu6050_status_e dal_mpu6050_init_start(void)
{
    if (g_dal_mpu6050_state != DAL_MPU6050_STATE_IDLE) {
        return DAL_MPU6050_BUSY;
    }

    g_dal_mpu6050_addr = DAL_MPU6050_ADDR_LOW;
    g_dal_mpu6050_cfg_idx = 0U;
    g_dal_mpu6050_has_sample = false;
    g_dal_mpu6050_last_ms = 0U;
    g_dal_mpu6050_yaw_mdeg = 0;
    g_dal_mpu6050_error_retry_ms = 0U;
    g_dal_mpu6050_gyro_z_bias_mdps = 0;
    g_dal_mpu6050_yaw_bias_skip_count = 0U;
    g_dal_mpu6050_sample.yaw_mdeg = 0;
    g_dal_mpu6050_sample.yaw_calibrated = false;
    g_dal_mpu6050_sample.sequence = 0U;
    dal_mpu6050_kalman_reset(&g_dal_mpu6050_kalman_x);
    dal_mpu6050_kalman_reset(&g_dal_mpu6050_kalman_y);
    g_dal_mpu6050_state = DAL_MPU6050_STATE_PROBE_START;
    return DAL_MPU6050_OK;
}

/**
 * @brief 推进 MPU6050 WHO_AM_I 探测流程。
 * @param void 无参数。
 * @return 无。
 */
static void dal_mpu6050_process_probe(void)
{
    bsp_status_e status;

    if (g_dal_mpu6050_state == DAL_MPU6050_STATE_PROBE_START) {
        status = bsp_i2c_mem_read_async(g_dal_mpu6050_addr,
            DAL_MPU6050_REG_WHO_AM_I, &g_dal_mpu6050_who, 1U);
        if (status == BSP_OK) {
            g_dal_mpu6050_state = DAL_MPU6050_STATE_PROBE_WAIT;
        }
        return;
    }

    status = bsp_i2c_poll();
    if (status == BSP_ERR_BUSY) {
        return;
    }
    if ((status == BSP_OK) && (g_dal_mpu6050_who == DAL_MPU6050_WHO_VALUE)) {
        g_dal_mpu6050_state = DAL_MPU6050_STATE_CONFIG_START;
    } else if (g_dal_mpu6050_addr == DAL_MPU6050_ADDR_LOW) {
        g_dal_mpu6050_addr = DAL_MPU6050_ADDR_HIGH;
        g_dal_mpu6050_state = DAL_MPU6050_STATE_PROBE_START;
    } else {
        g_dal_mpu6050_state = DAL_MPU6050_STATE_ERROR;
    }
}

/**
 * @brief 推进 MPU6050 寄存器配置流程。
 * @param void 无参数。
 * @return 无。
 */
static void dal_mpu6050_process_config(void)
{
    bsp_status_e status;
    const dal_mpu6050_reg_cfg_t *cfg =
        &g_dal_mpu6050_cfg[g_dal_mpu6050_cfg_idx];

    if (g_dal_mpu6050_state == DAL_MPU6050_STATE_CONFIG_START) {
        status = bsp_i2c_mem_write_async(g_dal_mpu6050_addr,
            cfg->reg, &cfg->val, 1U);
        if (status == BSP_OK) {
            g_dal_mpu6050_state = DAL_MPU6050_STATE_CONFIG_WAIT;
        }
        return;
    }

    status = bsp_i2c_poll();
    if (status == BSP_ERR_BUSY) {
        return;
    }
    if (status != BSP_OK) {
        g_dal_mpu6050_state = DAL_MPU6050_STATE_ERROR;
        return;
    }

    g_dal_mpu6050_cfg_idx++;
    g_dal_mpu6050_state =
        (g_dal_mpu6050_cfg_idx < DAL_MPU6050_CFG_COUNT) ?
        DAL_MPU6050_STATE_CONFIG_START : DAL_MPU6050_STATE_READY;
}

/**
 * @brief 推进 MPU6050 异步状态机。
 * @param now_ms 当前毫秒计数。
 * @return 无。
 */
static void dal_mpu6050_process(uint32_t now_ms)
{
    bsp_status_e status;

    if ((g_dal_mpu6050_state == DAL_MPU6050_STATE_PROBE_START) ||
        (g_dal_mpu6050_state == DAL_MPU6050_STATE_PROBE_WAIT)) {
        dal_mpu6050_process_probe();
    } else if ((g_dal_mpu6050_state == DAL_MPU6050_STATE_CONFIG_START) ||
               (g_dal_mpu6050_state == DAL_MPU6050_STATE_CONFIG_WAIT)) {
        dal_mpu6050_process_config();
    } else if (g_dal_mpu6050_state == DAL_MPU6050_STATE_SAMPLE_WAIT) {
        status = bsp_i2c_poll();
        if (status == BSP_OK) {
            dal_mpu6050_decode_frame(now_ms);
            g_dal_mpu6050_state = DAL_MPU6050_STATE_READY;
        } else if (status != BSP_ERR_BUSY) {
            g_dal_mpu6050_state = DAL_MPU6050_STATE_ERROR;
        }
    }
}

/**
 * @brief 向 MPU6050 发起一次异步采样帧读取。
 * @param void 无参数。
 * @return 请求结果。
 */
static dal_mpu6050_status_e dal_mpu6050_request_sample(void)
{
    bsp_status_e status;

    if (g_dal_mpu6050_state == DAL_MPU6050_STATE_SAMPLE_WAIT) {
        return DAL_MPU6050_BUSY;
    }
    if (g_dal_mpu6050_state != DAL_MPU6050_STATE_READY) {
        return DAL_MPU6050_NOT_READY;
    }

    status = bsp_i2c_mem_read_async(g_dal_mpu6050_addr,
        DAL_MPU6050_REG_ACCEL_XOUT_H, g_dal_mpu6050_frame,
        DAL_MPU6050_FRAME_LEN);
    if (status != BSP_OK) {
        return (status == BSP_ERR_BUSY) ? DAL_MPU6050_BUSY :
            DAL_MPU6050_ERROR;
    }

    g_dal_mpu6050_state = DAL_MPU6050_STATE_SAMPLE_WAIT;
    return DAL_MPU6050_OK;
}

/**
 * @brief 判断 MPU6050 是否已可接受采样调度。
 * @param void 无参数。
 * @return 可采样或正在采样返回 true，否则返回 false。
 */
static bool dal_mpu6050_is_ready(void)
{
    return (g_dal_mpu6050_state == DAL_MPU6050_STATE_READY) ||
        (g_dal_mpu6050_state == DAL_MPU6050_STATE_SAMPLE_WAIT);
}

/**
 * @brief 从错误态重新启动 MPU6050 地址探测流程。
 * @param void 无参数。
 * @return 无。
 */
static void dal_mpu6050_retry_probe(void)
{
    g_dal_mpu6050_addr = DAL_MPU6050_ADDR_LOW;
    g_dal_mpu6050_who = 0U;
    g_dal_mpu6050_cfg_idx = 0U;
    g_dal_mpu6050_state = DAL_MPU6050_STATE_PROBE_START;
}

/* ======== 公开 API ======== */

void dal_mpu6050_init(void)
{
    uint32_t now_ms = bsp_time_get_ms();

    g_dal_mpu6050_sample_due_ms = now_ms;
    (void)dal_mpu6050_init_start();
}

void dal_mpu6050_refresh(void)
{
    uint32_t now_ms = bsp_time_get_ms();

    dal_mpu6050_process(now_ms);

    if ((g_dal_mpu6050_state == DAL_MPU6050_STATE_ERROR) &&
        ((int32_t)(now_ms - g_dal_mpu6050_error_retry_ms) >= 0)) {
        g_dal_mpu6050_error_retry_ms = now_ms +
            DAL_MPU6050_ERROR_RETRY_MS;
        dal_mpu6050_retry_probe();
    }

    if ((int32_t)(now_ms - g_dal_mpu6050_sample_due_ms) >= 0) {
        g_dal_mpu6050_sample_due_ms += DAL_MPU6050_SAMPLE_PERIOD_MS;
        if (dal_mpu6050_is_ready()) {
            (void)dal_mpu6050_request_sample();
        }
    }
}
