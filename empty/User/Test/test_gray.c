/**
 * @file test_gray.c
 * @brief 八路灰度传感器 UART 调试测试实现。
 */

/* ======== include ======== */

#include "test_gray.h"

#include <stddef.h>
#include <stdint.h>

#include "bsp_uart.h"
#include "dal_gray.h"

/* ======== 可调参数宏定义 ======== */

/* 单个十六进制数字表示的位数；用于从灰度原始位图取高、低半字节。 */
#define TEST_GRAY_HEX_NIBBLE_BITS       (4U)

/* 十六进制低半字节掩码；限制字符索引处于 0x0~0xF。 */
#define TEST_GRAY_HEX_NIBBLE_MASK       (0x0FU)

/* 一个灰度原始位图需要的十六进制字符数，单位字符。 */
#define TEST_GRAY_RAW_HEX_CHAR_COUNT    (2U)

/* ======== 内部变量 ======== */

static const uint8_t g_test_gray_frame_prefix[] = "GRAY=";
static const uint8_t g_test_gray_frame_suffix[] = "\r\n";
static const uint8_t g_test_gray_hex_chars[] = "0123456789ABCDEF";

/* ======== 内部函数声明 ======== */

/**
 * @brief 将灰度原始位图编码为最小 ASCII 调试帧。
 * @param raw_mask 灰度传感器原始位图。
 * @param frame 输出文本缓冲区。
 * @param frame_size 输出文本缓冲区大小，单位字节。
 * @return 成功返回待发送字节数，缓冲区不足返回 0。
 */
static size_t test_gray_build_frame(uint8_t raw_mask,
    uint8_t *frame, size_t frame_size);

/* ======== 内部函数 ======== */

/**
 * @brief 将灰度原始位图编码为最小 ASCII 调试帧。
 * @param raw_mask 灰度传感器原始位图。
 * @param frame 输出文本缓冲区。
 * @param frame_size 输出文本缓冲区大小，单位字节。
 * @return 成功返回待发送字节数，缓冲区不足返回 0。
 */
static size_t test_gray_build_frame(uint8_t raw_mask,
    uint8_t *frame, size_t frame_size)
{
    size_t frame_index = 0U;
    size_t prefix_index;
    size_t suffix_index;
    size_t required_size = (sizeof(g_test_gray_frame_prefix) - 1U) +
        (sizeof(g_test_gray_frame_suffix) - 1U) +
        TEST_GRAY_RAW_HEX_CHAR_COUNT;

    if ((frame == NULL) || (frame_size < required_size)) {
        return 0U;
    }

    for (prefix_index = 0U;
        prefix_index < (sizeof(g_test_gray_frame_prefix) - 1U);
        prefix_index++) {
        frame[frame_index++] = g_test_gray_frame_prefix[prefix_index];
    }

    frame[frame_index++] = g_test_gray_hex_chars[
        (raw_mask >> TEST_GRAY_HEX_NIBBLE_BITS) & TEST_GRAY_HEX_NIBBLE_MASK];
    frame[frame_index++] =
        g_test_gray_hex_chars[raw_mask & TEST_GRAY_HEX_NIBBLE_MASK];

    for (suffix_index = 0U;
        suffix_index < (sizeof(g_test_gray_frame_suffix) - 1U);
        suffix_index++) {
        frame[frame_index++] = g_test_gray_frame_suffix[suffix_index];
    }

    return frame_index;
}

/* ======== 公开 API ======== */

void test_gray_init(void)
{
    dal_gray_init();
}

void test_gray_refresh(void)
{
    uint8_t tx_frame[TEST_GRAY_UART_FRAME_SIZE];
    size_t tx_length;

    dal_gray_refresh();
    tx_length = test_gray_build_frame(g_dal_gray_sample.raw_mask,
        tx_frame, sizeof(tx_frame));

    /* WHY: 异步接口会立即复制帧数据，避免日志发送占用下一次灰度采样。 */
    (void)bsp_uart_write_async(tx_frame, tx_length);
}
