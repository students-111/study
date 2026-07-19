/**
 * @file dal_jy901p.h
 * @brief JY901P 姿态角度快照 DAL 接口。
 */

#ifndef DAL_JY901P_H
#define DAL_JY901P_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* JY901P 的 7 位 I2C 地址；与官方 JY61P 例程的 IIC_ADDR 保持一致。 */
#define DAL_JY901P_I2C_ADDRESS                       (0x50U)

/* JY901P Roll 低字节寄存器地址；连续读取可取得 Roll、Pitch、Yaw。 */
#define DAL_JY901P_REG_ROLL_L                        (0x3DU)

/* JY901P 配置解锁寄存器地址。 */
#define DAL_JY901P_REG_UNLOCK                        (0x69U)

/* JY901P 角度参考操作寄存器地址。 */
#define DAL_JY901P_REG_ANGLE_REFERENCE               (0x01U)

/* JY901P 配置保存寄存器地址。 */
#define DAL_JY901P_REG_SAVE                           (0x00U)

/* 角度帧长度，单位字节；依次为 Roll、Pitch、Yaw 的小端 16 位值。 */
#define DAL_JY901P_ANGLE_FRAME_LEN                    (6U)

/* 角度帧中 Roll 的字节偏移。 */
#define DAL_JY901P_FRAME_ROLL_OFFSET                  (0U)

/* 角度帧中 Pitch 的字节偏移。 */
#define DAL_JY901P_FRAME_PITCH_OFFSET                 (2U)

/* 角度帧中 Yaw 的字节偏移。 */
#define DAL_JY901P_FRAME_YAW_OFFSET                   (4U)

/* 小端 16 位数据高字节的左移位数。 */
#define DAL_JY901P_HIGH_BYTE_SHIFT                    (8U)

/* 模块角度原始数据的满量程分母，单位 LSB，对应正负 180 度。 */
#define DAL_JY901P_ANGLE_RAW_FULL_SCALE               (32768L)

/* Yaw 半圈原始角度，单位 LSB；供 APP 计算最短角差。 */
#define DAL_JY901P_YAW_HALF_TURN_RAW                 (32768L)

/* Yaw 一圈原始角度，单位 LSB；供 APP 计算最短角差。 */
#define DAL_JY901P_YAW_FULL_TURN_RAW                 (65536L)

/* 角度采样周期，单位 ms；必须不快于模块输出刷新速度。 */
#define DAL_JY901P_SAMPLE_PERIOD_MS                   (10U)

/* I2C 事务或模块配置失败后的重试间隔，单位 ms。 */
#define DAL_JY901P_ERROR_RETRY_MS                     (100U)
/* 最近有效角度快照允许的最长保持时间，单位 ms；短暂 I2C 异常不应立即打断直行控制。 */
#define DAL_JY901P_SNAPSHOT_STALE_TIMEOUT_MS         (200U)

/* 相邻有效 Yaw 帧允许的最大变化量，单位原始角度 LSB；超过该值视为异常帧而丢弃。 */
#define DAL_JY901P_YAW_MAX_STEP_RAW                   (3641L)

/* 是否在启动时按官方例程执行 Z 轴和角度参考归零；默认 0 直接读取模块已解算角度，只有明确需要重置模块参考时才设为 1。 */
#define DAL_JY901P_REFERENCE_RESET_ENABLE             (0U)

/* 官方参考归零命令写入的数据长度，单位字节。 */
#define DAL_JY901P_COMMAND_DATA_LEN                   (2U)

/* 官方参考归零流程包含的 I2C 写命令数量。 */
#define DAL_JY901P_CONFIGURATION_COMMAND_COUNT        (6U)

/* 每条参考归零命令后的模块稳定时间，单位 ms；与官方例程一致。 */
#define DAL_JY901P_COMMAND_SETTLE_MS                  (200U)

/* 官方例程使用的解锁命令第一个数据字节。 */
#define DAL_JY901P_UNLOCK_DATA_LOW                    (0x88U)

/* 官方例程使用的解锁命令第二个数据字节。 */
#define DAL_JY901P_UNLOCK_DATA_HIGH                   (0xB5U)

/* 官方例程中将 Z 轴参考置零的命令字。 */
#define DAL_JY901P_RESET_YAW_REFERENCE_COMMAND        (0x04U)

/* 官方例程中将三轴角度参考置零的命令字。 */
#define DAL_JY901P_RESET_ANGLE_REFERENCE_COMMAND      (0x08U)

/* 参考归零命令的数据高字节；协议要求写入零。 */
#define DAL_JY901P_COMMAND_DATA_ZERO                  (0x00U)

/* ======== 类型定义 ======== */

/**
 * @brief JY901P 最新已解算姿态角快照。
 */
typedef struct {
    int16_t roll_raw;                 /**< 横滚角寄存器原始值，单位 LSB。 */
    int16_t pitch_raw;                /**< 俯仰角寄存器原始值，单位 LSB。 */
    int16_t yaw_raw;                  /**< 航向角寄存器原始值，单位 LSB。 */
    uint32_t last_update_ms;          /**< 最近有效角度帧时刻，单位 ms。 */
    uint32_t i2c_error_count;         /**< I2C 事务失败累计次数，仅递增供调试读取。 */
    uint32_t rejected_yaw_count;      /**< 被航向跳变保护丢弃的异常帧累计次数。 */
    bool angle_reference_ready;        /**< 首帧原始角度读取是否完成。 */
    uint32_t sequence;                 /**< 成功读取一帧角度后递增；0 表示尚未成功刷新。 */
} dal_jy901p_sample_t;

/* ======== 全局实例 ======== */

extern dal_jy901p_sample_t g_dal_jy901p_sample;

/* ======== 公开 API ======== */

/**
 * @brief 初始化 JY901P 参考归零和异步采样状态机。
 * @param void 无参数。
 * @return 无。
 */
void dal_jy901p_init(void);

/**
 * @brief 推进 JY901P 异步状态机并按周期刷新已解算角度快照。
 * @param void 无参数。
 * @return 无。
 */
void dal_jy901p_refresh(void);

#endif /* DAL_JY901P_H */