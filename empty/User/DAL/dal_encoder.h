/**
 * @file dal_encoder.h
 * @brief 编码器正交解码 DAL 接口。
 */

#ifndef DAL_ENCODER_H
#define DAL_ENCODER_H

/* ======== include ======== */

#include <stdbool.h>
#include <stdint.h>

#include "../Driver/drv_encoder.h"

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
 * @brief 编码器最新解码样本。
 */
typedef struct {
    int32_t count;                       /**< 累计编码器计数。 */
    int16_t delta;                       /**< 本次刷新产生的计数增量。 */
    int16_t speed_cp;                    /**< 测速周期内计数增量，单位 counts/speed-period。 */
    EncoderState_t state;                /**< 驱动层编码器状态。 */
    uint8_t raw_state;                   /**< 当前 A/B 相压缩状态。 */
    bool speed_valid;                    /**< 速度是否已建立上一帧计数基准。 */
    uint32_t sequence;                   /**< 每次刷新递增的样本序号。 */
} dal_encoder_sample_t;

/* ======== 公开 API ======== */

/**
 * @brief 编码器最新快照。
 *
 * GPIO 双边沿中断负责刷新计数和状态，`dal_encoder_update_speed()`
 * 每 10 ms 刷新速度；上层只读，不写。
 * `sequence == 0U` 表示该通道还没有完成过刷新。
 */
extern volatile dal_encoder_sample_t g_dal_encoder_sample[DAL_ENCODER_COUNT];

/**
 * @brief 初始化编码器解码状态。
 * @param void 无参数。
 * @return 无。
 */
void dal_encoder_init(void);

/**
 * @brief 手动刷新所有编码器通道的正交解码状态。
 *
 * 正常运行由 GPIO 双边沿中断刷新，此接口仅保留给兼容测试使用。
 * @param void 无参数。
 * @return 无。
 */
void dal_encoder_refresh(void);

/**
 * @brief 刷新所有编码器通道的测速快照。
 * @param void 无参数。
 * @return 无。
 */
void dal_encoder_update_speed(void);

/**
 * @brief 清零指定编码器累计计数和运行状态。
 * @param id 编码器通道编号。
 * @return 无。
 */
void dal_encoder_reset(dal_encoder_id_e id);

#endif /* DAL_ENCODER_H */
