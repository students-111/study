/**
 * @file empty.c
 * @brief 固件主入口。
 */

/* ======== include ======== */

#include "cpu_scheduler.h"

/* ======== 可调参数宏定义 ======== */

/* ======== 公开 API ======== */

int main(void)
{
    cpu_init();
    cpu_run();
}









