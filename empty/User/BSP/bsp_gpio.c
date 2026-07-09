/**
 * @file bsp_gpio.c
 * @brief 通用 GPIO BSP 实现。
 */

/* ======== include ======== */

#include "bsp_gpio.h"

#include <stdint.h>

#include "ti_msp_dl_config.h"

/* ======== 类型定义 ======== */

/**
 * @brief BSP GPIO 描述表项。
 */
typedef struct {
    GPIO_Regs *port;       /**< DriverLib GPIO 端口。 */
    uint32_t pin;          /**< DriverLib GPIO pin mask。 */
    uint32_t pincm;        /**< SysConfig 生成的 PINCM 编号。 */
} bsp_gpio_desc_t;

/* ======== 内部变量 ======== */

static const bsp_gpio_desc_t g_bsp_gpio_desc[BSP_GPIO_COUNT] = {
    [BSP_GPIO_KEY1] = {
        BOARD_GPIO_KEY1_PORT,
        BOARD_GPIO_KEY1_PIN,
        BOARD_GPIO_KEY1_IOMUX
    },
    [BSP_GPIO_MOTOR_AIN1] = {
        BOARD_GPIO_MOTOR_AIN1_PORT,
        BOARD_GPIO_MOTOR_AIN1_PIN,
        BOARD_GPIO_MOTOR_AIN1_IOMUX
    },
    [BSP_GPIO_MOTOR_AIN2] = {
        BOARD_GPIO_MOTOR_AIN2_PORT,
        BOARD_GPIO_MOTOR_AIN2_PIN,
        BOARD_GPIO_MOTOR_AIN2_IOMUX
    },
    [BSP_GPIO_MOTOR_BIN1] = {
        BOARD_GPIO_MOTOR_BIN1_PORT,
        BOARD_GPIO_MOTOR_BIN1_PIN,
        BOARD_GPIO_MOTOR_BIN1_IOMUX
    },
    [BSP_GPIO_MOTOR_BIN2] = {
        BOARD_GPIO_MOTOR_BIN2_PORT,
        BOARD_GPIO_MOTOR_BIN2_PIN,
        BOARD_GPIO_MOTOR_BIN2_IOMUX
    },
    [BSP_GPIO_ENC_M2_A] = {
        BOARD_GPIO_ENC_M2_A_PORT,
        BOARD_GPIO_ENC_M2_A_PIN,
        BOARD_GPIO_ENC_M2_A_IOMUX
    },
    [BSP_GPIO_ENC_M2_B] = {
        BOARD_GPIO_ENC_M2_B_PORT,
        BOARD_GPIO_ENC_M2_B_PIN,
        BOARD_GPIO_ENC_M2_B_IOMUX
    },
    [BSP_GPIO_MOTOR_STBY] = {
        BOARD_GPIO_MOTOR_STBY_PORT,
        BOARD_GPIO_MOTOR_STBY_PIN,
        BOARD_GPIO_MOTOR_STBY_IOMUX
    },
    [BSP_GPIO_ENC_M1_A] = {
        BOARD_GPIO_ENC_M1_A_PORT,
        BOARD_GPIO_ENC_M1_A_PIN,
        BOARD_GPIO_ENC_M1_A_IOMUX
    },
    [BSP_GPIO_ENC_M1_B] = {
        BOARD_GPIO_ENC_M1_B_PORT,
        BOARD_GPIO_ENC_M1_B_PIN,
        BOARD_GPIO_ENC_M1_B_IOMUX
    },
    [BSP_GPIO_GRAY_D1] = {
        BOARD_GPIO_GRAY_D1_PORT,
        BOARD_GPIO_GRAY_D1_PIN,
        BOARD_GPIO_GRAY_D1_IOMUX
    },
    [BSP_GPIO_GRAY_D2] = {
        BOARD_GPIO_GRAY_D2_PORT,
        BOARD_GPIO_GRAY_D2_PIN,
        BOARD_GPIO_GRAY_D2_IOMUX
    },
    [BSP_GPIO_GRAY_D3] = {
        BOARD_GPIO_GRAY_D3_PORT,
        BOARD_GPIO_GRAY_D3_PIN,
        BOARD_GPIO_GRAY_D3_IOMUX
    },
    [BSP_GPIO_GRAY_D4] = {
        BOARD_GPIO_GRAY_D4_PORT,
        BOARD_GPIO_GRAY_D4_PIN,
        BOARD_GPIO_GRAY_D4_IOMUX
    },
    [BSP_GPIO_GRAY_D5] = {
        BOARD_GPIO_GRAY_D5_PORT,
        BOARD_GPIO_GRAY_D5_PIN,
        BOARD_GPIO_GRAY_D5_IOMUX
    },
    [BSP_GPIO_GRAY_D6] = {
        BOARD_GPIO_GRAY_D6_PORT,
        BOARD_GPIO_GRAY_D6_PIN,
        BOARD_GPIO_GRAY_D6_IOMUX
    },
    [BSP_GPIO_GRAY_D7] = {
        BOARD_GPIO_GRAY_D7_PORT,
        BOARD_GPIO_GRAY_D7_PIN,
        BOARD_GPIO_GRAY_D7_IOMUX
    },
    [BSP_GPIO_GRAY_D8] = {
        BOARD_GPIO_GRAY_D8_PORT,
        BOARD_GPIO_GRAY_D8_PIN,
        BOARD_GPIO_GRAY_D8_IOMUX
    },
};

/* ======== 内部函数 ======== */

/**
 * @brief 判断 GPIO 引脚编号是否合法。
 * @param pin BSP GPIO 引脚。
 * @return 合法返回 true，否则返回 false。
 */
static bool bsp_gpio_pin_is_valid(bsp_gpio_pin_e pin)
{
    return ((unsigned int)pin < (unsigned int)BSP_GPIO_COUNT);
}

/**
 * @brief 将 BSP 上下拉配置转换为 DriverLib 参数。
 * @param pull BSP 上下拉配置。
 * @param out 输出 DriverLib 上下拉配置。
 * @return 成功返回 BSP_OK，参数非法返回 BSP_ERR_PARAM。
 */
static bsp_status_e bsp_gpio_to_dl_pull(bsp_gpio_pull_e pull,
    DL_GPIO_RESISTOR *out)
{
    if (out == NULL) {
        return BSP_ERR_PARAM;
    }

    switch (pull) {
    case BSP_GPIO_PULL_NONE:
        *out = DL_GPIO_RESISTOR_NONE;
        break;
    case BSP_GPIO_PULL_UP:
        *out = DL_GPIO_RESISTOR_PULL_UP;
        break;
    case BSP_GPIO_PULL_DOWN:
        *out = DL_GPIO_RESISTOR_PULL_DOWN;
        break;
    default:
        return BSP_ERR_PARAM;
    }

    return BSP_OK;
}

/* ======== 公开 API ======== */

bsp_status_e bsp_gpio_init_input(bsp_gpio_pin_e pin, bsp_gpio_pull_e pull,
    bool hysteresis_enable)
{
    const bsp_gpio_desc_t *desc;
    DL_GPIO_RESISTOR dl_pull;

    if (!bsp_gpio_pin_is_valid(pin)) {
        return BSP_ERR_PARAM;
    }
    if (bsp_gpio_to_dl_pull(pull, &dl_pull) != BSP_OK) {
        return BSP_ERR_PARAM;
    }

    desc = &g_bsp_gpio_desc[pin];
    DL_GPIO_initDigitalInputFeatures(desc->pincm, DL_GPIO_INVERSION_DISABLE,
        dl_pull,
        hysteresis_enable ? DL_GPIO_HYSTERESIS_ENABLE
                          : DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_disableOutput(desc->port, desc->pin);

    return BSP_OK;
}

bsp_status_e bsp_gpio_init_output(bsp_gpio_pin_e pin)
{
    const bsp_gpio_desc_t *desc;

    if (!bsp_gpio_pin_is_valid(pin)) {
        return BSP_ERR_PARAM;
    }

    desc = &g_bsp_gpio_desc[pin];
    DL_GPIO_initDigitalOutputFeatures(desc->pincm, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_DRIVE_STRENGTH_HIGH,
        DL_GPIO_HIZ_DISABLE);
    DL_GPIO_clearPins(desc->port, desc->pin);
    DL_GPIO_enableOutput(desc->port, desc->pin);

    return BSP_OK;
}

bsp_status_e bsp_gpio_read(bsp_gpio_pin_e pin, bool *out)
{
    const bsp_gpio_desc_t *desc;

    if (!bsp_gpio_pin_is_valid(pin) || (out == NULL)) {
        return BSP_ERR_PARAM;
    }

    desc = &g_bsp_gpio_desc[pin];
    *out = ((DL_GPIO_readPins(desc->port, desc->pin) & desc->pin) != 0U);
    return BSP_OK;
}

bsp_status_e bsp_gpio_write(bsp_gpio_pin_e pin, bool high)
{
    const bsp_gpio_desc_t *desc;

    if (!bsp_gpio_pin_is_valid(pin)) {
        return BSP_ERR_PARAM;
    }

    desc = &g_bsp_gpio_desc[pin];
    if (high) {
        DL_GPIO_setPins(desc->port, desc->pin);
    } else {
        DL_GPIO_clearPins(desc->port, desc->pin);
    }

    return BSP_OK;
}
