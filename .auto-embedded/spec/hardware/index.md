# 硬件资源 spec（hardware）

> 全工程共享的"硬件事实基线"。RESEARCH 阶段冻结，后续以此为准；改动留变更记录。
> 机器可读锁定区在 `hw-lock.yaml`，REVIEW 的 ARCH-8 与冲突检测据此运行。

## 芯片与框架
- 芯片型号：MSPM0G3507，LQFP-64（PM）封装。
- 开发框架：TI MSPM0 SDK 2.02.00.05，SysConfig 1.21.1。
- 主频 / 时钟树：80 MHz 系统时钟，HFXT 40 MHz。

## 引脚 / DMA / 中断（人类视图）
> 详细分配维护在 `hw-lock.yaml`（机器可读）；此处写设计理由与变更记录。

- 已冻结资源：28 个引脚、DMA_CH0、GPIOB/I2C0/UART0/SysTick 中断和 TIMG8 PWM（40 MHz 计数时钟、2,000 计数周期、20 kHz）。

## 变更记录
| 日期 | 改了什么 | 原因 |
|---|---|---|
| 2026-07-12 | 按 `empty.syscfg` 登记现有硬件资源锁并人工确认芯片型号 | 防止自动探测误判为 ESP32-Arduino 后产生错误资源规划 |
| 2026-07-12 | PWM 周期从 1,600 调整为 2,000，SysTick 配置收敛到 SysConfig | 在 80 MHz CPUCLK / 40 MHz BUSCLK 下保持原 20 kHz PWM 和 1 ms 时基 |
| 2026-07-12 | Key1 从 PA18 改到 PB21，GRAY_D2 从 PB21 改到 PB0 | 释放 PB21 供新接入的 Key1 使用，同时保持八路灰度传感器可用 |
| 2026-07-12 | Key1 配置为 PB21 内部上拉、低电平有效 | 按键原理图显示按下时 PB21 直接接 GND，且无外部上拉电阻 |

## 沉淀（promote 回流）
> 本板踩过的硬件坑（如某 strap 脚、某外设时钟门）沉淀于此。
- [坑/gotcha] CPU 时钟切换后，必须按实际外设时钟重算 PWM 周期并以 SysConfig 生成值复核；BSP 不得重复配置 SysConfig 已管理的固定时基。
