/**
 * @file dal_encoder.c
 * @brief 编码器正交解码 DAL 实现。
 */

/* ======== include ======== */

#include "dal_encoder.h"

#include <stddef.h>

#include "bsp_encoder.h"

/* ======== 可调参数宏定义 ======== */

/* M1 编码器安装方向；1 表示反装。 */
#define DAL_ENCODER_M1_REVERSE          (1U)

/* M2 编码器安装方向；0 表示正装。 */
#define DAL_ENCODER_M2_REVERSE          (0U)

/* ======== 全局实例 ======== */

volatile dal_encoder_sample_t g_dal_encoder_sample[DAL_ENCODER_COUNT];

/* ======== 内部变量 ======== */

static Encoder_t g_dal_encoder_instances[DAL_ENCODER_COUNT];
static int32_t g_dal_encoder_last_count[DAL_ENCODER_COUNT];
static bool g_dal_encoder_has_last_count[DAL_ENCODER_COUNT];

static const bsp_encoder_channel_e g_dal_encoder_bsp_channels[DAL_ENCODER_COUNT] = {
    BSP_ENCODER_M1,
    BSP_ENCODER_M2
};

/* ======== 内部函数 ======== */

/**
 * @brief 将 32 位计数差限幅到 16 位速度快照。
 * @param count_delta 本周期编码器计数差。
 * @return 限幅后的 counts/speed-period 速度。
 */
static int16_t dal_encoder_limit_speed_cp(int32_t count_delta)
{
    if (count_delta > INT16_MAX) {
        return INT16_MAX;
    }
    if (count_delta < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)count_delta;
}

/**
 * @brief 从驱动对象同步最新快照到 DAL 全局快照。
 * @param id 编码器通道编号。
 * @return 无。
 */
static void dal_encoder_sync_sample(dal_encoder_id_e id)
{
    const Encoder_t *encoder = &g_dal_encoder_instances[id];
    volatile dal_encoder_sample_t *sample = &g_dal_encoder_sample[id];

    sample->count = encoder->count;
    sample->delta = encoder->delta;
    sample->state = encoder->state;
    sample->raw_state = encoder->raw_state;
    sample->sequence = encoder->sequence;
}

/**
 * @brief 读取并解码单个编码器当前 A/B 相状态。
 * @param id 编码器通道编号。
 * @return 刷新成功返回 true，否则返回 false。
 */
static bool dal_encoder_read_and_update_one(dal_encoder_id_e id)
{
    bool phase_a;
    bool phase_b;

    if (bsp_encoder_read_phases(g_dal_encoder_bsp_channels[id],
        &phase_a, &phase_b) != BSP_OK) {
        return false;
    }

    return (Encoder_Update(&g_dal_encoder_instances[id],
        phase_a, phase_b) != 0U);
}

/**
 * @brief 处理 BSP 上报的单路编码器边沿。
 * @param channel 发生边沿的板级编码器通道。
 * @param context 回调上下文，本实现未使用。
 * @return 无。
 */
static void dal_encoder_handle_edge(bsp_encoder_channel_e channel,
    void *context)
{
    dal_encoder_id_e id;

    (void)context;
    if (channel == BSP_ENCODER_M1) {
        id = DAL_ENCODER_M1;
    } else if (channel == BSP_ENCODER_M2) {
        id = DAL_ENCODER_M2;
    } else {
        return;
    }

    if (dal_encoder_read_and_update_one(id)) {
        dal_encoder_sync_sample(id);
    }
}

/**
 * @brief 根据累计计数刷新单个编码器速度快照。
 * @param id 编码器通道编号。
 * @return 无。
 */
static void dal_encoder_update_speed_one(dal_encoder_id_e id)
{
    volatile dal_encoder_sample_t *sample = &g_dal_encoder_sample[id];
    int32_t count_delta;

    if ((sample->sequence == 0U) || (sample->state == ENCODER_STATE_ERROR)) {
        g_dal_encoder_has_last_count[id] = false;
        sample->speed_cp = 0;
        sample->speed_valid = false;
        return;
    }

    if (!g_dal_encoder_has_last_count[id]) {
        g_dal_encoder_last_count[id] = sample->count;
        g_dal_encoder_has_last_count[id] = true;
        sample->speed_cp = 0;
        sample->speed_valid = false;
        return;
    }

    count_delta = sample->count - g_dal_encoder_last_count[id];
    g_dal_encoder_last_count[id] = sample->count;
    sample->speed_cp = dal_encoder_limit_speed_cp(count_delta);
    sample->speed_valid = true;
}

/* ======== 公开 API ======== */

void dal_encoder_init(void)
{
    unsigned int idx;

    (void)Encoder_Init(&g_dal_encoder_instances[DAL_ENCODER_M1],
        DAL_ENCODER_M1_REVERSE);
    (void)Encoder_Init(&g_dal_encoder_instances[DAL_ENCODER_M2],
        DAL_ENCODER_M2_REVERSE);

    for (idx = 0U; idx < (unsigned int)DAL_ENCODER_COUNT; idx++) {
        (void)dal_encoder_read_and_update_one((dal_encoder_id_e)idx);
        g_dal_encoder_last_count[idx] = 0;
        g_dal_encoder_has_last_count[idx] = false;
        dal_encoder_sync_sample((dal_encoder_id_e)idx);
        dal_encoder_update_speed_one((dal_encoder_id_e)idx);
    }

    (void)bsp_encoder_init(dal_encoder_handle_edge, NULL);
}


/* 刷新编码器计数。 */
void dal_encoder_refresh(void)
{
    unsigned int idx;
    uint32_t irq_was_enabled = bsp_encoder_irq_lock();

    for (idx = 0U; idx < (unsigned int)DAL_ENCODER_COUNT; idx++) {
        (void)dal_encoder_read_and_update_one((dal_encoder_id_e)idx);
        dal_encoder_sync_sample((dal_encoder_id_e)idx);
    }
    bsp_encoder_irq_unlock(irq_was_enabled);
}



/* 按当前测速周期刷新速度。 */
void dal_encoder_update_speed(void)
{
    unsigned int idx;
    uint32_t irq_was_enabled = bsp_encoder_irq_lock();

    for (idx = 0U; idx < (unsigned int)DAL_ENCODER_COUNT; idx++) {
        dal_encoder_update_speed_one((dal_encoder_id_e)idx);
    }
    bsp_encoder_irq_unlock(irq_was_enabled);
}

void dal_encoder_reset(dal_encoder_id_e id)
{
    uint32_t irq_was_enabled;

    if ((unsigned int)id >= (unsigned int)DAL_ENCODER_COUNT) {
        return;
    }

    irq_was_enabled = bsp_encoder_irq_lock();
    (void)Encoder_Reset(&g_dal_encoder_instances[id]);
    (void)dal_encoder_read_and_update_one(id);
    g_dal_encoder_last_count[id] = 0;
    g_dal_encoder_has_last_count[id] = false;
    dal_encoder_sync_sample(id);
    dal_encoder_update_speed_one(id);
    bsp_encoder_irq_unlock(irq_was_enabled);
}
