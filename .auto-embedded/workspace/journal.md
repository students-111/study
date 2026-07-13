## #1  task=循迹逻辑正确性评估  phase=RESEARCH
break-loop: 循迹 PID 天文浮点根因=Keil 主栈仅 0x100 字节且向低地址溢出，覆盖紧邻的 uwTick 与 pid_output；证据=1.35542539e-19 的位型 0x20200538 对应 g_dal_encoder_instances[1]。防复发=链接后核对 map 的 STACK/关键全局布局并保留栈余量，已 promote 至 spec/conventions。

## #2  task=循迹赛道第二题闭环行驶方案  phase=INNOVATE
T-循迹赛道第二题闭环行驶方案：已完成需求收敛。路线采用灰度黑白边沿判点，白底走现有直线保持、弧线走现有循迹；每个边沿停车 1 s，Key1 启动，首版沿用现有速度。下一步：INNOVATE 比较状态机的模块落点与切换保护方案。

## #3  task=循迹赛道第二题闭环行驶方案  phase=PLAN
T-循迹赛道第二题闭环行驶方案：INNOVATE 已选定复用 app_drive_mode 的赛道状态机。直线只等待黑线出现，弧线只等待黑线消失；每次确认连续三帧后停车 1 s。已进入 PLAN，实施前仍待用户授权，不改生产代码。

## #4  task=循迹赛道第二题闭环行驶方案  phase=EXECUTE
T-循迹赛道第二题闭环行驶方案：已实现 app.[ch] 状态机并接入 scheduler，未使用 app_drive_mode。静态证据：Keil XML 可解析且包含 app.[ch]，本轮补丁 diff --check 通过；未构建原因：环境探测未发现 UV4.exe、ARMCC 或 ARMCLANG。下一步：在具备 Keil 环境的机器编译并上板验证四次暂停。

## #5  task=循迹赛道第二题闭环行驶方案  phase=EXECUTE
T-循迹赛道第二题闭环行驶方案：按 Key 原理图完成硬件迁移。Key1=PB21，内部上拉、低电平按下；GRAY_D2=PB0。已通过 SysConfig CLI 生成并核对宏，check.py --hw 通过。仍待具备 Keil 环境后的编译与上板验证。

## #6  task=循迹赛道第二题闭环行驶方案  phase=EXECUTE
T-循迹赛道第二题闭环行驶方案：已找到并修复 Keil 的 19 个报错。根因是 Key1 迁到 PB21 后所有 BOARD_GPIO 位于 GPIOB，SysConfig 只生成 BOARD_GPIO_PORT；BSP/DAL 已改用该宏。另以 bsp_time_get_ms() 移除 APP 层 extern 时基。E:\\Kill_5\\UV4\\UV4.exe 重建成功（0 error/0 warning），check.py ARCH/HW/SPEC 全通过。下一步上板验证 Key1 低有效与赛道状态机。
