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

#include "smtc_hal_mcu.h"
#include "smtc_hal_lp_timer.h"
#include "smtc_hal_spi.h"
#include "smtc_hal_rtc.h"
#include "modem_pinout.h"
#include "smtc_modem_utilities.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCD40_SAMPLE_PERIOD_MS (30000)

#define LORA_STACK_ID    (0)
#define LORA_FPORT       (1)
#define LORA_UPLINK_PERIOD_MS (30000)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// TODO: replace with the DevEUI/JoinEUI/AppKey registered on your LoRaWAN
// network server (e.g. The Things Network / ChirpStack) for this device.
static uint8_t lora_dev_eui[8]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
static uint8_t lora_join_eui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uint8_t lora_app_key[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                     0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

static volatile bool lora_event_pending = false;
static volatile bool lora_joined        = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void SystemPower_Config(void);
/* USER CODE BEGIN PFP */
static void lora_on_modem_event(void);
static void lora_process_events(void);
static void lora_send_uplink(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//struct for the scd40 sensor
scd4x_handle_t scd40 = {};
struct bme68x_dev bme680 = {};
bsec_output_t bme680_data[6] = {};
uint8_t noutputs = 6;

// Called by the modem stack whenever an event is waiting; keep this short,
// it can run from radio-IRQ context. Actual handling happens in the main loop.
static void lora_on_modem_event(void)
{
    lora_event_pending = true;
}

static void lora_process_events(void)
{
    smtc_modem_event_t event;
    uint8_t event_pending_count = 0;

    lora_event_pending = false;

    do
    {
        if (smtc_modem_get_event(&event, &event_pending_count) != SMTC_MODEM_RC_OK)
        {
            break;
        }

        switch (event.event_type)
        {
        case SMTC_MODEM_EVENT_RESET:
            smtc_modem_set_deveui(LORA_STACK_ID, lora_dev_eui);
            smtc_modem_set_joineui(LORA_STACK_ID, lora_join_eui);
            smtc_modem_set_appkey(LORA_STACK_ID, lora_app_key);
            // TODO: pick the region that matches your test network
            smtc_modem_set_region(LORA_STACK_ID, SMTC_MODEM_REGION_US_915);
            smtc_modem_join_network(LORA_STACK_ID);
            break;

        case SMTC_MODEM_EVENT_JOINED:
            lora_joined = true;
            break;

        case SMTC_MODEM_EVENT_JOINFAIL:
            lora_joined = false;
            break;

        case SMTC_MODEM_EVENT_TXDONE:
        case SMTC_MODEM_EVENT_DOWNDATA:
        case SMTC_MODEM_EVENT_ALARM:
        default:
            break;
        }
    } while (event_pending_count > 0);
}

static void lora_send_uplink(void)
{
    static uint32_t counter = 0;
    uint8_t payload[4];

    payload[0] = (uint8_t)(counter >> 24);
    payload[1] = (uint8_t)(counter >> 16);
    payload[2] = (uint8_t)(counter >> 8);
    payload[3] = (uint8_t)(counter);

    if (smtc_modem_request_uplink(LORA_STACK_ID, LORA_FPORT, false, payload, sizeof(payload)) == SMTC_MODEM_RC_OK)
    {
        counter++;
    }
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
 // MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  //initialize the scd40

  if(init_scd40(&scd40, 0, 0.0,0.0) != 0){
    Error_Handler();
  }
  HAL_Delay(100);
  if(bme680_init(&bme680, BME68X_I2C_INTF) != BSEC_OK){
    Error_Handler();
  }

  // LoRa radio bring-up: CubeMX's MX_GPIO_Init/MX_SPI1_Init/MX_RTC_Init/
  // MX_LPTIM1_Init already configured the underlying peripherals; these
  // calls populate the smtc_hal layer's own handles on top of them and
  // set up the radio-specific control pins CubeMX doesn't know about.
  mcu_gpio_init();
  hal_lp_timer_init(HAL_LP_TIMER_ID_1);
  hal_spi_init(RADIO_SPI_ID, RADIO_SPI_MOSI, RADIO_SPI_MISO, RADIO_SPI_SCLK);
  hal_rtc_init();

  smtc_modem_init(&lora_on_modem_event);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
   static uint32_t last_sensor_read_tick = 0;
   static uint32_t last_uplink_tick = 0;

   // smtc_modem_run_engine() drives the LoRaWAN stack (join, radio timing,
   // retries...) and must be called frequently -- it also services the
   // IWDG internally, so avoid long blocking delays once this is in place.
   smtc_modem_run_engine();

   if (lora_event_pending)
   {
       lora_process_events();
   }

   if (lora_joined && (HAL_GetTick() - last_uplink_tick >= LORA_UPLINK_PERIOD_MS))
   {
       last_uplink_tick = HAL_GetTick();
       lora_send_uplink();
   }

   if (HAL_GetTick() - last_sensor_read_tick >= 100)
   {
       last_sensor_read_tick = HAL_GetTick();
       int8_t bme_status = bme680_step(&bme680, bme680_data, &noutputs);
       uint8_t scd_status = scd4x_basic_read(&scd40);
       (void)bme_status;
       (void)scd_status;
   }
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
