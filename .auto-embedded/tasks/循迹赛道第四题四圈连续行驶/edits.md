# 编辑清单

| 文件 | 改动 | 验证标准 | 结果 | commit |
|---|---|---|---|---|
| `empty/User/APP/app_task4.[ch]` | 新增第四题四圈连续状态机，负责 Key1 启动、低速差速转向/找线、圈数计数与第四圈零目标停车。 | A→C→B→D→A 连续四圈；仅第四圈在 A 停车。 | 已完成代码与编译验证；待上板验收。 | 未提交 |
| `empty/User/APP/app.[ch]` | 增加 `APP_TASK_4`，并把 task4 纳入 APP 初始化与周期调度。 | 选择 `APP_TASK_4` 后 task2/task3 不参与调度。 | 已完成。 | 未提交 |
| `empty/User/APP/app_line_follow.[ch]` | 增加速度 PID 零目标保持接口，供正常最终停车使用。 | 正常停车不直接失能电机。 | 已完成。 | 未提交 |
| `empty/keil/empty_LP_MSPM0G3507_nortos_keil.uvprojx` | 将 `app_task4.c` 加入 Keil 编译清单。 | Keil 能编译、链接新模块。 | 0 error / 0 warning。 | 未提交 |
| `empty/User/APP/app_task4.c` | 灰度黑白边沿确认改为仅消费新的灰度快照序号，防止同一帧抖动被重复累计。 | 连续 3 个独立灰度样本才允许 B 点离线并进入 B→D 转向。 | 已完成编译验证；待上板验证。 | 未提交 |
| `empty/User/APP/app.c` | task2/task3 的共用灰度确认同步改为按新快照计数。 | 同类状态机不再重复消费同一灰度样本。 | 已完成编译验证。 | 未提交 |
| `empty/User/APP/app_task4.[ch]` | 新增 300 ms 弧线捕线期；捕线期丢线回到对应低速找线，稳定后才允许离线判为 B/D 终点。 | C/D 入弧偏差或短暂丢线不触发跨地形转向。 | 已完成编译验证；待上板验证。 | 未提交 |
