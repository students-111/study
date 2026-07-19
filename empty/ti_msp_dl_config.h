/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2000
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for MOTOR_PWM */
#define MOTOR_PWM_INST                                                     TIMG8
#define MOTOR_PWM_INST_IRQHandler                               TIMG8_IRQHandler
#define MOTOR_PWM_INST_INT_IRQN                                 (TIMG8_INT_IRQn)
#define MOTOR_PWM_INST_CLK_FREQ                                         40000000
/* GPIO defines for channel 0 */
#define GPIO_MOTOR_PWM_C0_PORT                                             GPIOB
#define GPIO_MOTOR_PWM_C0_PIN                                      DL_GPIO_PIN_6
#define GPIO_MOTOR_PWM_C0_IOMUX                                  (IOMUX_PINCM23)
#define GPIO_MOTOR_PWM_C0_IOMUX_FUNC                 IOMUX_PINCM23_PF_TIMG8_CCP0
#define GPIO_MOTOR_PWM_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_MOTOR_PWM_C1_PORT                                             GPIOB
#define GPIO_MOTOR_PWM_C1_PIN                                      DL_GPIO_PIN_7
#define GPIO_MOTOR_PWM_C1_IOMUX                                  (IOMUX_PINCM24)
#define GPIO_MOTOR_PWM_C1_IOMUX_FUNC                 IOMUX_PINCM24_PF_TIMG8_CCP1
#define GPIO_MOTOR_PWM_C1_IDX                                DL_TIMER_CC_1_INDEX




/* Defines for I2C_JY901P */
#define I2C_JY901P_INST                                                     I2C0
#define I2C_JY901P_INST_IRQHandler                               I2C0_IRQHandler
#define I2C_JY901P_INST_INT_IRQN                                   I2C0_INT_IRQn
#define I2C_JY901P_BUS_SPEED_HZ                                           100000
#define GPIO_I2C_JY901P_SDA_PORT                                           GPIOA
#define GPIO_I2C_JY901P_SDA_PIN                                    DL_GPIO_PIN_0
#define GPIO_I2C_JY901P_IOMUX_SDA                                 (IOMUX_PINCM1)
#define GPIO_I2C_JY901P_IOMUX_SDA_FUNC                  IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_I2C_JY901P_SCL_PORT                                           GPIOA
#define GPIO_I2C_JY901P_SCL_PIN                                    DL_GPIO_PIN_1
#define GPIO_I2C_JY901P_IOMUX_SCL                                 (IOMUX_PINCM2)
#define GPIO_I2C_JY901P_IOMUX_SCL_FUNC                  IOMUX_PINCM2_PF_I2C0_SCL


/* Defines for UART_DEBUG */
#define UART_DEBUG_INST                                                    UART0
#define UART_DEBUG_INST_FREQUENCY                                       40000000
#define UART_DEBUG_INST_IRQHandler                              UART0_IRQHandler
#define UART_DEBUG_INST_INT_IRQN                                  UART0_INT_IRQn
#define GPIO_UART_DEBUG_RX_PORT                                            GPIOA
#define GPIO_UART_DEBUG_TX_PORT                                            GPIOA
#define GPIO_UART_DEBUG_RX_PIN                                    DL_GPIO_PIN_11
#define GPIO_UART_DEBUG_TX_PIN                                    DL_GPIO_PIN_10
#define GPIO_UART_DEBUG_IOMUX_RX                                 (IOMUX_PINCM22)
#define GPIO_UART_DEBUG_IOMUX_TX                                 (IOMUX_PINCM21)
#define GPIO_UART_DEBUG_IOMUX_RX_FUNC                  IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_DEBUG_IOMUX_TX_FUNC                  IOMUX_PINCM21_PF_UART0_TX
#define UART_DEBUG_BAUD_RATE                                            (115200)
#define UART_DEBUG_IBRD_40_MHZ_115200_BAUD                                  (21)
#define UART_DEBUG_FBRD_40_MHZ_115200_BAUD                                  (45)





/* Defines for DMA_CH0 */
#define DMA_CH0_CHAN_ID                                                      (0)
#define UART_DEBUG_INST_DMA_TRIGGER                          (DMA_UART0_RX_TRIG)


/* Port definition for Pin Group BOARD_GPIO */
#define BOARD_GPIO_PORT                                                  (GPIOB)

/* Defines for KEY1: GPIOB.21 with pinCMx 49 on package pin 20 */
#define BOARD_GPIO_KEY1_PIN                                     (DL_GPIO_PIN_21)
#define BOARD_GPIO_KEY1_IOMUX                                    (IOMUX_PINCM49)
/* Defines for MOTOR_AIN1: GPIOB.8 with pinCMx 25 on package pin 60 */
#define BOARD_GPIO_MOTOR_AIN1_PIN                                (DL_GPIO_PIN_8)
#define BOARD_GPIO_MOTOR_AIN1_IOMUX                              (IOMUX_PINCM25)
/* Defines for MOTOR_AIN2: GPIOB.9 with pinCMx 26 on package pin 61 */
#define BOARD_GPIO_MOTOR_AIN2_PIN                                (DL_GPIO_PIN_9)
#define BOARD_GPIO_MOTOR_AIN2_IOMUX                              (IOMUX_PINCM26)
/* Defines for MOTOR_BIN1: GPIOB.10 with pinCMx 27 on package pin 62 */
#define BOARD_GPIO_MOTOR_BIN1_PIN                               (DL_GPIO_PIN_10)
#define BOARD_GPIO_MOTOR_BIN1_IOMUX                              (IOMUX_PINCM27)
/* Defines for MOTOR_BIN2: GPIOB.11 with pinCMx 28 on package pin 63 */
#define BOARD_GPIO_MOTOR_BIN2_PIN                               (DL_GPIO_PIN_11)
#define BOARD_GPIO_MOTOR_BIN2_IOMUX                              (IOMUX_PINCM28)
/* Defines for MOTOR_STBY: GPIOB.14 with pinCMx 31 on package pin 2 */
#define BOARD_GPIO_MOTOR_STBY_PIN                               (DL_GPIO_PIN_14)
#define BOARD_GPIO_MOTOR_STBY_IOMUX                              (IOMUX_PINCM31)
/* Defines for ENC_M1_A: GPIOB.15 with pinCMx 32 on package pin 3 */
// pins affected by this interrupt request:["ENC_M1_A","ENC_M1_B","ENC_M2_A","ENC_M2_B"]
#define BOARD_GPIO_INT_IRQN                                     (GPIOB_INT_IRQn)
#define BOARD_GPIO_INT_IIDX                     (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define BOARD_GPIO_ENC_M1_A_IIDX                            (DL_GPIO_IIDX_DIO15)
#define BOARD_GPIO_ENC_M1_A_PIN                                 (DL_GPIO_PIN_15)
#define BOARD_GPIO_ENC_M1_A_IOMUX                                (IOMUX_PINCM32)
/* Defines for ENC_M1_B: GPIOB.16 with pinCMx 33 on package pin 4 */
#define BOARD_GPIO_ENC_M1_B_IIDX                            (DL_GPIO_IIDX_DIO16)
#define BOARD_GPIO_ENC_M1_B_PIN                                 (DL_GPIO_PIN_16)
#define BOARD_GPIO_ENC_M1_B_IOMUX                                (IOMUX_PINCM33)
/* Defines for ENC_M2_A: GPIOB.12 with pinCMx 29 on package pin 64 */
#define BOARD_GPIO_ENC_M2_A_IIDX                            (DL_GPIO_IIDX_DIO12)
#define BOARD_GPIO_ENC_M2_A_PIN                                 (DL_GPIO_PIN_12)
#define BOARD_GPIO_ENC_M2_A_IOMUX                                (IOMUX_PINCM29)
/* Defines for ENC_M2_B: GPIOB.13 with pinCMx 30 on package pin 1 */
#define BOARD_GPIO_ENC_M2_B_IIDX                            (DL_GPIO_IIDX_DIO13)
#define BOARD_GPIO_ENC_M2_B_PIN                                 (DL_GPIO_PIN_13)
#define BOARD_GPIO_ENC_M2_B_IOMUX                                (IOMUX_PINCM30)
/* Defines for GRAY_D1: GPIOB.20 with pinCMx 48 on package pin 19 */
#define BOARD_GPIO_GRAY_D1_PIN                                  (DL_GPIO_PIN_20)
#define BOARD_GPIO_GRAY_D1_IOMUX                                 (IOMUX_PINCM48)
/* Defines for GRAY_D2: GPIOB.0 with pinCMx 12 on package pin 47 */
#define BOARD_GPIO_GRAY_D2_PIN                                   (DL_GPIO_PIN_0)
#define BOARD_GPIO_GRAY_D2_IOMUX                                 (IOMUX_PINCM12)
/* Defines for GRAY_D3: GPIOB.22 with pinCMx 50 on package pin 21 */
#define BOARD_GPIO_GRAY_D3_PIN                                  (DL_GPIO_PIN_22)
#define BOARD_GPIO_GRAY_D3_IOMUX                                 (IOMUX_PINCM50)
/* Defines for GRAY_D4: GPIOB.23 with pinCMx 51 on package pin 22 */
#define BOARD_GPIO_GRAY_D4_PIN                                  (DL_GPIO_PIN_23)
#define BOARD_GPIO_GRAY_D4_IOMUX                                 (IOMUX_PINCM51)
/* Defines for GRAY_D5: GPIOB.24 with pinCMx 52 on package pin 23 */
#define BOARD_GPIO_GRAY_D5_PIN                                  (DL_GPIO_PIN_24)
#define BOARD_GPIO_GRAY_D5_IOMUX                                 (IOMUX_PINCM52)
/* Defines for GRAY_D6: GPIOB.25 with pinCMx 56 on package pin 27 */
#define BOARD_GPIO_GRAY_D6_PIN                                  (DL_GPIO_PIN_25)
#define BOARD_GPIO_GRAY_D6_IOMUX                                 (IOMUX_PINCM56)
/* Defines for GRAY_D7: GPIOB.26 with pinCMx 57 on package pin 28 */
#define BOARD_GPIO_GRAY_D7_PIN                                  (DL_GPIO_PIN_26)
#define BOARD_GPIO_GRAY_D7_IOMUX                                 (IOMUX_PINCM57)
/* Defines for GRAY_D8: GPIOB.27 with pinCMx 58 on package pin 29 */
#define BOARD_GPIO_GRAY_D8_PIN                                  (DL_GPIO_PIN_27)
#define BOARD_GPIO_GRAY_D8_IOMUX                                 (IOMUX_PINCM58)




/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_MOTOR_PWM_init(void);
void SYSCFG_DL_I2C_JY901P_init(void);
void SYSCFG_DL_UART_DEBUG_init(void);
void SYSCFG_DL_DMA_init(void);

void SYSCFG_DL_SYSTICK_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
