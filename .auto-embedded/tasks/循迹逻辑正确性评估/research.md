# 研究发现

| 关键词 | 来源 | 摘要 | 可信度 | 状态 |
|---|---|---|---|---|
| 运行入口 | `empty/User/CPU/scheduler.c:34-38,84-93` | 调度表只包含编码器测速、`test_speed_loop_refresh` 和测速打印；初始化只调用 `test_speed_loop_init`，未调用 `dal_gray_init` 或 `app_drive_mode_init`。 | 高 | 已确认 |
| 循迹输出 | `empty/User/APP/app_line_follow.c:216-269` | 循迹 APP 使用灰度快照计算外环输出，再转换为左右轮目标速度并调用两个速度 PID。 | 高 | 已确认 |
| 外环增益 | `empty/User/DAL/dal_pid.h:15-28`、`empty/User/APP/app_line_follow.c:226-233` | 线位最大约为 ±4，`Kp=0.0008`、Ki/Kd 均为 0，浮点输出最大约 ±0.0032；随后转换为 `int16_t turn_cp`，结果恒为 0。 | 高 | 已确认 |
| 灰度数据 | `empty/User/DAL/dal_gray.c:142-157` | 灰度快照仅由 `dal_gray_refresh()` 更新；当前调度表未登记该函数，`sequence` 会保持 0。 | 高 | 已确认 |
| 构建环境 | `.auto-embedded/tools/build-keil/scripts/keil_builder.py --detect` | 当前环境未发现 UV4.exe、ARMCC 或 ARMCLANG，不能执行 Keil 命令行构建。 | 高 | 阻塞 |
| 外环无差速 | `empty/User/DAL/dal_pid.h:16`、`empty/User/APP/app_line_follow.c:227-233`、`empty/User/Arithmetic/PID.c:519` | 当前 Kp 仍为 0.0008，零设定值下误差为 `-position`；线位范围 ±4 时输出范围仅 ±0.0032，强制转换为 `int16_t turn_cp` 后恒为 0，左右目标速度始终同为 20。 | 高 | 已确认根因 |
| 运行期日志 | COM15（USB 串行设备） | 串口存在但被其他程序占用，无法打开；正常循迹路径的调试打印代码目前也处于注释状态。 | 高 | 阻塞 |
| 当前参数未落盘 | `empty/User/DAL/dal_pid.h:16` | 工作区中的循迹外环 Kp 仍为 0.0008f，未见用户实测的 1~100 参数值。若实测使用过该范围，修改未保存到此文件、未构建进当前固件，或烧录目标不是该工作区产物。 | 高 | 已确认 |
| 右轮输出前置门 | `empty/User/APP/app_line_follow.c:235-269` | 任何一侧 `speed_valid` 为 false 时，函数直接调用 `dal_motor_stop_all()` 并返回，未执行左右速度 PID 和 `dal_motor_set_output()`；右编码器无效会表现为右轮无输出，但同时左轮也会被停止。 | 高 | 已确认 |
| 外环限幅结果 | `empty/User/DAL/dal_pid.h:25`、`empty/User/APP/app_line_follow.c:231-233` | 外环输出限幅为 ±18，基础速度为 20；即使 Kp=100，左右目标速度为 2 和 38，不会因外环本身得到负速度命令。低速侧 2 counts/周期可能无法克服静摩擦。 | 高 | 已确认 |
