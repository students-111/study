/**
 * @file dal_mpu6050.h
 * @brief MPU6050 姿态传感器 DAL 接口。
 */

#ifndef DAL_MPU6050_H
#define DAL_MPU6050_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* AD0=0 时 MPU6050 的 7 位 I2C 地址。 */
#define DAL_MPU6050_ADDR_LOW                   (0x68U)

/* AD0=1 时 MPU6050 的 7 位 I2C 地址。 */
#define DAL_MPU6050_ADDR_HIGH                  (0x69U)

/* WHO_AM_I 寄存器地址。 */
#define DAL_MPU6050_REG_WHO_AM_I               (0x75U)

/* WHO_AM_I 期望返回值。 */
#define DAL_MPU6050_WHO_VALUE                  (0x68U)

/* 加速度 X 高字节起始寄存器地址。 */
#define DAL_MPU6050_REG_ACCEL_XOUT_H           (0x3BU)

/* 电源管理寄存器地址。 */
#define DAL_MPU6050_REG_PWR_MGMT_1             (0x6BU)

/* 采样率分频寄存器地址。 */
#define DAL_MPU6050_REG_SMPLRT_DIV             (0x19U)

/* 加速度量程配置寄存器地址。 */
#define DAL_MPU6050_REG_ACCEL_CONFIG           (0x1CU)

/* 陀螺仪量程配置寄存器地址。 */
#define DAL_MPU6050_REG_GYRO_CONFIG            (0x1BU)

/* 唤醒设备配置值。 */
#define DAL_MPU6050_PWR_WAKE_VALUE             (0x00U)

/* 采样率分频配置值，配合内部 1 kHz 得到 125 Hz。 */
#define DAL_MPU6050_SMPLRT_DIV_VALUE           (0x07U)

/* 加速度 +-2g 量程配置值。 */
#define DAL_MPU6050_ACCEL_CONFIG_VALUE         (0x00U)

/* 陀螺仪 +-250 dps 量程配置值。 */
#define DAL_MPU6050_GYRO_CONFIG_VALUE          (0x00U)

/* 原始数据帧长度，单位字节；包含 Accel XYZ、Temp、Gyro XYZ。 */
#define DAL_MPU6050_FRAME_LEN                  (14U)

/* MPU6050 采样周期，单位 ms。 */
#define DAL_MPU6050_SAMPLE_PERIOD_MS           (10U)

/* MPU6050 探测或事务失败后的重试间隔，单位 ms；用于上电阶段传感器尚未稳定时自恢复。 */
#define DAL_MPU6050_ERROR_RETRY_MS             (100U)

/* 三轴向量数量。 */
#define DAL_MPU6050_AXIS_COUNT                 (3U)

/* 配置表项目数量；由电源、采样率、加速度量程、陀螺仪量程四项组成。 */
#define DAL_MPU6050_CFG_COUNT                  (4U)

/* Kalman 协方差矩阵维度。 */
#define DAL_MPU6050_KALMAN_DIM                 (2U)

/* 原始帧中陀螺仪数据起始偏移。 */
#define DAL_MPU6050_GYRO_FRAME_OFFSET          (8U)

/* 原始帧中温度数据起始偏移。 */
#define DAL_MPU6050_TEMP_FRAME_OFFSET          (6U)

/* 16 位寄存器高低字节跨度。 */
#define DAL_MPU6050_WORD_BYTES                 (2U)

/* 大端 16 位寄存器高字节左移位数。 */
#define DAL_MPU6050_HIGH_BYTE_SHIFT            (8U)

/* Z 轴下标。 */
#define DAL_MPU6050_AXIS_Z                     (2U)

/* 弧度转角度系数。 */
#define DAL_MPU6050_RAD_TO_DEG                 (57.295779513082320876798154814105)

/* MPU6050_STD 中使用的 Z 轴加速度修正系数。 */
#define DAL_MPU6050_ACCEL_Z_CORRECTOR          (14418.0)

/* +-2g 量程下加速度 LSB/g。 */
#define DAL_MPU6050_ACCEL_LSB_PER_G            (16384L)

/* +-250 dps 量程下陀螺仪 LSB/dps。 */
#define DAL_MPU6050_GYRO_LSB_PER_DPS           (131L)

/* 输出整数单位换算比例；单位表示 0.001 工程单位。 */
#define DAL_MPU6050_MILLI_SCALE                (1000L)

/* 浮点数转换为 0.001 单位整数时的四舍五入补偿。 */
#define DAL_MPU6050_ROUND_OFFSET               (0.5)

/* 温度公式偏移，单位 0.001 摄氏度。 */
#define DAL_MPU6050_TEMP_OFFSET_MC             (36530L)

/* 温度公式斜率分母，来自 MPU6050 数据手册。 */
#define DAL_MPU6050_TEMP_SLOPE_DEN             (340L)

/* 默认 Kalman dt，单位 s；首帧或异常时间差时使用。 */
#define DAL_MPU6050_DEFAULT_DT_S               (0.01)

/* 姿态跨越保护角度，单位度。 */
#define DAL_MPU6050_CROSS_LIMIT_DEG            (90.0)

/* Kalman 角度过程噪声。 */
#define DAL_MPU6050_KALMAN_Q_ANGLE             (0.001)

/* Kalman 零偏过程噪声。 */
#define DAL_MPU6050_KALMAN_Q_BIAS              (0.003)

/* Kalman 测量噪声。 */
#define DAL_MPU6050_KALMAN_R_MEASURE           (0.03)

/* Yaw 零偏锁定前跳过的启动样本数；用于避开 MPU6050 刚唤醒后的首帧瞬态。 */
#define DAL_MPU6050_YAW_BIAS_SKIP_SAMPLES      (2U)

/* Yaw 半圈角度，单位 0.001 度；用于归一化相对航向。 */
#define DAL_MPU6050_YAW_HALF_TURN_MDEG         (180000L)

/* Yaw 一圈角度，单位 0.001 度；用于归一化相对航向。 */
#define DAL_MPU6050_YAW_FULL_TURN_MDEG         (360000L)

/* ======== 类型定义 ======== */

/**
 * @brief MPU6050 最新采样快照。
 */
typedef struct {
    int16_t accel_raw[DAL_MPU6050_AXIS_COUNT];    /**< 加速度原始 XYZ。 */
    int16_t gyro_raw[DAL_MPU6050_AXIS_COUNT];     /**< 陀螺仪原始 XYZ。 */
    int16_t temp_raw;                             /**< 温度原始寄存器值。 */
    int32_t accel_mg[DAL_MPU6050_AXIS_COUNT];     /**< 加速度，单位 mg。 */
    int32_t gyro_mdps[DAL_MPU6050_AXIS_COUNT];    /**< 角速度，单位 mdps。 */
    int32_t temp_mc;                              /**< 温度，单位 0.001 摄氏度。 */
    int32_t pitch_mdeg;                           /**< 俯仰角，单位 0.001 度。 */
    int32_t roll_mdeg;                            /**< 横滚角，单位 0.001 度。 */
    int32_t yaw_mdeg;                             /**< 相对航向角，单位 0.001 度。 */
    bool yaw_calibrated;                          /**< Yaw 零偏是否已经完成校准。 */
    uint32_t sequence;                            /**< 每次解码递增的样本序号。 */
    uint8_t address;                              /**< 实际探测到的 7 位 I2C 地址。 */
} dal_mpu6050_sample_t;

/* ======== 全局实例 ======== */

extern dal_mpu6050_sample_t g_dal_mpu6050_sample;

/* ======== 公开 API ======== */

/**
 * @brief 初始化 MPU6050 异步探测和配置状态机。
 * @param void 无参数。
 * @return 无。
 */
void dal_mpu6050_init(void);

/**
 * @brief 推进 MPU6050 状态机并按采样周期发起读帧。
 * @param void 无参数。
 * @return 无。
 */
void dal_mpu6050_refresh(void);

#endif /* DAL_MPU6050_H */
