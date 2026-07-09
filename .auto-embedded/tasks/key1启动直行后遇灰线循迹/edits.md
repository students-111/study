# 编辑清单

| 文件 | 改动 | 验证标准 | 结果 | commit |
|---|---|---|---|---|
| `empty/User/APP/app_drive_mode.c` | 将 Key1 逻辑从停止/循迹/直行轮换改为仅在停止态启动直行；直行态检测到灰线后自动切入循迹。 | `rg` 确认旧轮换函数移除，新状态转换函数接入；`.auto-embedded/scripts/check.py` 通过。 | 已通过本地一致性检查；Keil 编译因本机缺 UV4/ARMCLANG 未执行。 | 未提交 |
| `empty/User/APP/app_drive_mode.h` | 更新 `app_drive_mode_refresh()` 注释，说明 Key1 启动和灰线自动切换语义。 | 头文件公开 API 注释与实现行为一致。 | 已检查。 | 未提交 |
| `.auto-embedded/tasks/key1启动直行后遇灰线循迹/prd.md` | 记录 MVP 需求、验收标准和约束。 | PRD 覆盖用户确认的 Key1 不停车、不轮换需求。 | 已记录。 | 未提交 |
