/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

int _write(int, uint8_t *ptr, int len);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define WKUP_RTC_Pin GPIO_PIN_13
#define WKUP_RTC_GPIO_Port GPIOC
#define VBAT_ADC_Pin GPIO_PIN_2
#define VBAT_ADC_GPIO_Port GPIOC
#define WKUP_Pin GPIO_PIN_0
#define WKUP_GPIO_Port GPIOA
#define LORA_AUX_Pin GPIO_PIN_1
#define LORA_AUX_GPIO_Port GPIOA
#define LORA_TX_Pin GPIO_PIN_2
#define LORA_TX_GPIO_Port GPIOA
#define LORA_RX_Pin GPIO_PIN_3
#define LORA_RX_GPIO_Port GPIOA
#define FLASH_SPI_CS_Pin GPIO_PIN_4
#define FLASH_SPI_CS_GPIO_Port GPIOA
#define POWER_L4_Pin GPIO_PIN_4
#define POWER_L4_GPIO_Port GPIOC
#define USB_HOST_EN_Pin GPIO_PIN_0
#define USB_HOST_EN_GPIO_Port GPIOB
#define RS485_EN_Pin GPIO_PIN_15
#define RS485_EN_GPIO_Port GPIOB
#define RS485_TX_Pin GPIO_PIN_6
#define RS485_TX_GPIO_Port GPIOC
#define RS485_RX_Pin GPIO_PIN_7
#define RS485_RX_GPIO_Port GPIOC
#define POWER_L3_Pin GPIO_PIN_8
#define POWER_L3_GPIO_Port GPIOC
#define POWER_L2_Pin GPIO_PIN_9
#define POWER_L2_GPIO_Port GPIOC
#define STEPUP_5V_ON_Pin GPIO_PIN_12
#define STEPUP_5V_ON_GPIO_Port GPIOC
#define MODBUS_EN_Pin GPIO_PIN_5
#define MODBUS_EN_GPIO_Port GPIOB
#define MODBUS_TX_Pin GPIO_PIN_6
#define MODBUS_TX_GPIO_Port GPIOB
#define MODBUS_RX_Pin GPIO_PIN_7
#define MODBUS_RX_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_8
#define LED_GPIO_Port GPIOB
#define STEPUP_VCC_ON_Pin GPIO_PIN_9
#define STEPUP_VCC_ON_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

// General settings
#define GENERAL_TIMEOUT_MS ((uint32_t)100)

// SD card
extern SPI_HandleTypeDef   hspi1;
#define FLASH_SPI          (hspi1)

// MODBUS
#define MODBUS_TIMEOUS_MS  ((uint32_t)70)
#define MODBUS_SENS_COUNT  ((uint8_t)127)

// MODBUS 1
extern UART_HandleTypeDef  huart1;
#define MODBUS1_UART       (huart1)

// MODBUS 2
extern UART_HandleTypeDef  huart6;
#define MODBUS2_UART       (huart6)

// Bedug UART
extern UART_HandleTypeDef  huart2;
#define BEDUG_UART         (huart2)

// LED TIM
extern TIM_HandleTypeDef   htim4;
#define LED_TIM            (htim4)

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
