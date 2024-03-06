/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bedug.h"

#include "log.h"
#include "w25qxx.h"
#include "hal_defs.h"
#include "settings.h"

#include "Timer.h"
#include "Record.h"
#include "Measure.h"
#include "Watchdogs.h"
#include "SoulGuard.h"
#include "StorageAT.h"
#include "StorageDriver.h"
#include "CodeStopwatch.h"
#include "USBController.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static constexpr char MAIN_TAG[] = "MAIN";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
StorageDriver storageDriver;
StorageAT* storage;

utl::Timer exitTimer(10000);
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  MX_USB_DEVICE_Init();
  MX_RTC_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	// TODO: RAM analyzer & crystal check & clock check & modbus check & reload controller
	SoulGuard<
		RestartWatchdog,
		PowerWatchdog,
		MemoryWatchdog,
		StackWatchdog,
		StandbyWatchdog
	> hardwareSoulGuard;
	SoulGuard<
		SettingsWatchdog,
		RTCWatchdog
	> softwareSoulGuard;
	USBController usbc;
	Measure measure;

	set_status(WAIT_LOAD);
	set_error(POWER_ERROR);
//	set_error(MEMORY_ERROR); // TODO
	set_error(STACK_ERROR);

	HAL_Delay(100);

	exitTimer.start();

	HAL_TIM_Base_Start_IT(&LED_TIM);

	gprint("\n\n\n");
	printTagLog(MAIN_TAG, "The device is loading");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (has_errors()) hardwareSoulGuard.defend();

    flash_w25qxx_init();
    storage = new StorageAT(
		flash_w25qxx_get_pages_count(),
		&storageDriver
	);

    while (is_status(WAIT_LOAD)) {
    	hardwareSoulGuard.defend();
    	softwareSoulGuard.defend();
    }

    printTagLog(MAIN_TAG, "The device has been loaded");

	while (1)
	{
		utl::CodeStopwatch stopwatch(MAIN_TAG, GENERAL_TIMEOUT_MS);

		hardwareSoulGuard.defend();
		softwareSoulGuard.defend();

		if (has_errors() || is_status(WAIT_LOAD)) {
			continue;
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		measure.process();

		usbc.proccess();
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

int _write(int, uint8_t *ptr, int len) {
	(void)ptr;
	(void)len;
#ifdef DEBUG
    HAL_UART_Transmit(&BEDUG_UART, (uint8_t *)ptr, static_cast<uint16_t>(len), GENERAL_TIMEOUT_MS);
    for (int DataIdx = 0; DataIdx < len; DataIdx++) {
        ITM_SendChar(*ptr++);
    }
    return len;
#endif
    return 0;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == LED_TIM.Instance) //check if the interrupt comes from TIM1
	{
		static utl::Timer timer(SECOND_MS / 10);
		static utl::Timer errTimer(SECOND_MS);
		static bool errEnabled = false;
		if (errEnabled) {
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
		} else if (!timer.wait()) {
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			timer.start();
		}
		if (!errTimer.wait() && has_errors()) {
			timer.changeDelay(SECOND_MS / 50);
			errEnabled = !errEnabled;
			errTimer.start();
		} else if (has_errors()) {

		} else if (is_status(WAIT_LOAD)) {
			timer.changeDelay(SECOND_MS / 50);
			errEnabled = false;
		} else if (!has_errors()) {
			timer.changeDelay(SECOND_MS / 10);
			errEnabled = false;
		}
		if (!has_errors()) {
			exitTimer.start();
		} else if (!exitTimer.wait() && !USBController::connected()) {
			set_status(NEED_STANDBY);
		}
	}
}

void system_fault_handler()
{
	set_error(INTERNAL_ERROR);
	NVIC_SystemReset();
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    b_assert(__FILE__, __LINE__, "The error handler has been called");
	set_error(INTERNAL_ERROR);
	while (1);
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	b_assert((char*)file, line, "Wrong parameters value");
	set_error(INTERNAL_ERROR);
	while (1);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
