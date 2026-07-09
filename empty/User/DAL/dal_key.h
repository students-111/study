/**
 * @file dal_key.h
 * @brief 板载功能按键 DAL 接口。
 */

#ifndef DAL_KEY_H
#define DAL_KEY_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* Key1 消抖确认周期数，单位为刷新次数；连续稳定达到该值才更新按键状态。 */
#define DAL_KEY_DEBOUNCE_TICKS        (3U)

/* ======== 类型定义 ======== */

/**
 * @brief 功能按键编号。
 */
typedef enum {
    DAL_KEY_KEY1 = 0,     /**< 板载 Key1。 */
    DAL_KEY_COUNT         /**< 功能按键数量。 */
} dal_key_id_e;

/**
 * @brief 功能按键最新快照。
 */
typedef struct {
    bool pressed;         /**< 当前消抖后的按下状态。 */
    bool pressed_edge;    /**< 本次刷新是否产生按下沿。 */
    uint32_t sequence;    /**< 每次刷新递增的样本序号。 */
} dal_key_sample_t;

/* ======== 全局实例 ======== */

/**
 * @brief 功能按键最新快照数组。
 *
 * `dal_key_refresh()` 负责更新该数组；APP 层只读，不写。
 */
extern dal_key_sample_t g_dal_key_sample[DAL_KEY_COUNT];

/* ======== 公开 API ======== */

/**
 * @brief 初始化功能按键 GPIO 和快照状态。
 * @param void 无参数。
 * @return 无。
 */
void dal_key_init(void);

/**
 * @brief 刷新功能按键消抖状态。
 * @param void 无参数。
 * @return 无。
 */
void dal_key_refresh(void);

#endif /* DAL_KEY_H */
