/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32u5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADC1_IN16_Pin GPIO_PIN_1
#define ADC1_IN16_GPIO_Port GPIOB
#define GPO_Pin GPIO_PIN_6
#define GPO_GPIO_Port GPIOC
#define GPOA15_Pin GPIO_PIN_15
#define GPOA15_GPIO_Port GPIOA
#define GPOD0_Pin GPIO_PIN_0
#define GPOD0_GPIO_Port GPIOD
#define GPI_Pin GPIO_PIN_1
#define GPI_GPIO_Port GPIOD
#define GPOD2_Pin GPIO_PIN_2
#define GPOD2_GPIO_Port GPIOD
#define GPOD5_Pin GPIO_PIN_5
#define GPOD5_GPIO_Port GPIOD
#define GPOD7_Pin GPIO_PIN_7
#define GPOD7_GPIO_Port GPIOD
#define GPIO_EXTI4_Pin GPIO_PIN_4
#define GPIO_EXTI4_GPIO_Port GPIOB
#define GPIO_EXTI4_EXTI_IRQn EXTI4_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
