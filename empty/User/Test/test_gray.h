/**
 * @file test_gray.h
 * @brief 八路灰度传感器 UART 调试测试接口。
 */

#ifndef TEST_GRAY_H
#define TEST_GRAY_H

/* ======== include ======== */

/* ======== 可调参数宏定义 ======== */

/* 灰度传感器采样和打印周期，单位 ms；100 ms 兼顾观察实时性与 UART 输出负载。 */
#define TEST_GRAY_PERIOD_MS             (100U)

/* 单帧 UART 测试文本缓冲区大小，单位字节；需容纳灰度原始位图和换行符。 */
#define TEST_GRAY_UART_FRAME_SIZE       (16U)

/* ======== 公开 API ======== */

/**
 * @brief 初始化灰度传感器 UART 调试测试。
 * @param void 无参数。
 * @return 无。
 */
void test_gray_init(void);

/**
 * @brief 刷新灰度快照并尝试输出到 UART0。
 * @param void 无参数。
 * @return 无。
 */
void test_gray_refresh(void);

#endif /* TEST_GRAY_H */
