
## #1  task=-  phase=-
break-loop: 循迹无法正常启动 根因=单键模式顺序 STOP->STRAIGHT->LINE_FOLLOW，第一下按键先进入仍在调试的直线模式，用户验收循迹时表现为未启动；证据=OpenOCD/GDB 读到灰度与 MPU 快照有效、按键边沿能命中 app_drive_mode_next；修复=改为 STOP->LINE_FOLLOW->STRAIGHT->STOP；防复发=已 promote 到 spec/conventions。

## #2  task=继续aemb任务  phase=REVIEW
review: 已修复 AEMB 门禁脚本 Windows 兼容性问题：PowerShell 入口增加 ExecutionPolicy Bypass，arch-check.ps1 改用兼容 .NET Framework 的相对路径计算，修复 mega-header 正则拼接，并将 empty/User/APP 纳入 APP 层扫描；同步更新 arch-check.sh 的 APP 层目录。验证=python .auto-embedded/scripts/check.py 输出 ARCH/HW/SPEC 全 PASS。构建=当前 PATH 未发现 UV4.exe/ARMCC/ARMCLANG，无法现场重编译；已有 Keil 日志显示 2026-07-09 10:42:57 生成 axf/hex，app_drive_mode.c 已参与编译，0 Error(s), 0 Warning(s)。

## #3  task=task1循迹启动验证  phase=REVIEW
runtime: 使用 OpenOCD + arm-none-eabi-gdb 对 empty/keil/Objects/empty_LP_MSPM0G3507_nortos_keil.axf 做 attach-only 运行态检查；OpenOCD 识别 CMSIS-DAPv2、SWD DPIDR=0x6ba02477、Cortex-M0+。GDB 复位运行 1s 后停核，PC 位于 cpu_run 主循环；g_app_drive_mode_current=APP_DRIVE_MODE_STOP，g_dal_gray_sample.sequence=953/raw_mask=249/active_count=6/line_detected=true/position=666，g_dal_key_sample[0].sequence=105/pressed=false/pressed_edge=false，g_dal_mpu6050_sample.sequence=105/yaw_calibrated=true。结论=传感器快照与调度运行有效，task1 原根因“第一下按键进入直线而非循迹”已由代码顺序修正；未做 RAM 写入或函数调用触发，避免车辆误动作。仍需人工按键实测确认第一下进入循迹。

## #4  task=-  phase=-
break-loop: 按键后小车无论怎么按都不动 根因=KEY 任务10ms刷新但行车模式1ms刷新，pressed_edge在同一key快照内保持约10ms，被app_drive_mode_refresh重复消费导致模式连续跳转并可能回到STOP；修复=app_drive_mode按g_dal_key_sample[KEY1].sequence记录已消费事件，同一sequence只切换一次；验证=Keil重编译0错误0警告、Keil烧录Verify OK、AEMB门禁全PASS；防复发=已promote到spec/conventions。
