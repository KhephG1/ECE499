#ifndef LORA_P2P_H
#define LORA_P2P_H

#include <stdint.h>
#include <stdbool.h>

/* RF parameters -- must match the receiver exactly. */
#define LORA_P2P_FREQ_HZ       (915000000UL)
#define LORA_P2P_TX_POWER_DBM  (14)
#define LORA_P2P_TX_TIMEOUT_MS (4000UL)

/* Waveshare Core1262-868M carries a 32MHz TCXO (Q1) powered directly from
 * the sx1262's DIO3 pin (confirmed against the module schematic). These
 * values are NOT a guess -- they're taken directly from Waveshare's own
 * official Pico library for this exact module
 * (github.com/siuwahzhong/lorawan-library-for-pico,
 * src/boards/rp2040/sx126x-board.c: SX126xIoTcxoInit / BOARD_TCXO_WAKEUP_TIME),
 * confirmed to be the correct reference values for this module's TCXO. */
#define LORA_P2P_TCXO_VOLTAGE                 SX126X_TCXO_CTRL_1_7V
#define LORA_P2P_TCXO_STARTUP_TIME_RTC_STEPS  (320UL) /* 5ms, 64 steps/ms -- matches BOARD_TCXO_WAKEUP_TIME */

bool lora_p2p_init(void);
bool lora_p2p_send(const uint8_t *payload, uint8_t len, uint32_t timeout_ms);
void lora_p2p_sleep(void);

// Minimal chip bring-up check: reset, log GetStatus/GetDeviceErrors before
// and after enabling the TCXO and forcing a calibration. No RF freq, no
// packet params, no TX -- just "is the chip alive and responding over SPI,
// and does it think its clock/PLL calibration succeeded." Meant for
// bring-up debugging, not normal operation.
void lora_p2p_selftest(void);

#endif /* LORA_P2P_H */
