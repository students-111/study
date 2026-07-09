/**
 * @file bsp_status.h
 * @brief BSP 层通用状态码定义。
 */

#ifndef BSP_STATUS_H
#define BSP_STATUS_H

/* ======== 可调参数宏定义 ======== */

/* ======== 类型定义 ======== */

/**
 * @brief BSP 通用返回码，用于屏蔽 DriverLib 或外设细节错误。
 */
typedef enum {
    BSP_OK = 0,          /**< 操作完成或异步事务已成功启动。 */
    BSP_ERR_BUSY,        /**< 外设或软件事务正忙。 */
    BSP_ERR_TIMEOUT,     /**< 外设操作超过限定时间。 */
    BSP_ERR_PARAM,       /**< 调用参数非法。 */
    BSP_ERR_HW           /**< NACK、仲裁丢失等硬件层错误。 */
} bsp_status_e;

#endif /* BSP_STATUS_H */
