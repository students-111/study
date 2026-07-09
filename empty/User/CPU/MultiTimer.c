/**
 * @file MultiTimer.c
 * @brief 轻量级软件定时器链表调度实现。
 */

/* ======== include ======== */

#include "MultiTimer.h"

#include <stddef.h>

/* ======== 可调参数宏定义 ======== */

/* ======== 内部变量 ======== */

static multi_timer_t *g_multi_timer_list = NULL;
static multi_timer_ticks_fn_t g_multi_timer_ticks_func = NULL;

/* ======== 内部函数 ======== */

/**
 * @brief 从定时器链表中移除指定定时器。
 * @param timer 定时器句柄。
 * @return 无。
 */
static void multi_timer_remove(multi_timer_t *timer)
{
    multi_timer_t **current = &g_multi_timer_list;

    while (*current != NULL) {
        if (*current == timer) {
            *current = timer->next;
            break;
        }
        current = &(*current)->next;
    }
}

/* ======== 公开 API ======== */

int multi_timer_install(multi_timer_ticks_fn_t ticks_func)
{
    if (ticks_func == NULL) {
        return MULTI_TIMER_ERR;
    }

    g_multi_timer_ticks_func = ticks_func;
    return MULTI_TIMER_OK;
}

int multi_timer_start(multi_timer_t *timer, uint64_t timing,
    multi_timer_callback_t callback, void *user_data)
{
    multi_timer_t **current;

    if ((timer == NULL) || (callback == NULL) ||
        (g_multi_timer_ticks_func == NULL)) {
        return MULTI_TIMER_ERR;
    }

    multi_timer_remove(timer);

    timer->deadline = g_multi_timer_ticks_func() + timing;
    timer->callback = callback;
    timer->user_data = user_data;

    current = &g_multi_timer_list;
    while ((*current != NULL) && ((*current)->deadline < timer->deadline)) {
        current = &(*current)->next;
    }
    timer->next = *current;
    *current = timer;

    return MULTI_TIMER_OK;
}

int multi_timer_stop(multi_timer_t *timer)
{
    multi_timer_remove(timer);
    return MULTI_TIMER_OK;
}

int multi_timer_yield(void)
{
    uint64_t current_ticks;

    if (g_multi_timer_ticks_func == NULL) {
        return MULTI_TIMER_ERR;
    }

    current_ticks = g_multi_timer_ticks_func();
    while ((g_multi_timer_list != NULL) &&
           (current_ticks >= g_multi_timer_list->deadline)) {
        multi_timer_t *timer = g_multi_timer_list;

        g_multi_timer_list = timer->next;

        if (timer->callback != NULL) {
            timer->callback(timer, timer->user_data);
        }
    }

    return (g_multi_timer_list != NULL) ?
        (int)(g_multi_timer_list->deadline - current_ticks) : 0;
}
