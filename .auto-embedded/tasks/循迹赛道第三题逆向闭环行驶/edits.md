# 编辑清单

| 文件 | 改动 | 验证标准 | 结果 | commit |
|---|---|---|---|---|
| `empty/User/APP/app.h` | 增加 task2/task3 选择、task3 相对转角、转向输出、容差与正负方向宏；默认启用 task3。 | 所有 task3 初始转向参数集中可调，切换 task 仅修改 `APP_ACTIVE_TASK`。 | Keil 重建通过；ARCH/HW/SPEC 通过 | 未提交 |
| `empty/User/APP/app.c` | 保留 task2 状态机，新增 task3 原地转向、直行、循迹、节点停车和最终停车状态机。 | Key1 启动；C/B/D 各暂停 1 s；转向由相对 Yaw 闭环停止。 | Keil 重建通过；待上板验证 | 未提交 |
| `empty/User/APP/app.c`、`empty/User/APP/app.h` | C/D 入弧改为先离开到点黑线、再重新捕获弧线；增加捕线方向与 5 s 超时参数。 | 到点黑线不直接启动循迹；重新捕获弧线后才进入循迹。 | Keil 重建通过；ARCH/HW/SPEC 通过；待上板验证 | 未提交 |
| `empty/User/APP/app.c` | C/D 停车结束后按当前灰度状态选择入弧方式。 | 已检测到线直接循迹；未检测到线才自转找线。 | Keil 重建通过；ARCH/HW/SPEC 通过；待上板验证 | 未提交 |
| `empty/User/APP/app.c`、`empty/User/APP/app_line_follow.[ch]` | 正常到点、暂停、最终停车与灰线丢失时改为速度 PID 零目标保持。 | 不直接失能电机；异常保护仍硬停止。 | Keil 重建通过；ARCH/HW/SPEC 通过；待上板验证 | 未提交 |
| `CODE_STYLE_AND_ARCHITECTURE.md` | 记录多赛题 APP 的“任务选择 + 独立状态机 + 共用机制”组织约定。 | 规范反映当前 APP 组织方式。 | 已更新 | 未提交 |
