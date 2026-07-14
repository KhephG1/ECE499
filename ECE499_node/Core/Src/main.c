/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcache.h"
#include "icache.h"
#include "iwdg.h"
#include "lptim.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "scd40_driver.h"
#include "bme680_defs.h"
#include "bme680_driver.h"
#include "i2c.h"
#include <bsec_datatypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "smtc_hal_mcu.h"
#include "smtc_hal_lp_timer.h"
#include "smtc_hal_spi.h"
#include "smtc_hal_rtc.h"
#include "modem_pinout.h"
#include "lora_p2p.h"
#include "uart_logs.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Set to 1 for bare-bones radio bring-up: skip sensors/sleep/TX entirely and
// just loop lora_p2p_selftest() over UART. Flip back to 0 once the radio's
// confirmed alive to restore the full sensor+TX+sleep cycle below.
#define LORA_P2P_SELFTEST_ONLY (1)

#define SLEEP_PERIOD_MS      (30000)
#define SENSOR_WAIT_TIMEOUT_MS (5000)
#define PAYLOAD_LEN           (17)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
/* USER CODE BEGIN PFP */
static size_t build_payload(uint8_t *out, const bsec_output_t *bme_outputs, uint8_t n_bme_outputs);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//struct for the scd40 sensor
scd4x_handle_t scd40 = {};
struct bme68x_dev bme680 = {};
bsec_output_t bme680_data[6] = {};
uint8_t noutputs = 6;

// Packs the latest SCD40 + BME680/BSEC readings into a fixed 17-byte
// little-endian payload. Layout (offset:type):
//   0:  uint16  CO2, ppm                        (SCD40)
//   2:  int16   temperature x100, degC           (SCD40)
//   4:  int16   humidity x100, %RH                (SCD40)
//   6:  uint16  CO2 equivalent, ppm                (BME680/BSEC)
//   8:  uint16  breath VOC equivalent x100, ppm    (BME680/BSEC)
//   10: int16   heat-compensated humidity x100     (BME680/BSEC)
//   12: int16   heat-compensated temperature x100  (BME680/BSEC)
//   14: uint16  raw pressure, hPa x10              (BME680/BSEC)
//   16: uint8   stabilization status (0/1)         (BME680/BSEC)
// Any receiver decoding this payload must use the same layout.
static void put_u16(uint8_t *out, uint16_t v)
{
    out[0] = (uint8_t)(v & 0xFF);
    out[1] = (uint8_t)(v >> 8);
}

static size_t build_payload(uint8_t *out, const bsec_output_t *bme_outputs, uint8_t n_bme_outputs)
{
    memset(out, 0, PAYLOAD_LEN);

    uint8_t idx = get_scd40_latest_index();
    put_u16(&out[0], get_scd40_CO2_readings()[idx]);
    put_u16(&out[2], (int16_t)(get_scd40_temp_readings()[idx] * 100.0f));
    put_u16(&out[4], (int16_t)(get_scd40_hum_readings()[idx] * 100.0f));

    for (uint8_t i = 0; i < n_bme_outputs; i++)
    {
        const bsec_output_t *o = &bme_outputs[i];
        switch (o->sensor_id)
        {
        case BSEC_OUTPUT_CO2_EQUIVALENT:
            put_u16(&out[6], (uint16_t)o->signal);
            break;
        case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
            put_u16(&out[8], (uint16_t)(o->signal * 100.0f));
            break;
        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
            put_u16(&out[10], (int16_t)(o->signal * 100.0f));
            break;
        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
            put_u16(&out[12], (int16_t)(o->signal * 100.0f));
            break;
        case BSEC_OUTPUT_RAW_PRESSURE:
            put_u16(&out[14], (uint16_t)(o->signal / 10.0f));
            break;
        case BSEC_OUTPUT_STABILIZATION_STATUS:
            out[16] = (uint8_t)o->signal;
            break;
        default:
            break;
        }
    }

    return PAYLOAD_LEN;
}
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

  dwt_delay_init();
  /* USER CODE END Init */

  /* Configure the System Power */
  SystemPower_Config();

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DCACHE1_Init();
  MX_ICACHE_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_RTC_Init();
  MX_LPTIM1_Init();
  MX_RNG_Init();
  //MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
#if !LORA_P2P_SELFTEST_ONLY
  //initialize the scd40

  if(init_scd40(&scd40, 0, 0.0,0.0) != 0){
    Error_Handler();
  }
  HAL_Delay(100);
  if(bme680_init(&bme680, BME68X_I2C_INTF) != BSEC_OK){
    Error_Handler();
  }
#endif

  // LoRa radio bring-up: CubeMX's MX_GPIO_Init/MX_SPI1_Init/MX_RTC_Init/
  // MX_LPTIM1_Init already configured the underlying peripherals; these
  // calls populate the smtc_hal layer's own handles on top of them and
  // set up the radio-specific control pins CubeMX doesn't know about.
  mcu_gpio_init();
  hal_lp_timer_init(HAL_LP_TIMER_ID_1);
  hal_spi_init(RADIO_SPI_ID, RADIO_SPI_MOSI, RADIO_SPI_MISO, RADIO_SPI_SCLK);
  hal_rtc_init();

#if !LORA_P2P_SELFTEST_ONLY
  lora_p2p_init();
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#if LORA_P2P_SELFTEST_ONLY
   lora_p2p_selftest();
   HAL_Delay(2000);
#else
   // Wake cycle: poll sensors at 100ms cadence until the SCD40's periodic
   // measurement is ready (or we give up after SENSOR_WAIT_TIMEOUT_MS), TX
   // one packet, then sleep the radio and MCU until the next cycle.
   uint32_t cycle_start = HAL_GetTick();
   uint8_t scd_status;
   do
   {
       bme680_step(&bme680, bme680_data, &noutputs);
       scd_status = scd4x_basic_read(&scd40);
       if (scd_status == 0)
       {
           break;
       }
       HAL_Delay(100);
   } while (HAL_GetTick() - cycle_start < SENSOR_WAIT_TIMEOUT_MS);

   uint8_t payload[PAYLOAD_LEN];
   size_t len = build_payload(payload, bme680_data, noutputs);

   bool tx_ok = lora_p2p_send(payload, (uint8_t)len, LORA_P2P_TX_TIMEOUT_MS);
   log_debug("scd_status=%u tx=%s len=%u\r\n", scd_status, tx_ok ? "OK" : "TIMEOUT", (unsigned)len);

   lora_p2p_sleep();
   hal_mcu_set_sleep_for_ms(SLEEP_PERIOD_MS);
#endif
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_MSI|RCC_OSCILLATORTYPE_MSIK;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.MSIKClockRange = RCC_MSIKRANGE_4;
  RCC_OscInitStruct.MSIKState = RCC_MSIK_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the force of MSIK in stop mode
  */
  __HAL_RCC_MSIKSTOP_ENABLE();
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * Switch to SMPS regulator instead of LDO
   */
  if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK)
  {
    Error_Handler();
  }
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
