/*!
 * \file      sx1262_hal.c
 *
 * \brief     Implements the sx126x radio HAL functions on top of the STM32U5
 *            CubeMX HAL (SPI1 + the RADIO_* / LoRaCS pins from main.h).
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type

#include "sx126x_hal.h"

#include "main.h"
#include "spi.h"

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * Board wiring -- fill these in.
 *
 * CubeMX generates a <label>_GPIO_Port / <label>_Pin pair in main.h for every
 * pin labelled in the .ioc, so each blank below has a symbol there to match.
 * The labels currently in main.h are LoRaCS, RADIO_BUSY, RADIO_RST, RADIO_DIO
 * and RF_SWITCH.
 */
#define RADIO_SPI       ( &hspi3 )

#define RADIO_NSS_PORT (GPIOA)
#define RADIO_NSS_PIN (GPIO_PIN_15)

#define RADIO_BUSY_PORT (GPIOD)
#define RADIO_BUSY_PIN (GPIO_PIN_1)

#define RADIO_NRST_PORT (GPIOD)
#define RADIO_NRST_PIN (GPIO_PIN_0)

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

#define SX126X_SPI_TIMEOUT_MS ( 1000 )

/**
 * @brief Opcode of the SetSleep command, see SX126x datasheet section 13.1.1
 */
#define SX126X_SET_SLEEP_OPCODE ( 0x84 )

/**
 * @brief NRESET must be held low for at least 100us, see SX126x datasheet
 * section 8.1. HAL_Delay's granularity is 1ms, so round the pulse up to 2ms.
 */
#define SX126X_RESET_PULSE_MS ( 2 )

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

typedef enum
{
    RADIO_SLEEP,
    RADIO_AWAKE
} radio_sleep_mode_t;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */

// This variable will hold the current sleep status of the radio
static radio_sleep_mode_t radio_mode = RADIO_AWAKE;

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/**
 * @brief Wait until radio busy pin returns to 0
 */
static void sx126x_hal_wait_on_busy( void );

/**
 * @brief Check if device is ready to receive spi transaction.
 * @remark If the device is in sleep mode, it will awake it and wait until it is ready
 */
static void sx126x_hal_check_device_ready( void );

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */

sx126x_hal_status_t sx126x_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length )
{
    ( void ) context;

    sx126x_hal_check_device_ready( );

    // Put NSS low to start spi transaction
    HAL_GPIO_WritePin( RADIO_NSS_PORT, RADIO_NSS_PIN, GPIO_PIN_RESET );

    HAL_StatusTypeDef status = HAL_SPI_Transmit( RADIO_SPI, command, command_length, SX126X_SPI_TIMEOUT_MS );
    if( ( status == HAL_OK ) && ( data_length > 0 ) )
    {
        status = HAL_SPI_Transmit( RADIO_SPI, data, data_length, SX126X_SPI_TIMEOUT_MS );
    }

    // Put NSS high as the spi transaction is finished
    HAL_GPIO_WritePin( RADIO_NSS_PORT, RADIO_NSS_PIN, GPIO_PIN_SET );

    if( status != HAL_OK )
    {
        return SX126X_HAL_STATUS_ERROR;
    }

    // In sleep mode the radio busy pin is stuck at 1 => do not test it
    if( command[0] != SX126X_SET_SLEEP_OPCODE )
    {
        sx126x_hal_check_device_ready( );
    }
    else
    {
        radio_mode = RADIO_SLEEP;
    }

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read( const void* context, const uint8_t* command, const uint16_t command_length,
                                     uint8_t* data, const uint16_t data_length )
{
    ( void ) context;

    sx126x_hal_check_device_ready( );

    // Put NSS low to start spi transaction
    HAL_GPIO_WritePin( RADIO_NSS_PORT, RADIO_NSS_PIN, GPIO_PIN_RESET );

    HAL_StatusTypeDef status = HAL_SPI_Transmit( RADIO_SPI, command, command_length, SX126X_SPI_TIMEOUT_MS );
    if( ( status == HAL_OK ) && ( data_length > 0 ) )
    {
        status = HAL_SPI_Receive( RADIO_SPI, data, data_length, SX126X_SPI_TIMEOUT_MS );
    }

    // Put NSS high as the spi transaction is finished
    HAL_GPIO_WritePin( RADIO_NSS_PORT, RADIO_NSS_PIN, GPIO_PIN_SET );

    if( status != HAL_OK )
    {
        return SX126X_HAL_STATUS_ERROR;
    }

    sx126x_hal_check_device_ready( );

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_reset( const void* context )
{
    ( void ) context;

    // MX_GPIO_Init leaves NSS low, which the radio reads as the start of a
    // transaction. Park it high before releasing NRESET.
    HAL_GPIO_WritePin( RADIO_NSS_PORT, RADIO_NSS_PIN, GPIO_PIN_SET );

    HAL_GPIO_WritePin( RADIO_NRST_PORT, RADIO_NRST_PIN, GPIO_PIN_RESET );
    HAL_Delay( SX126X_RESET_PULSE_MS );
    HAL_GPIO_WritePin( RADIO_NRST_PORT, RADIO_NRST_PIN, GPIO_PIN_SET );
    HAL_Delay( SX126X_RESET_PULSE_MS );

    radio_mode = RADIO_AWAKE;

    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup( const void* context )
{
    ( void ) context;

    sx126x_hal_check_device_ready( );

    return SX126X_HAL_STATUS_OK;
}

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

static void sx126x_hal_wait_on_busy( void )
{
    while( HAL_GPIO_ReadPin( RADIO_BUSY_PORT, RADIO_BUSY_PIN ) == GPIO_PIN_SET )
    {
    };
}

static void sx126x_hal_check_device_ready( void )
{
    if( radio_mode != RADIO_SLEEP )
    {
        sx126x_hal_wait_on_busy( );
    }
    else
    {
        // Busy is HIGH in sleep mode, wake-up the device
        HAL_GPIO_WritePin( RADIO_NSS_PORT, RADIO_NSS_PIN, GPIO_PIN_RESET );
        sx126x_hal_wait_on_busy( );
        HAL_GPIO_WritePin( RADIO_NSS_PORT, RADIO_NSS_PIN, GPIO_PIN_SET );
        radio_mode = RADIO_AWAKE;
    }
}
