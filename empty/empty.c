/**
 * @file empty.c
 * @brief 固件主入口。
 */

/* ======== include ======== */

#include "scheduler.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 公开 API ======== */

int main(void)
{
    scheduler_init();
    scheduler_run();
}









