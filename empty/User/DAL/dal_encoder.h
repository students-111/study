/**
 * @file dal_encoder.h
 * @brief 编码器正交解码 DAL 接口。
 */

#ifndef DAL_ENCODER_H
#define DAL_ENCODER_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

/**
 * @brief 编码器 DAL 通道编号。
 */
typedef enum {
    DAL_ENCODER_M1 = 0,          /**< M1 编码器通道。 */
    DAL_ENCODER_M2,              /**< M2 编码器通道。 */
    DAL_ENCODER_COUNT            /**< 编码器通道数量。 */
} dal_encoder_id_e;

/**
 * @brief 编码器运动方向。
 */
typedef enum {
    DAL_ENCODER_DIR_STOP = 0,    /**< 本周期无位移。 */
    DAL_ENCODER_DIR_FORWARD,     /**< 本周期正向位移。 */
    DAL_ENCODER_DIR_REVERSE,     /**< 本周期反向位移。 */
    DAL_ENCODER_DIR_ERROR        /**< BSP 读取失败或状态异常。 */
} dal_encoder_direction_e;

/**
 * @brief 编码器最新解码样本。
 */
typedef struct {
    int32_t count;                       /**< 累计编码器计数。 */
    int16_t delta;                       /**< 本次刷新产生的计数增量。 */
    dal_encoder_direction_e direction;   /**< 本次刷新判定方向。 */
    uint8_t raw_state;                   /**< 当前 A/B 相压缩状态。 */
    uint32_t sequence;                   /**< 每次刷新递增的样本序号。 */
} dal_encoder_sample_t;

/* ======== 公开 API ======== */

/**
 * @brief 编码器最新快照。
 *
 * `dal_encoder_refresh()` 负责刷新该数组；上层只读，不写。
 * `sequence == 0U` 表示该通道还没有完成过刷新。
 */
extern dal_encoder_sample_t g_dal_encoder_sample[DAL_ENCODER_COUNT];

/**
 * @brief 初始化编码器解码状态。
 * @param void 无参数。
 * @return 无。
 */
void dal_encoder_init(void);

/**
 * @brief 刷新所有编码器通道的正交解码状态。
 * @param void 无参数。
 * @return 无。
 */
void dal_encoder_refresh(void);

/**
 * @brief 清零指定编码器累计计数和运行状态。
 * @param id 编码器通道编号。
 * @return 无。
 */
void dal_encoder_reset(dal_encoder_id_e id);

#endif /* DAL_ENCODER_H */
