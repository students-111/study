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

## #7  task=循迹赛道第三题逆向闭环行驶  phase=PLAN
T-循迹赛道第三题逆向闭环行驶：需求已收敛。使用 app.c 编排四次相对 Yaw 原地转向，白底直线用现有直线保持、半圆弧线用既有循迹；C/B/D 各停 1 s，A 最终停车。初始转角为几何预测值，待上板经验调参。

## #8  task=循迹赛道第三题逆向闭环行驶  phase=EXECUTE
T-循迹赛道第三题逆向闭环行驶：已在 app.c 保留 task2 并新增 task3，APP_ACTIVE_TASK 默认选择 task3。task3 使用相对 Yaw 原地转向、白底直行、弧线循迹和 C/B/D 各 1 s 暂停；转角/转向方向/输出/容差均为 app.h 宏。E:\\Kill_5\\UV4\\UV4.exe 重建 0 error/0 warning，check.py ARCH/HW/SPEC 全通过。下一步上板确认 Yaw 正负方向并经验微调四个转角。

## #9  task=循迹赛道第三题逆向闭环行驶  phase=EXECUTE
T-循迹赛道第三题逆向闭环行驶：根据上板反馈修正 C/D 入弧逻辑。原固定角后直接循迹会在灰度未压到弧线时立即停车；现改为先离开端点黑线、再捕获弧线才循迹，5 s 未捕线安全停车。Keil 重建 0 error/0 warning，check.py 全通过。

## #10  task=循迹赛道第三题逆向闭环行驶  phase=EXECUTE
T-循迹赛道第三题逆向闭环行驶：用户澄清 C/D 检测到的黑线即目标弧线。已删除 C/D 离线找线逻辑：C/D 停 1 s 后直接进入对应循迹；仅 A/B 保留相对 Yaw 定角转向。Keil 重建 0 error/0 warning，check.py 全通过。

## #11  task=循迹赛道第三题逆向闭环行驶  phase=EXECUTE
T-循迹赛道第三题逆向闭环行驶：已按用户最终澄清恢复 C/D 的正确流程。到点黑线只确认 C/D；停 1 s 后原地转离端点黑线，再重新捕获弧线才进入循迹。Keil 重建 0 error/0 warning，check.py 全通过。

## #12  task=循迹赛道第三题逆向闭环行驶  phase=EXECUTE
T-循迹赛道第三题逆向闭环行驶：C/D 入弧最终策略已实现。停 1 s 后当前灰度仍检测到线则直接循迹；未检测到则原地找线并捕获后循迹。Keil 重建 0 error/0 warning，check.py 全通过。

## #13  task=循迹赛道第三题逆向闭环行驶  phase=EXECUTE
T-循迹赛道第三题逆向闭环行驶：正常到点、节点暂停、最终停车及灰线丢失已改为左右轮速度 PID 零目标保持；不再直接失能电机。IMU 未校准和找线超时等异常仍硬停止。Keil 重建 0 error/0 warning，check.py 全通过。

## #14  task=循迹赛道第四题四圈连续行驶  phase=PLAN
T-循迹赛道第四题四圈连续行驶：需求已收敛。按第三题路线连续 4 圈，C/B/D 与前三圈 A 无 1 s 暂停；节点允许低速连续差速转向找线，基础速度初值 10，直线/弧线保持当前速度。D→A 离线记一圈，第四圈才停车。下一步：讨论 task4 状态机与低速转向控制实现。

## #15  task=循迹赛道第四题四圈连续行驶  phase=EXECUTE
T-循迹赛道第四题四圈连续行驶：已新增 app_task4.[ch] 并接入 app 调度，默认选择 APP_TASK_4。状态机按 A→C→B→D→A 循环；C/D 已在线直接循迹，离线才用基础速度 10、差速 5 的低速 PID 找线；D→A 离线累计圈数，第 4 圈左右轮 PID 目标归零停车。E:\\Kill_5\\UV4\\UV4.exe 重建 0 error/0 warning，check.py ARCH/HW/SPEC 全通过。待上板确认转向正负、转角、差速和找线方向。

## #16  task=循迹赛道第四题四圈连续行驶  phase=EXECUTE
break-loop: 第四题 B 点偶发向地图外转向根因=灰度连续确认按 APP 循环而非新灰度样本计数，单帧白色抖动被重复累计为离线；修复=app_task4 仅在 sequence 更新时累计并在状态切换时重置已消费序号；防复发=已 promote 至 spec/conventions。

## #17  task=循迹赛道第四题四圈连续行驶  phase=EXECUTE
T-循迹赛道第四题四圈连续行驶：已修复 B 点偶发过早进入 B→D 转向的问题。根因是灰度三次确认重复消费同一快照；现仅在灰度 sequence 更新时才计数，并同步修复 task2/task3 共用确认逻辑。Keil 重建 0 error/0 warning，check.py ARCH/HW/SPEC 全通过。待上板连续运行验证 B 点不再误转向。

## #18  task=循迹赛道第四题四圈连续行驶  phase=EXECUTE
T-循迹赛道第四题四圈连续行驶：针对入弧位置不准导致跨地形的问题，C→B、D→A 已加入 300 ms 捕线期。捕线期离开黑线时回到对应低速找线，只有稳定循迹后离线才分别判定 B/D 或完成一圈。Keil 重建 0 error/0 warning，check.py ARCH/HW/SPEC 全通过。待上板验证入弧能重新咬线且正常终点仍能切换。
