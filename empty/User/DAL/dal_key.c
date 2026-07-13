/**
 * @file dal_key.c
 * @brief 板载功能按键 DAL 实现。
 */

/* ======== include ======== */

#include "dal_key.h"

#include <stddef.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

/**
 * @brief 按键通道运行状态。
 */
typedef struct {
    GPIO_Regs *port;             /**< 按键 GPIO 端口。 */
    uint32_t pin;                /**< 按键 GPIO pin mask。 */
    bool active_high;            /**< true 表示高电平为按下。 */
    bool raw_last;               /**< 上一次采到的原始按下状态。 */
    bool stable_pressed;         /**< 当前消抖后的按下状态。 */
    uint8_t stable_ticks;        /**< 连续稳定的刷新次数。 */
} dal_key_channel_t;

/* ======== 全局实例 ======== */

dal_key_sample_t g_dal_key_sample[DAL_KEY_COUNT];

/* ======== 内部变量 ======== */

static dal_key_channel_t g_dal_key1_channel = {
    .port = BOARD_GPIO_PORT,
    .pin = BOARD_GPIO_KEY1_PIN,
    .active_high = false,
};

/* ======== 内部函数 ======== */

/**
 * @brief 读取按键原始按下状态。
 * @param channel 按键通道状态。
 * @param pressed 输出按键是否按下。
 * @return 读取成功返回 true，否则返回 false。
 */
static bool dal_key_read_pressed(const dal_key_channel_t *channel,
    bool *pressed)
{
    bool high = false;

    if ((channel == NULL) || (pressed == NULL)) {
        return false;
    }

    high = ((DL_GPIO_readPins(channel->port, channel->pin) & channel->pin) !=
        0U);

    *pressed = channel->active_high ? high : !high;
    return true;
}

/* ======== 公开 API ======== */

void dal_key_init(void)
{
    unsigned int idx;

    g_dal_key1_channel.raw_last = false;
    g_dal_key1_channel.stable_pressed = false;
    g_dal_key1_channel.stable_ticks = 0U;

    for (idx = 0U; idx < (unsigned int)DAL_KEY_COUNT; idx++) {
        g_dal_key_sample[idx].pressed = false;
        g_dal_key_sample[idx].pressed_edge = false;
        g_dal_key_sample[idx].sequence = 0U;
    }
}

void dal_key_refresh(void)
{
    bool raw_pressed;
    bool old_pressed;

    g_dal_key_sample[DAL_KEY_KEY1].pressed_edge = false;

    if (!dal_key_read_pressed(&g_dal_key1_channel, &raw_pressed)) {
        g_dal_key_sample[DAL_KEY_KEY1].sequence++;
        return;
    }

    if (raw_pressed == g_dal_key1_channel.raw_last) {
        if (g_dal_key1_channel.stable_ticks < DAL_KEY_DEBOUNCE_TICKS) {
            g_dal_key1_channel.stable_ticks++;
        }
    } else {
        g_dal_key1_channel.raw_last = raw_pressed;
        g_dal_key1_channel.stable_ticks = 1U;
    }

    if (g_dal_key1_channel.stable_ticks >= DAL_KEY_DEBOUNCE_TICKS) {
        old_pressed = g_dal_key1_channel.stable_pressed;
        g_dal_key1_channel.stable_pressed = raw_pressed;
        g_dal_key_sample[DAL_KEY_KEY1].pressed =
            g_dal_key1_channel.stable_pressed;
        g_dal_key_sample[DAL_KEY_KEY1].pressed_edge =
            (!old_pressed && g_dal_key1_channel.stable_pressed);
    }

    g_dal_key_sample[DAL_KEY_KEY1].sequence++;
}
