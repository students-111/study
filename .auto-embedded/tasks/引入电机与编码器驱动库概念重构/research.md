# 研究发现

| 关键词 | 来源 | 摘要 | 可信度 | 状态 |
|---|---|---|---|---|
| 当前电机 DAL | `empty/User/DAL/dal_motor.c` | 原实现直接在 DAL 内处理 TB6612 方向脚、PWM 通道、速度正负号和停止逻辑。 | 高 | 已重构 |
| 当前编码器 DAL | `empty/User/DAL/dal_encoder.c` | 原实现直接在 DAL 内保存 A/B 相引脚、正交解码表、累计计数和方向快照。 | 高 | 已重构 |
| APP 调用面 | `empty/User/APP/app_line_follow.c`、`empty/User/APP/app_straight_drive.c` | APP 通过 `dal_motor_set_speed()` 和 `g_dal_encoder_sample[]` 使用电机/编码器能力，本次 MVP 可保持不变。 | 高 | 已保持 |
| 工程入口 | `empty/keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx` | Keil 工程显式维护源文件和 include path，需要加入 `User/Driver`。 | 高 | 已更新 |
