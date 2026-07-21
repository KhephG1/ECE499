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
#include "adc.h"
#include "dcache.h"
#include "i2c.h"
#include "icache.h"
#include "lptim.h"
#include "usart.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "scd40_driver.h"
#include "bme680_defs.h"
#include "bme680_driver.h"
#include <bsec_datatypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "sx126x.h"
#include "uart_logs.h"
#include "lora_link.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIMER_ARR_5SECONDS      (10240) //(32.768kHz / 16) / 10240 = 0.2Hz = 5s period
#define SENSOR_WAIT_TIMEOUT_MS (5000)
#define PAYLOAD_LEN           (LORA_LINK_PAYLOAD_LEN)
#define ADC_CODE_MAX (1 << 14)
#define NODE_ID (0x01) 

/*
 * Radio configuration.
 *
 * Everything the gateway also has to agree on -- frequency, SF, bandwidth,
 * coding rate, preamble, sync word -- comes from shared/lora_link.h and must
 * be changed there, not here. Only settings local to this board live below.
 *
 * The TCXO settings describe the crystal on the Waveshare module, wired to
 * DIO3. RADIO_TCXO_VOLTAGE takes an sx126x_tcxo_ctrl_voltages_t
 * (e.g. SX126X_TCXO_CTRL_1_7) and RADIO_TCXO_STARTUP_MS is how long the TCXO
 * needs to settle before the PLL may use it.
 */
#define RADIO_FREQ_HZ (LORA_LINK_FREQ_HZ)
#define RADIO_TX_POWER_DBM (-9)
#define RADIO_TCXO_VOLTAGE SX126X_TCXO_CTRL_1_7V
#define RADIO_TCXO_STARTUP_MS (5)
#define RADIO_TX_TIMEOUT_MS   (5000)
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
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// sx1262_hal.c talks to a single hard-wired radio and ignores the context
// argument, so there is nothing to point this at.
static const void *radio_ctx = NULL;

// The sx126x enums encode SF as its own numeric value and CR as (denominator -
// 4), so the shared link constants can be checked against the enums the driver
// actually wants at compile time rather than trusted to stay in step.
_Static_assert(SX126X_LORA_SF9 == LORA_LINK_SF,
               "SF disagrees with shared/lora_link.h");
_Static_assert(SX126X_LORA_CR_4_7 + 4 == LORA_LINK_CR_DENOM,
               "coding rate disagrees with shared/lora_link.h");
_Static_assert(LORA_LINK_EXPLICIT_HDR && LORA_LINK_CRC_ON && !LORA_LINK_INVERT_IQ,
               "packet params below no longer match shared/lora_link.h");

static sx126x_pkt_params_lora_t radio_pkt_params = {
    .preamble_len_in_symb = LORA_LINK_PREAMBLE_SYMB,
    .header_type          = SX126X_LORA_PKT_EXPLICIT,
    .pld_len_in_bytes     = 0,  // set per packet in radio_transmit()
    .crc_is_on            = true,
    .invert_iq_is_on      = false,
};

// Brings the radio from power-on to "configured, in standby, ready to be
// handed a payload". Returns false at the first command the chip rejects.
static bool radio_init(void)
{
    // Must stay in step with shared/lora_link.h; the _Static_asserts above
    // cover sf and cr, and BW_125 is the enum for LORA_LINK_BW_KHZ.
    const sx126x_mod_params_lora_t mod_params = {
        .sf   = SX126X_LORA_SF9,
        .bw   = SX126X_LORA_BW_125,
        .cr   = SX126X_LORA_CR_4_7,
        .ldro = 0,  // only needed when symbol time exceeds 16ms (SF11/SF12 at BW125)
    };

    // +22dBm PA config for the SX1262, datasheet table 13-21. device_sel picks
    // the SX1262's high-power PA over the SX1261's.
    const sx126x_pa_cfg_params_t pa_cfg = {
        .pa_duty_cycle = 0x04,
        .hp_max        = 0x07,
        .device_sel    = 0x00,
        .pa_lut        = 0x01,
    };

    if (sx126x_reset(radio_ctx) != SX126X_STATUS_OK) return false;
    if (sx126x_set_standby(radio_ctx, SX126X_STANDBY_CFG_RC) != SX126X_STATUS_OK) return false;

    // The Waveshare module clocks the chip from a TCXO powered off DIO3, which
    // means the chip booted on a crystal that is not there. Every block
    // calibrated at boot has to be recalibrated once the TCXO is running.
    if (sx126x_set_dio3_as_tcxo_ctrl(radio_ctx, RADIO_TCXO_VOLTAGE,
                                     sx126x_convert_timeout_in_ms_to_rtc_step(RADIO_TCXO_STARTUP_MS))
        != SX126X_STATUS_OK) return false;
    if (sx126x_cal(radio_ctx, SX126X_CAL_ALL) != SX126X_STATUS_OK) return false;

    if (sx126x_set_dio2_as_rf_sw_ctrl(radio_ctx, true) != SX126X_STATUS_OK) return false;
    if (sx126x_set_reg_mode(radio_ctx, SX126X_REG_MODE_DCDC) != SX126X_STATUS_OK) return false;

    if (sx126x_set_pkt_type(radio_ctx, SX126X_PKT_TYPE_LORA) != SX126X_STATUS_OK) return false;
    if (sx126x_set_rf_freq(radio_ctx, RADIO_FREQ_HZ) != SX126X_STATUS_OK) return false;
    if (sx126x_cal_img_in_mhz(radio_ctx, RADIO_FREQ_HZ / 1000000, RADIO_FREQ_HZ / 1000000)
        != SX126X_STATUS_OK) return false;

    if (sx126x_set_pa_cfg(radio_ctx, &pa_cfg) != SX126X_STATUS_OK) return false;
    // SX1262 errata 15.2: the TX clamp must be reconfigured after set_pa_cfg.
    if (sx126x_cfg_tx_clamp(radio_ctx) != SX126X_STATUS_OK) return false;
    if (sx126x_set_tx_params(radio_ctx, RADIO_TX_POWER_DBM, SX126X_RAMP_200_US) != SX126X_STATUS_OK) return false;

    if (sx126x_set_buffer_base_address(radio_ctx, 0, 0) != SX126X_STATUS_OK) return false;
    if (sx126x_set_lora_mod_params(radio_ctx, &mod_params) != SX126X_STATUS_OK) return false;
    if (sx126x_set_lora_pkt_params(radio_ctx, &radio_pkt_params) != SX126X_STATUS_OK) return false;

    // The chip powers up with this already at 0x1424, so the link happened to
    // work without it, but a receiver only accepts a matching sync word and
    // nothing else here pins the value down. Set it explicitly.
    if (sx126x_set_lora_sync_word(radio_ctx, LORA_LINK_SYNC_WORD) != SX126X_STATUS_OK) return false;

    if (sx126x_set_dio_irq_params(radio_ctx, SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT,
                                  SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT, 0, 0) != SX126X_STATUS_OK) return false;

    return true;
}

// Sends one packet and blocks until the chip reports TX_DONE. Polls the IRQ
// register rather than DIO1 so this stays independent of EXTI setup.
static bool radio_transmit(const uint8_t *payload, uint8_t len)
{
    radio_pkt_params.pld_len_in_bytes = len;
    if (sx126x_set_lora_pkt_params(radio_ctx, &radio_pkt_params) != SX126X_STATUS_OK) return false;

    if (sx126x_write_buffer(radio_ctx, 0, payload, len) != SX126X_STATUS_OK) return false;
    if (sx126x_clear_irq_status(radio_ctx, SX126X_IRQ_ALL) != SX126X_STATUS_OK) return false;
    if (sx126x_set_tx(radio_ctx, RADIO_TX_TIMEOUT_MS) != SX126X_STATUS_OK) return false;

    // The chip's own TX timeout should fire first; this bound only keeps a
    // wedged radio from hanging the loop forever.
    uint32_t started = HAL_GetTick();
    while (HAL_GetTick() - started < RADIO_TX_TIMEOUT_MS + 1000)
    {
        sx126x_irq_mask_t irq = 0;
        if (sx126x_get_irq_status(radio_ctx, &irq) != SX126X_STATUS_OK) return false;

        if (irq & SX126X_IRQ_TX_DONE)
        {
            sx126x_clear_irq_status(radio_ctx, SX126X_IRQ_TX_DONE);
            return true;
        }
        if (irq & SX126X_IRQ_TIMEOUT)
        {
            sx126x_clear_irq_status(radio_ctx, SX126X_IRQ_TIMEOUT);
            log_error("radio: tx timeout\r\n");
            return false;
        }
    }

    log_error("radio: tx never completed\r\n");
    return false;
}

//struct for the scd40 sensor
scd4x_handle_t scd40 = {};
struct bme68x_dev bme680 = {};
bsec_output_t bme680_data[6] = {};
float battery_voltage = 0.0f;
#define BME680_MAX_OUTPUTS ((uint8_t)(sizeof(bme680_data) / sizeof(bme680_data[0])))
uint8_t noutputs = BME680_MAX_OUTPUTS;
// Packs the latest SCD40 + BME680/BSEC readings into the fixed little-endian
// payload described by shared/lora_link.h. The gateway decodes it from those
// same offsets, so the layout is defined there rather than here.
static void put_u16(uint8_t *out, uint16_t v)
{
    out[0] = (uint8_t)(v & 0xFF);
    out[1] = (uint8_t)(v >> 8);
}

static size_t build_payload(uint8_t *out, const bsec_output_t *bme_outputs, uint8_t n_bme_outputs)
{
    memset(out, 0, PAYLOAD_LEN);

    uint8_t idx = get_scd40_latest_index();
    put_u16(&out[LORA_PL_OFF_SCD_CO2], get_scd40_CO2_readings()[idx]);
    put_u16(&out[LORA_PL_OFF_SCD_TEMP], (int16_t)(get_scd40_temp_readings()[idx] * 100.0f));
    put_u16(&out[LORA_PL_OFF_SCD_HUM], (int16_t)(get_scd40_hum_readings()[idx] * 100.0f));
    put_u16(&out[LORA_PL_OFF_VBATT],(uint16_t)(battery_voltage * 100.0f));
    put_u16(&out[LORA_PL_OFF_NODEID],(uint16_t)(NODE_ID));
    for (uint8_t i = 0; i < n_bme_outputs; i++)
    {
        const bsec_output_t *o = &bme_outputs[i];
        switch (o->sensor_id)
        {
        case BSEC_OUTPUT_CO2_EQUIVALENT:
            put_u16(&out[LORA_PL_OFF_BME_CO2EQ], (uint16_t)o->signal);
            break;
        case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
            put_u16(&out[LORA_PL_OFF_BME_VOC], (uint16_t)(o->signal * 100.0f));
            break;
        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
            put_u16(&out[LORA_PL_OFF_BME_HUM], (int16_t)(o->signal * 100.0f));
            break;
        case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
            put_u16(&out[LORA_PL_OFF_BME_TEMP], (int16_t)(o->signal * 100.0f));
            break;
        case BSEC_OUTPUT_RAW_PRESSURE:
            put_u16(&out[LORA_PL_OFF_BME_PRESS], (uint16_t)(o->signal / 10.0f));
            break;
        case BSEC_OUTPUT_STABILIZATION_STATUS:
            out[LORA_PL_OFF_BME_STAB] = (uint8_t)o->signal;
            break;
        default:
            break;
        }
    }

    return PAYLOAD_LEN;
}
//software emulation of shorting DIO2 and RXEN for the lora module together.
/*
- the interrupt is set to trigger on both rising and falling edge
- the interrupt is attached to PB4 (connected to radio module DIO2)
- the callback below reads the state of DIO2 and writes it to PD5 (connected to Lora RXEN)
*/
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_EXTI4_Pin) {
    GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB,GPIO_EXTI4_Pin);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, state);
  } else {
      __NOP();
  }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin == GPIO_EXTI4_Pin) {
    GPIO_PinState state = HAL_GPIO_ReadPin(GPIOB,GPIO_EXTI4_Pin);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, state);
  } else if(GPIO_Pin==GPIO_PIN_2){
  //invalid battery volatge to indicate vbatok has gone low
   battery_voltage  = 0;
   uint8_t payload[PAYLOAD_LEN];
   build_payload(payload, bme680_data, noutputs);
   //make an emergency transmission to indicate low battery state to gateway
   if (radio_ctx !=NULL && radio_transmit(payload, PAYLOAD_LEN))
   {
       log_info("radio: tx ok\r\n");
   }
  } else {
      __NOP();
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
  MX_ADC1_Init();
  MX_I2C4_Init();
  MX_SPI3_Init();
  MX_DCACHE1_Init();
  MX_ICACHE_Init();
  MX_LPTIM2_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */
  //status indicator
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6,GPIO_PIN_RESET);
  HAL_Delay(2000);
  HAL_GPIO_WritePin(GPIOC,  GPIO_PIN_6, GPIO_PIN_SET);
  //initialize the scd40

  if(init_scd40(&scd40, 0, 0.0,0.0) != 0){
    Error_Handler();
  }
  HAL_Delay(100);
  if(bme680_init(&bme680, BME68X_I2C_INTF) != BSEC_OK){
    Error_Handler();
  }

  // LoRa radio bring-up. MX_GPIO_Init and MX_SPI1_Init have already set up the
  // pins and the SPI bus the HAL port drives, so the radio can be configured
  // directly from here.
  if (!radio_init())
  {
    log_error("radio: init failed\r\n");
    Error_Handler();
  }
  log_info("radio: init ok\r\n");
  //calibrate ADC
  HAL_ADCEx_Calibration_Start(&hadc1,ADC_CALIB_OFFSET,ADC_SINGLE_ENDED );
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
   // Wake cycle: poll the sensors at a 100ms cadence until the SCD40's
   // periodic measurement is ready (or we give up after
   // SENSOR_WAIT_TIMEOUT_MS), then TX one packet and idle until the next
   // cycle. Sensors are read before the payload is built so the first packet
   // out carries real readings rather than zeros.
   uint32_t cycle_start = HAL_GetTick();
   uint8_t scd_status;
   uint8_t bme_noutputs = 0;

   do
   {
       // BSEC reads noutputs as the capacity of bme680_data on the way in and
       // overwrites it with the number of outputs it produced, so it has to be
       // reset before every call or the capacity ratchets down.
       noutputs = BME680_MAX_OUTPUTS;
       if (bme680_step(&bme680, bme680_data, &noutputs) == BME68X_OK)
       {
           bme_noutputs = noutputs;
       }

       scd_status = scd4x_basic_read(&scd40);
       if (scd_status == 0)
       {
           break;
       }
       //read the battery voltage
       if(HAL_ADC_Start(&hadc1) == HAL_OK){
            HAL_ADC_PollForConversion(&hadc1, 1000);
            uint32_t adc_code = HAL_ADC_GetValue(&hadc1);
            HAL_ADC_Stop(&hadc1);      
            //multiply by two since voltage divider divides actual voltage in half  
            battery_voltage = ((adc_code * 3.3f) / ADC_CODE_MAX) * 2.0f;
       };
       HAL_Delay(100);
   } while (HAL_GetTick() - cycle_start < SENSOR_WAIT_TIMEOUT_MS);

   if (scd_status != 0)
   {
       // Send anyway: the BME680 fields are still fresh and the SCD40 fields
       // fall back to the previous cycle's readings.
       log_error("scd40: no measurement this cycle\r\n");
   }

   uint8_t payload[PAYLOAD_LEN];
   build_payload(payload, bme680_data, bme_noutputs);
   if (radio_transmit(payload, PAYLOAD_LEN))
   {
       log_info("radio: tx ok\r\n");
   }
   HAL_LPTIM_TimeOut_Start_IT(&hlptim2, TIMER_ARR_5SECONDS);
   HAL_DBGMCU_EnableDBGStopMode();
   HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
   SystemClock_Config();
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
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
}

/**
  * @brief Power Configuration
  * @retval None
  */
static void SystemPower_Config(void)
{

  /*
   * SRAM Power Down In Stop Mode Config
   */
  HAL_PWREx_DisableRAMsContentStopRetention(PWR_ICACHE_FULL_STOP_RETENTION);
  HAL_PWREx_DisableRAMsContentStopRetention(PWR_DCACHE1_FULL_STOP_RETENTION);
  HAL_PWREx_DisableRAMsContentStopRetention(PWR_DMA2DRAM_FULL_STOP_RETENTION);
  HAL_PWREx_DisableRAMsContentStopRetention(PWR_PKA32RAM_FULL_STOP_RETENTION);
/* USER CODE BEGIN PWR */
/* USER CODE END PWR */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param None
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
