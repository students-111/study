# 硬件识别记录

> 已人工核对 SysConfig 配置；不采用文件名索引产生的 ESP32-Arduino 误判。

- 芯片：MSPM0G3507，LQFP-64（PM）封装。
- SDK：mspm0_sdk@2.02.00.05。

## 证据

- `empty/empty.syscfg` 的 `@v2CliArgs` 指定 `--device "MSPM0G3507"`。
- `empty/empty.syscfg` 指定 80 MHz 系统时钟、I2C0、UART0、TIMG8 和 DMA_CH0。
