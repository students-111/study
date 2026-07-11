# 编辑清单

| 文件 | 改动 | 验证标准 | 结果 | commit |
|---|---|---|---|---|
| `empty/empty.syscfg`、`empty/ti_msp_dl_config.c/.h` | 四路编码器输入启用双边沿中断并重新生成 | 生成差异只包含编码器中断 | 通过 | 无，工作区已有用户改动 |
| `empty/User/BSP/bsp_encoder.c/.h` | 新增 GPIOB Group1 中断分发与相位快照 | 两路 simultaneous pending 均分发，ISR 无阻塞 | 通过 | 无，工作区已有用户改动 |
| Driver、DAL、CPU 编码器链路 | 四倍频边沿解码、volatile/临界区、停用 1 ms 轮询 | Keil 0 Error/0 Warning，10 ms 测速保留 | 通过 | 无，工作区已有用户改动 |
| 架构、资源与 spec | 同步编码器中断边界、hw-lock 和防复发规则 | ARCH/HW/SPEC 全 PASS | 通过 | 无，工作区已有用户改动 |

待实机验证：两路正反转符号、单圈四倍频计数、两路同时高速旋转和最高可靠边沿频率。
