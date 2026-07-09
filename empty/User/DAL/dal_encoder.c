/**
 * @file dal_encoder.c
 * @brief 编码器正交解码 DAL 实现。
 */

/* ======== include ======== */

#include "dal_encoder.h"

#include <stddef.h>

#include "../Driver/drv_encoder.h"
#include "bsp_gpio.h"

/* ======== 可调参数宏定义 ======== */

/* M1 编码器安装方向；1 表示反装。 */
#define DAL_ENCODER_M1_REVERSE          (1U)

/* M2 编码器安装方向；0 表示正装。 */
#define DAL_ENCODER_M2_REVERSE          (0U)

/* ======== 内部变量 ======== */

static Encoder_t g_dal_encoder_instances[DAL_ENCODER_COUNT];

dal_encoder_sample_t g_dal_encoder_sample[DAL_ENCODER_COUNT];

static const EncoderHW_t g_dal_encoder_hw[DAL_ENCODER_COUNT] = {
    {
        NULL,
        BSP_GPIO_ENC_M1_A,
        NULL,
        BSP_GPIO_ENC_M1_B
    },
    {
        NULL,
        BSP_GPIO_ENC_M2_A,
        NULL,
        BSP_GPIO_ENC_M2_B
    }
};

/* ======== 内部函数 ======== */

/**
 * @brief 将驱动层状态转换为 DAL 方向。
 * @param state 驱动层状态。
 * @return DAL 方向。
 */
static dal_encoder_direction_e dal_encoder_to_direction(EncoderState_t state)
{
    switch (state) {
    case ENCODER_STATE_FORWARD:
        return DAL_ENCODER_DIR_FORWARD;
    case ENCODER_STATE_BACKWARD:
        return DAL_ENCODER_DIR_REVERSE;
    case ENCODER_STATE_ERROR:
        return DAL_ENCODER_DIR_ERROR;
    case ENCODER_STATE_STOP:
    default:
        return DAL_ENCODER_DIR_STOP;
    }
}

/**
 * @brief 从驱动对象同步最新快照到 DAL 全局快照。
 * @param id 编码器通道编号。
 * @return 无。
 */
static void dal_encoder_sync_sample(dal_encoder_id_e id)
{
    const Encoder_t *encoder = &g_dal_encoder_instances[id];
    dal_encoder_sample_t *sample = &g_dal_encoder_sample[id];

    sample->count = encoder->count;
    sample->delta = encoder->delta;
    sample->direction = dal_encoder_to_direction(encoder->state);
    sample->raw_state = encoder->raw_state;
    sample->sequence = encoder->sequence;
}

/* ======== 公开 API ======== */

void dal_encoder_init(void)
{
    unsigned int idx;

    /*
     * WHY: 编码器 A/B 相是开关量输入，上拉和滞回可降低悬空与边沿抖动
     * 对低速软件解码的影响。
     */
    (void)bsp_gpio_init_input(BSP_GPIO_ENC_M1_A, BSP_GPIO_PULL_UP, true);
    (void)bsp_gpio_init_input(BSP_GPIO_ENC_M1_B, BSP_GPIO_PULL_UP, true);
    (void)bsp_gpio_init_input(BSP_GPIO_ENC_M2_A, BSP_GPIO_PULL_UP, true);
    (void)bsp_gpio_init_input(BSP_GPIO_ENC_M2_B, BSP_GPIO_PULL_UP, true);

    (void)Encoder_Init(&g_dal_encoder_instances[DAL_ENCODER_M1],
        &g_dal_encoder_hw[DAL_ENCODER_M1], DAL_ENCODER_M1_REVERSE);
    (void)Encoder_Init(&g_dal_encoder_instances[DAL_ENCODER_M2],
        &g_dal_encoder_hw[DAL_ENCODER_M2], DAL_ENCODER_M2_REVERSE);

    for (idx = 0U; idx < (unsigned int)DAL_ENCODER_COUNT; idx++) {
        dal_encoder_sync_sample((dal_encoder_id_e)idx);
    }
}

void dal_encoder_refresh(void)
{
    unsigned int idx;

    for (idx = 0U; idx < (unsigned int)DAL_ENCODER_COUNT; idx++) {
        (void)Encoder_Update(&g_dal_encoder_instances[idx]);
        dal_encoder_sync_sample((dal_encoder_id_e)idx);
    }
}

void dal_encoder_reset(dal_encoder_id_e id)
{
    (void)Encoder_Reset(&g_dal_encoder_instances[id]);
    dal_encoder_sync_sample(id);
}
