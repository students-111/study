/**
 * @file MultiTimer.h
 * @brief 轻量级软件定时器链表调度接口。
 */

#ifndef MULTITIMER_H
#define MULTITIMER_H

/* ======== include ======== */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======== 可调参数宏定义 ======== */

/* 定时器接口成功返回值。 */
#define MULTI_TIMER_OK                 (0)

/* 定时器接口失败返回值。 */
#define MULTI_TIMER_ERR                (-1)

/* ======== 类型定义 ======== */

/**
 * @brief 平台 tick 获取函数类型。
 * @param void 无参数。
 * @return 当前平台 tick。
 */
typedef uint64_t (*multi_timer_ticks_fn_t)(void);

typedef struct multi_timer_s multi_timer_t;

/**
 * @brief 软件定时器到期回调函数类型。
 * @param timer 触发回调的软件定时器。
 * @param user_data 启动定时器时传入的用户数据。
 * @return 无。
 */
typedef void (*multi_timer_callback_t)(multi_timer_t *timer, void *user_data);

/**
 * @brief 软件定时器链表节点。
 */
struct multi_timer_s {
    multi_timer_t *next;               /**< 下一个定时器节点。 */
    uint64_t deadline;                 /**< 到期时间戳，单位由平台 tick 决定。 */
    multi_timer_callback_t callback;   /**< 到期回调函数。 */
    void *user_data;                   /**< 传给回调的用户数据。 */
};

/* ======== 公开 API ======== */

/**
 * @brief 安装平台 tick 获取函数。
 * @param ticks_func 平台 tick 获取函数。
 * @return 成功返回 MULTI_TIMER_OK，失败返回 MULTI_TIMER_ERR。
 */
int multi_timer_install(multi_timer_ticks_fn_t ticks_func);

/**
 * @brief 启动定时器并插入工作链表。
 * @param timer 定时器句柄。
 * @param timing 从当前 tick 开始的延迟时间。
 * @param callback 到期回调函数。
 * @param user_data 传给回调的用户数据。
 * @return 成功返回 MULTI_TIMER_OK，失败返回 MULTI_TIMER_ERR。
 */
int multi_timer_start(multi_timer_t *timer, uint64_t timing,
    multi_timer_callback_t callback, void *user_data);

/**
 * @brief 停止定时器并从工作链表移除。
 * @param timer 定时器句柄。
 * @return 成功返回 MULTI_TIMER_OK。
 */
int multi_timer_stop(multi_timer_t *timer);

/**
 * @brief 检查已到期定时器并执行回调。
 * @param void 无参数。
 * @return 下一定时器剩余 tick，失败返回 MULTI_TIMER_ERR。
 */
int multi_timer_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* MULTITIMER_H */
