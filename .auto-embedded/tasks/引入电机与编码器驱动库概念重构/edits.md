# 编辑清单

| 文件 | 改动 | 验证标准 | 结果 | commit |
|---|---|---|---|---|
| `empty/User/Driver/drv_motor.h` / `drv_motor.c` | 将电机库简化为 `Motor_t`、`MotorHW_t`、`MotorState_t`，移除回调、config、bind。 | 电机 Driver 直接调用 BSP 抽象接口；无 `callback/ops/bind/config` 复杂转发。 | 已通过门禁，未完整编译。 | 未提交 |
| `empty/User/Driver/drv_encoder.h` / `drv_encoder.c` | 将编码器库简化为 `Encoder_t`、`EncoderHW_t`、`EncoderState_t`，移除回调、config、bind。 | 编码器 Driver 直接调用 BSP 抽象接口；无 `callback/ops/bind/config` 复杂转发。 | 已通过门禁，未完整编译。 | 未提交 |
| `empty/User/DAL/dal_motor.c` / `dal_motor.h` | DAL 改为只创建左右电机 `Motor_t` 实例并调用 `Motor_SetSpeed()`；公开 API 保持兼容。 | APP 无需改动；`dal_motor_set_speed/stop` 仍可用。 | 已通过静态引用检查。 | 未提交 |
| `empty/User/DAL/dal_encoder.c` | DAL 改为只创建 M1/M2 `Encoder_t` 实例并调用 `Encoder_Update()`；继续同步 `g_dal_encoder_sample[]`。 | 快照访问规则保持不变。 | 已通过静态引用检查。 | 未提交 |
| `empty/User/DAL/dal_key.c` | 删除 Key1 BSP 别名宏、按键合法性封装和单键分发函数，直接刷新 Key1 消抖状态。 | DAL 不再保留无意义 BSP 别名和单键 id 分发。 | 已通过静态引用检查。 | 未提交 |
| `empty/User/DAL/dal_motor.c` | 删除 AIN/BIN/STBY BSP 别名宏、枚举合法性判断、单行 BSP 转发函数。 | 硬件表和初始化直接使用 BSP 枚举。 | 已通过静态引用检查。 | 未提交 |
| `empty/keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx` | 增加 `../User/Driver` include path 和 `User/Driver` 工程组。 | Keil 工程解析目标成功。 | 已验证可解析；本机缺 UV4/ARMCLANG 无法编译。 | 未提交 |
| `CODE_STYLE_AND_ARCHITECTURE.md` | 新增/更新 Driver 与 DAL 边界说明。 | 框架分层变化已同步规范文档。 | 已更新。 | 未提交 |
