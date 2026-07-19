/**
 * @file dal_jy901p.c
 * @brief JY901P 已解算姿态角度 DAL 实现。
 */

/* ======== include ======== */

#include "dal_jy901p.h"

#include <stdbool.h>
#include <stdint.h>

#include "bsp_i2c.h"
#include "bsp_time.h"

/* ======== 类型定义 ======== */


/**
 * @brief JY901P 异步配置与采样状态。
 */
typedef enum {
    DAL_JY901P_STATE_IDLE = 0,             /**< 初始化前的空闲状态。 */
    DAL_JY901P_STATE_CONFIG_START,         /**< 准备发起参考归零写命令。 */
    DAL_JY901P_STATE_CONFIG_WAIT,          /**< 等待参考归零写命令完成。 */
    DAL_JY901P_STATE_CONFIG_SETTLE,        /**< 等待模块保存配置并稳定。 */
    DAL_JY901P_STATE_READY,                /**< 已可发起角度读帧。 */
    DAL_JY901P_STATE_SAMPLE_WAIT,          /**< 等待角度读帧完成。 */
    DAL_JY901P_STATE_ERROR                 /**< 等待重试配置。 */
} dal_jy901p_state_e;

/**
 * @brief JY901P 参考归零写命令。
 */
typedef struct {
    uint8_t reg;                                         /**< 写入目标寄存器地址。 */
    uint8_t data[DAL_JY901P_COMMAND_DATA_LEN];          /**< 写入数据。 */
} dal_jy901p_command_t;

/* ======== 全局实例 ======== */

dal_jy901p_sample_t g_dal_jy901p_sample;

/* ======== 内部变量 ======== */

static const dal_jy901p_command_t g_dal_jy901p_reference_commands[
    DAL_JY901P_CONFIGURATION_COMMAND_COUNT] = {
    {DAL_JY901P_REG_UNLOCK,
        {DAL_JY901P_UNLOCK_DATA_LOW, DAL_JY901P_UNLOCK_DATA_HIGH}},
    {DAL_JY901P_REG_ANGLE_REFERENCE,
        {DAL_JY901P_RESET_YAW_REFERENCE_COMMAND,
            DAL_JY901P_COMMAND_DATA_ZERO}},
    {DAL_JY901P_REG_SAVE,
        {DAL_JY901P_COMMAND_DATA_ZERO, DAL_JY901P_COMMAND_DATA_ZERO}},
    {DAL_JY901P_REG_UNLOCK,
        {DAL_JY901P_UNLOCK_DATA_LOW, DAL_JY901P_UNLOCK_DATA_HIGH}},
    {DAL_JY901P_REG_ANGLE_REFERENCE,
        {DAL_JY901P_RESET_ANGLE_REFERENCE_COMMAND,
            DAL_JY901P_COMMAND_DATA_ZERO}},
    {DAL_JY901P_REG_SAVE,
        {DAL_JY901P_COMMAND_DATA_ZERO, DAL_JY901P_COMMAND_DATA_ZERO}}
};

static dal_jy901p_state_e g_dal_jy901p_state = DAL_JY901P_STATE_IDLE;
static uint8_t g_dal_jy901p_angle_frame[DAL_JY901P_ANGLE_FRAME_LEN];
static uint8_t g_dal_jy901p_command_index;
static uint32_t g_dal_jy901p_sample_due_ms;
static uint32_t g_dal_jy901p_settle_due_ms;
static uint32_t g_dal_jy901p_error_retry_due_ms;

/* ======== 内部函数声明 ======== */

/**
 * @brief 判断当前时刻是否达到指定时刻。
 * @param now_ms 当前毫秒时刻。
 * @param due_ms 待比较的目标毫秒时刻。
 * @return 已到达或超过目标时刻返回 true，否则返回 false。
 */
static bool dal_jy901p_time_reached(uint32_t now_ms, uint32_t due_ms);

/**
 * @brief 将 Yaw 原始角度归一化到正负半圈范围。
 * @param yaw_raw 待归一化的 Yaw 原始角度，单位 LSB。
 * @return 归一化后的 Yaw 原始角度。
 */
static int32_t dal_jy901p_normalize_yaw_raw(int32_t yaw_raw);

/**
 * @brief 根据最近有效帧时刻更新姿态快照有效标志。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_update_snapshot_freshness(uint32_t now_ms);

/**
 * @brief 将 JY901P 小端角度寄存器数据转换为有符号 16 位原始值。
 * @param data 指向待转换数据低字节的指针。
 * @return 转换后的有符号原始角度值。
 */
static int16_t dal_jy901p_le16(const uint8_t *data);

/**
 * @brief 记录错误并禁止 APP 消费旧姿态数据。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_enter_error(uint32_t now_ms);

/**
 * @brief 从参考归零流程重新开始配置。
 * @param void 无参数。
 * @return 无。
 */
static void dal_jy901p_restart_configuration(void);

/**
 * @brief 发起当前参考归零写命令。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_start_reference_command(uint32_t now_ms);

/**
 * @brief 发起一次异步 JY901P 角度读帧。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_start_angle_read(uint32_t now_ms);

/**
 * @brief 解码并校验模块输出的原始 Roll、Pitch 和 Yaw 角度。
 * @param now_ms 当前毫秒时刻。
 * @return 接受有效角度帧返回 true，丢弃异常帧返回 false。
 */
static bool dal_jy901p_decode_angle_frame(uint32_t now_ms);

/**
 * @brief 推进 JY901P 配置或采样事务状态机。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_process(uint32_t now_ms);

/* ======== 内部函数 ======== */

/**
 * @brief 判断当前时刻是否达到指定时刻。
 * @param now_ms 当前毫秒时刻。
 * @param due_ms 待比较的目标毫秒时刻。
 * @return 已到达或超过目标时刻返回 true，否则返回 false。
 */
static bool dal_jy901p_time_reached(uint32_t now_ms, uint32_t due_ms)
{
    return ((int32_t)(now_ms - due_ms) >= 0);
}

/**
 * @brief 将 Yaw 原始角度归一化到正负半圈范围。
 * @param yaw_raw 待归一化的 Yaw 原始角度，单位 LSB。
 * @return 归一化后的 Yaw 原始角度。
 */
static int32_t dal_jy901p_normalize_yaw_raw(int32_t yaw_raw)
{
    while (yaw_raw >= DAL_JY901P_YAW_HALF_TURN_RAW) {
        yaw_raw -= DAL_JY901P_YAW_FULL_TURN_RAW;
    }

    while (yaw_raw < -DAL_JY901P_YAW_HALF_TURN_RAW) {
        yaw_raw += DAL_JY901P_YAW_FULL_TURN_RAW;
    }

    return yaw_raw;
}

/**
 * @brief 根据最近有效帧时刻更新姿态快照有效标志。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_update_snapshot_freshness(uint32_t now_ms)
{
    if ((g_dal_jy901p_sample.sequence != 0U) &&
        dal_jy901p_time_reached(now_ms,
            g_dal_jy901p_sample.last_update_ms +
            DAL_JY901P_SNAPSHOT_STALE_TIMEOUT_MS)) {
        g_dal_jy901p_sample.angle_reference_ready = false;
    }
}

/**
 * @brief 将 JY901P 小端角度寄存器数据转换为有符号 16 位原始值。
 * @param data 指向待转换数据低字节的指针。
 * @return 转换后的有符号原始角度值。
 */
static int16_t dal_jy901p_le16(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] |
        ((uint16_t)data[1] << DAL_JY901P_HIGH_BYTE_SHIFT));
}

/**
 * @brief 记录错误并禁止 APP 消费旧姿态数据。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_enter_error(uint32_t now_ms)
{
    g_dal_jy901p_sample.i2c_error_count++;
    if (g_dal_jy901p_sample.sequence == 0U) {
        g_dal_jy901p_sample.angle_reference_ready = false;
    }
    g_dal_jy901p_error_retry_due_ms = now_ms + DAL_JY901P_ERROR_RETRY_MS;
    g_dal_jy901p_state = DAL_JY901P_STATE_ERROR;
}

/**
 * @brief 从参考归零流程重新开始配置。
 * @param void 无参数。
 * @return 无。
 */
static void dal_jy901p_restart_configuration(void)
{
    g_dal_jy901p_command_index = 0U;
    if (g_dal_jy901p_sample.sequence == 0U) {
        g_dal_jy901p_sample.angle_reference_ready = false;
    }

    if (DAL_JY901P_REFERENCE_RESET_ENABLE != 0U) {
        g_dal_jy901p_state = DAL_JY901P_STATE_CONFIG_START;
    } else {
        g_dal_jy901p_state = DAL_JY901P_STATE_READY;
    }
}

/**
 * @brief 发起当前参考归零写命令。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_start_reference_command(uint32_t now_ms)
{
    const dal_jy901p_command_t *command =
        &g_dal_jy901p_reference_commands[g_dal_jy901p_command_index];
    bsp_status_e result = bsp_i2c_mem_write_async(DAL_JY901P_I2C_ADDRESS,
        command->reg, command->data, DAL_JY901P_COMMAND_DATA_LEN);

    if (result == BSP_OK) {
        g_dal_jy901p_state = DAL_JY901P_STATE_CONFIG_WAIT;
    } else if (result != BSP_ERR_BUSY) {
        dal_jy901p_enter_error(now_ms);
    }
}

/**
 * @brief 发起一次异步 JY901P 角度读帧。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_start_angle_read(uint32_t now_ms)
{
    bsp_status_e result = bsp_i2c_mem_read_async(DAL_JY901P_I2C_ADDRESS,
        DAL_JY901P_REG_ROLL_L, g_dal_jy901p_angle_frame,
        DAL_JY901P_ANGLE_FRAME_LEN);

    if (result == BSP_OK) {
        g_dal_jy901p_state = DAL_JY901P_STATE_SAMPLE_WAIT;
    } else if (result != BSP_ERR_BUSY) {
        dal_jy901p_enter_error(now_ms);
    }
}

/**
 * @brief 解码模块已解算的 Roll、Pitch 和 Yaw 角度。
 * @param void 无参数。
 * @return 无。
 */
static bool dal_jy901p_decode_angle_frame(uint32_t now_ms)
{
    int16_t roll_raw = dal_jy901p_le16(
        &g_dal_jy901p_angle_frame[DAL_JY901P_FRAME_ROLL_OFFSET]);
    int16_t pitch_raw = dal_jy901p_le16(
        &g_dal_jy901p_angle_frame[DAL_JY901P_FRAME_PITCH_OFFSET]);
    int16_t yaw_raw = dal_jy901p_le16(
        &g_dal_jy901p_angle_frame[DAL_JY901P_FRAME_YAW_OFFSET]);
    int32_t yaw_delta_raw;

    if (g_dal_jy901p_sample.sequence != 0U) {
        yaw_delta_raw = dal_jy901p_normalize_yaw_raw((int32_t)yaw_raw -
            (int32_t)g_dal_jy901p_sample.yaw_raw);
        if ((yaw_delta_raw > DAL_JY901P_YAW_MAX_STEP_RAW) ||
            (yaw_delta_raw < -DAL_JY901P_YAW_MAX_STEP_RAW)) {
            g_dal_jy901p_sample.rejected_yaw_count++;
            return false;
        }
    }

    g_dal_jy901p_sample.roll_raw = roll_raw;
    g_dal_jy901p_sample.pitch_raw = pitch_raw;
    g_dal_jy901p_sample.yaw_raw = yaw_raw;
    g_dal_jy901p_sample.last_update_ms = now_ms;
    g_dal_jy901p_sample.angle_reference_ready = true;
    g_dal_jy901p_sample.sequence++;
    return true;
}
/**
 * @brief 推进 JY901P 配置或采样事务状态机。
 * @param now_ms 当前毫秒时刻。
 * @return 无。
 */
static void dal_jy901p_process(uint32_t now_ms)
{
    bsp_status_e result;

    switch (g_dal_jy901p_state) {
    case DAL_JY901P_STATE_CONFIG_START:
        dal_jy901p_start_reference_command(now_ms);
        break;

    case DAL_JY901P_STATE_CONFIG_WAIT:
        result = bsp_i2c_poll();
        if (result == BSP_ERR_BUSY) {
            break;
        }
        if (result != BSP_OK) {
            dal_jy901p_enter_error(now_ms);
            break;
        }
        g_dal_jy901p_settle_due_ms = now_ms + DAL_JY901P_COMMAND_SETTLE_MS;
        g_dal_jy901p_state = DAL_JY901P_STATE_CONFIG_SETTLE;
        break;

    case DAL_JY901P_STATE_CONFIG_SETTLE:
        if (!dal_jy901p_time_reached(now_ms, g_dal_jy901p_settle_due_ms)) {
            break;
        }
        g_dal_jy901p_command_index++;
        if (g_dal_jy901p_command_index >=
            DAL_JY901P_CONFIGURATION_COMMAND_COUNT) {
            g_dal_jy901p_state = DAL_JY901P_STATE_READY;
        } else {
            g_dal_jy901p_state = DAL_JY901P_STATE_CONFIG_START;
        }
        break;

    case DAL_JY901P_STATE_SAMPLE_WAIT:
        result = bsp_i2c_poll();
        if (result == BSP_ERR_BUSY) {
            break;
        }
        if (result != BSP_OK) {
            dal_jy901p_enter_error(now_ms);
            break;
        }
        (void)dal_jy901p_decode_angle_frame(now_ms);
        g_dal_jy901p_state = DAL_JY901P_STATE_READY;
        break;

    case DAL_JY901P_STATE_IDLE:
    case DAL_JY901P_STATE_READY:
    case DAL_JY901P_STATE_ERROR:
    default:
        break;
    }
}

/* ======== 公开 API ======== */

void dal_jy901p_init(void)
{
    g_dal_jy901p_sample = (dal_jy901p_sample_t){0};
    g_dal_jy901p_sample_due_ms = bsp_time_get_ms();
    g_dal_jy901p_settle_due_ms = 0U;
    g_dal_jy901p_error_retry_due_ms = 0U;
    dal_jy901p_restart_configuration();
}

void dal_jy901p_refresh(void)
{
    uint32_t now_ms = bsp_time_get_ms();

    dal_jy901p_process(now_ms);
    dal_jy901p_update_snapshot_freshness(now_ms);

    if ((g_dal_jy901p_state == DAL_JY901P_STATE_ERROR) &&
        dal_jy901p_time_reached(now_ms, g_dal_jy901p_error_retry_due_ms)) {
        dal_jy901p_restart_configuration();
    }

    if (dal_jy901p_time_reached(now_ms, g_dal_jy901p_sample_due_ms)) {
        g_dal_jy901p_sample_due_ms += DAL_JY901P_SAMPLE_PERIOD_MS;
        if (g_dal_jy901p_state == DAL_JY901P_STATE_READY) {
            dal_jy901p_start_angle_read(now_ms);
        }
    }
}