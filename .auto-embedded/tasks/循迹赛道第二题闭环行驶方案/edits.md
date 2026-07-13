# 编辑清单

| 文件 | 改动 | 验证标准 | 结果 | commit |
|---|---|---|---|---|
| `empty/User/APP/app.h` | 新增赛题状态机接口与可调参数宏。 | 头文件声明可供调度器调用，宏覆盖周期、边沿确认次数和暂停时长。 | Keil 重建通过；ARCH/HW/SPEC 通过 | 未提交 |
| `empty/User/APP/app.c` | 新增 Key1 启动、黑白边沿确认、四次暂停和最终停车状态机。 | 状态仅复用既有直线/循迹 APP，且按 B/C/D/A 顺序切换。 | Keil 重建通过；ARCH/HW/SPEC 通过 | 未提交 |
| `empty/User/CPU/scheduler.c` | 调度 Key1 刷新和新 `app_refresh()`，替换直接循迹入口。 | 灰度、按键、MPU 刷新早于状态机调用。 | Keil 重建通过；ARCH/HW/SPEC 通过 | 未提交 |
| `empty/keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx` | 将 `app.[ch]` 纳入 Keil 工程。 | XML 可解析且 User/APP 组包含新文件。 | 已通过 XML 解析和清单检查 | 未提交 |
| `empty/empty.syscfg`、`empty/User/DAL/dal_key.c` | Key1 迁至 PB21，GRAY_D2 迁至 PB0；Key1 改为内部上拉、低电平有效。 | SysConfig 生成 KEY1/GRAY_D2 宏与原理图电平逻辑一致。 | 已生成核对；Keil 重建与 ARCH/HW/SPEC 通过 | 未提交 |
| `empty/User/BSP/bsp_encoder.c`、`empty/User/DAL/dal_gray.c`、`empty/User/DAL/dal_motor.c` | 板级 GPIO 端口引用改为 SysConfig 生成的 `BOARD_GPIO_PORT`。 | 修复同端口 GPIO 组不再生成各功能 `*_PORT` 宏导致的构建错误。 | Keil 重建通过 | 未提交 |
| `empty/User/APP/app_line_follow.c` | 以 `bsp_time_get_ms()` 替代 APP 层 `extern g_bsp_time_ms`。 | 消除 ARCH-4 违规，不改变 PID 时基来源。 | ARCH 门禁通过 | 未提交 |
