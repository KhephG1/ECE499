#include "lora_p2p.h"

#include <stddef.h>
#include "sx126x.h"
#include "stm32u5xx_hal.h"
#include "uart_logs.h"
#include "smtc_hal_gpio.h"
#include "modem_pinout.h"

#define LORA_P2P_CHECK(expr)                                                   \
    do {                                                                      \
        sx126x_status_t _status = (expr);                                     \
        if (_status != SX126X_STATUS_OK)                                      \
        {                                                                     \
            log_debug("lora_p2p: '%s' failed (%d)\r\n", #expr, (int)_status); \
            return false;                                                     \
        }                                                                     \
    } while (0)

static bool lora_p2p_set_pkt_params(uint8_t len)
{
    sx126x_pkt_params_lora_t pkt_params = {
        .preamble_len_in_symb = 8,
        .header_type          = SX126X_LORA_PKT_EXPLICIT,
        .pld_len_in_bytes     = len,
        .crc_is_on            = true,
        .invert_iq_is_on      = false,
    };
    LORA_P2P_CHECK(sx126x_set_lora_pkt_params(NULL, &pkt_params));
    return true;
}

bool lora_p2p_init(void)
{
    sx126x_mod_params_lora_t mod_params = {
        .sf   = SX126X_LORA_SF7,
        .bw   = SX126X_LORA_BW_125,
        .cr   = SX126X_LORA_CR_4_5,
        .ldro = 0,
    };
    sx126x_pa_cfg_params_t pa_cfg = {
        .pa_duty_cycle = 0x04,
        .hp_max        = 0x07,
        .device_sel    = 0x00,
        .pa_lut        = 0x01,
    };

    // Force a clean hardware reset before touching anything else -- nothing
    // else in this init sequence ever pulses NRST, and a chip left in an
    // undefined state (e.g. warm STM32 reboot while the radio stayed
    // powered) won't reliably drive its own status outputs (BUSY) until
    // it's actually been reset.
    LORA_P2P_CHECK(sx126x_reset(NULL));
    LORA_P2P_CHECK(sx126x_set_standby(NULL, SX126X_STANDBY_CFG_RC));
    LORA_P2P_CHECK(sx126x_init_retention_list(NULL));
    LORA_P2P_CHECK(sx126x_set_reg_mode(NULL, SX126X_REG_MODE_DCDC));
    // Confirmed against the module's own schematic (traced visually, not
    // just text-extracted): DIO2 has no wire to RXEN/TXEN anywhere on this
    // board. The RF switch (U4) is driven directly by RXEN/TXEN, which are
    // separate host-controlled GPIOs -- no DIO2-as-RF-switch call here.
    // This module's TCXO (Q1) is powered from DIO3, not a plain XTAL -- must
    // be enabled before any RF-frequency-dependent operation (image calib on
    // set_rf_freq needs a locked clock).
    LORA_P2P_CHECK(sx126x_set_dio3_as_tcxo_ctrl(NULL, LORA_P2P_TCXO_VOLTAGE, LORA_P2P_TCXO_STARTUP_TIME_RTC_STEPS));
    LORA_P2P_CHECK(sx126x_set_pkt_type(NULL, SX126X_PKT_TYPE_LORA));
    LORA_P2P_CHECK(sx126x_set_rf_freq(NULL, LORA_P2P_FREQ_HZ));
    LORA_P2P_CHECK(sx126x_set_lora_mod_params(NULL, &mod_params));
    LORA_P2P_CHECK(sx126x_set_pa_cfg(NULL, &pa_cfg));
    LORA_P2P_CHECK(sx126x_set_tx_params(NULL, LORA_P2P_TX_POWER_DBM, SX126X_RAMP_40_US));
    LORA_P2P_CHECK(sx126x_set_buffer_base_address(NULL, 0, 0));

    return true;
}

bool lora_p2p_send(const uint8_t *payload, uint8_t len, uint32_t timeout_ms)
{
    if (payload == NULL || len == 0)
    {
        log_debug("lora_p2p_send: invalid payload (ptr=%p len=%u)\r\n", (const void *)payload, (unsigned)len);
        return false;
    }

    if (!lora_p2p_set_pkt_params(len))
    {
        return false;
    }
    LORA_P2P_CHECK(sx126x_write_buffer(NULL, 0, payload, len));
    LORA_P2P_CHECK(sx126x_get_and_clear_irq_status(NULL, NULL));

    // Flip the external RF switch to the TX path right before triggering the
    // radio. Everything from here on funnels through the single "restore
    // idle" block below instead of early-returning, so the switch can never
    // get left stuck in TX mode on an error path.
    hal_gpio_set_value(RADIO_TXEN, 0);
    hal_gpio_set_value(RADIO_RXEN, 1);

    bool ok = false;
    sx126x_status_t status = sx126x_set_tx(NULL, timeout_ms);
    if (status != SX126X_STATUS_OK)
    {
        log_debug("lora_p2p_send: set_tx failed (%d)\r\n", (int)status);
    }
    else
    {
        uint32_t start = HAL_GetTick();
        sx126x_irq_mask_t irq = 0;
        while (HAL_GetTick() - start < timeout_ms)
        {
            status = sx126x_get_and_clear_irq_status(NULL, &irq);
            if (status != SX126X_STATUS_OK)
            {
                log_debug("lora_p2p_send: irq status read failed (%d)\r\n", (int)status);
                break;
            }
            if (irq & (SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT))
            {
                break;
            }
            HAL_Delay(5);
        }

        if (status != SX126X_STATUS_OK)
        {
            /* already logged above */
        }
        else if (irq & SX126X_IRQ_TIMEOUT)
        {
            log_debug("lora_p2p_send: radio reported TX timeout\r\n");
        }
        else if (!(irq & SX126X_IRQ_TX_DONE))
        {
            log_debug("lora_p2p_send: no TX_DONE within %lu ms (host-side timeout)\r\n", (unsigned long)timeout_ms);
        }
        else
        {
            ok = true;
        }
    }

    // Restore the documented RX/idle switch state (RXEN low, TXEN high).
    hal_gpio_set_value(RADIO_RXEN, 0);
    hal_gpio_set_value(RADIO_TXEN, 1);

    return ok;
}

void lora_p2p_sleep(void)
{
    sx126x_status_t status = sx126x_set_sleep(NULL, SX126X_SLEEP_CFG_WARM_START);
    if (status != SX126X_STATUS_OK)
    {
        log_debug("lora_p2p_sleep: set_sleep failed (%d)\r\n", (int)status);
    }
}

static void selftest_log_status(const char *label)
{
    sx126x_chip_status_t chip_status = {0};
    sx126x_status_t status = sx126x_get_status(NULL, &chip_status);
    log_debug("selftest: %s status -> ret=%d mode=%d cmd=%d\r\n",
              label, (int)status, (int)chip_status.chip_mode, (int)chip_status.cmd_status);
}

static void selftest_log_errors(const char *label)
{
    sx126x_errors_mask_t errors = 0;
    sx126x_status_t status = sx126x_get_device_errors(NULL, &errors);
    log_debug("selftest: %s errors -> ret=%d mask=0x%04X "
              "(bit0=RC64K bit1=RC13M bit2=PLL_CAL bit3=ADC bit4=IMG bit5=XOSC_START bit6=PLL_LOCK)\r\n",
              label, (int)status, (unsigned)errors);
}

void lora_p2p_selftest(void)
{
    sx126x_status_t status;

    status = sx126x_reset(NULL);
    log_debug("selftest: reset -> %d\r\n", (int)status);

    // Chip mode should already read STBY_RC (2) here -- the reset command
    // itself lands the chip in standby-RC. If ret != 0 or mode/cmd look like
    // garbage (e.g. always 0xFF-ish), that's a straight SPI/BUSY problem,
    // before TCXO or anything RF-related is even in the picture.
    selftest_log_status("post-reset");
    selftest_log_errors("post-reset"); // XOSC_START error here is EXPECTED -- TCXO isn't powered yet.

    // LORA_P2P_TCXO_VOLTAGE/STARTUP_TIME_RTC_STEPS (1.7V / 5ms) are taken
    // directly from Waveshare's own official firmware for this module, not
    // guessed -- see the comment in lora_p2p.h. A sweep across all 8
    // supported voltages, including this one, previously failed to clear
    // XOSC_START at all, which points at a hardware fault on this specific
    // unit rather than a parameter to keep tuning.
    status = sx126x_set_dio3_as_tcxo_ctrl(NULL, LORA_P2P_TCXO_VOLTAGE, LORA_P2P_TCXO_STARTUP_TIME_RTC_STEPS);
    log_debug("selftest: set_dio3_as_tcxo_ctrl(1.7V, 5ms) -> %d\r\n", (int)status);

    status = sx126x_cal(NULL, SX126X_CAL_ALL);
    log_debug("selftest: cal(ALL) -> %d\r\n", (int)status);
    HAL_Delay(10); // generous margin; the next SPI transaction also busy-waits internally

    selftest_log_status("post-cal");
    selftest_log_errors("post-cal"); // XOSC_START/PLL_LOCK should be CLEAR now if the TCXO is actually working.
}
