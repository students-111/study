# 研究发现

| 关键词 | 来源 | 摘要 | 可信度 | 状态 |
|---|---|---|---|---|
| 当前行车入口 | `empty/User/APP/app_drive_mode.c` | 原逻辑通过 Key1 在停止、循迹、直行之间轮换；本次需求要求取消轮换。 | 高 | 已处理 |
| 灰度快照 | `empty/User/DAL/dal_gray.h` | APP 可只读 `g_dal_gray_sample`，`sequence != 0U` 表示已有刷新，`line_detected` 表示检测到线。 | 高 | 已复用 |
| 直行/循迹子模式 | `empty/User/APP/app_straight_drive.c`、`empty/User/APP/app_line_follow.c` | 直行已有 `app_straight_drive_enter/refresh/stop`，循迹已有 `app_line_follow_refresh/stop`，无需新增硬件抽象。 | 高 | 已复用 |
