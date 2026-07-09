/**
 * @file dal_gray.h
 * @brief 八路灰度传感器数据转换 DAL 接口。
 */

#ifndef DAL_GRAY_H
#define DAL_GRAY_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* 灰度传感器数量；bit0~bit7 分别对应 D1~D8。 */
#define DAL_GRAY_SENSOR_COUNT          (8U)

/* 判定十字或宽线的最小有效通道数。 */
#define DAL_GRAY_CROSS_MIN_COUNT       (6U)

/* D1~D8 线位权重数组初始化列表；范围 -4.0f~+4.0f，左负右正。 */
#define DAL_GRAY_WEIGHT_VALUES         -4.0f, -3.0f, -2.0f, -1.0f, \
                                        1.0f, 2.0f, 3.0f, 4.0f

/* ======== 类型定义 ======== */

/**
 * @brief 灰度传感器最新快照。
 */
typedef struct {
    uint8_t raw_mask;        /**< 八路灰度检测位图；bit0~bit7 对应 D1~D8，1 表示检测到线。 */
    uint8_t active_count;    /**< 当前检测到线的通道数量。 */
    bool line_detected;      /**< 是否检测到线。 */
    bool cross_detected;     /**< 是否检测到十字或宽线。 */
    float position;          /**< 加权线位，范围 -4.0f~+4.0f，左负右正。 */
    uint32_t sequence;       /**< 每次刷新递增的样本序号。 */
} dal_gray_sample_t;

/* ======== 公开 API ======== */

/**
 * @brief 灰度传感器最新快照。
 *
 * `dal_gray_refresh()` 负责更新该快照；APP 层只读，不写。
 * `sequence == 0U` 表示还没有完成过刷新。
 */
extern dal_gray_sample_t g_dal_gray_sample;

/**
 * @brief 初始化灰度传感器 DAL 快照状态。
 * @param void 无参数。
 * @return 无。
 */
void dal_gray_init(void);

/**
 * @brief 刷新一次灰度传感器数据。
 * @param void 无参数。
 * @return 无。
 */
void dal_gray_refresh(void);

#endif /* DAL_GRAY_H */
