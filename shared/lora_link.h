#ifndef LORA_LINK_H
#define LORA_LINK_H

/*
 * Shared LoRa P2P link definition for the ECE499 node <-> gateway link.
 *
 * Included by both projects so the two ends cannot drift apart:
 *   - ECE499_node    (STM32U5 + Waveshare Core1262, Semtech sx126x driver)
 *   - ECE499_gateway (Heltec WiFi LoRa 32 V3, RadioLib)
 *
 * A LoRa receiver only demodulates a packet when the frequency, spreading
 * factor, bandwidth, coding rate, sync word, header type, CRC and IQ polarity
 * all agree with the transmitter, so every one of those lives here rather than
 * in either main file. Change a value here and rebuild BOTH projects.
 *
 * The values below match data/LoRa Test/, the bench pair that was confirmed
 * working on this hardware.
 */

/* -- RF parameters ------------------------------------------------------- */

/*
 * 915.0 MHz, centre of the 902-928 MHz ISM band. Both radios are the 915M
 * variant; the 868M Core1262 is a different part and will not tune here.
 */
#define LORA_LINK_FREQ_HZ        915000000UL
#define LORA_LINK_FREQ_MHZ       915.0f

#define LORA_LINK_BW_KHZ         125.0f
#define LORA_LINK_SF             7
#define LORA_LINK_CR_DENOM       7  /* coding rate 4/7 */
#define LORA_LINK_PREAMBLE_SYMB  8

/*
 * 0x12 = private network. Both drivers take this nibble-packed form and expand
 * it to the 0x1424 register value, so the same constant works on both ends.
 */
#define LORA_LINK_SYNC_WORD      0x12

#define LORA_LINK_CRC_ON         1
#define LORA_LINK_INVERT_IQ      0
#define LORA_LINK_EXPLICIT_HDR   1

/* -- Payload ------------------------------------------------------------- */

/*
 * Fixed-length sensor payload, little-endian, no padding. Byte offsets are
 * shared rather than expressed as a packed struct so neither end depends on
 * its compiler's layout rules.
 *
 * Scaled integers are used instead of floats to keep the packet small; divide
 * by the scale noted on each field to recover the physical value.
 */
#define LORA_LINK_PAYLOAD_LEN    21

#define LORA_PL_OFF_SCD_CO2      0   /* u16, ppm                  (SCD40)  */
#define LORA_PL_OFF_SCD_TEMP     2   /* i16, degC x100            (SCD40)  */
#define LORA_PL_OFF_SCD_HUM      4   /* i16, %RH x100             (SCD40)  */
#define LORA_PL_OFF_BME_CO2EQ    6   /* u16, ppm                  (BSEC)   */
#define LORA_PL_OFF_BME_VOC      8   /* u16, ppm x100             (BSEC)   */
#define LORA_PL_OFF_BME_HUM      10  /* i16, %RH x100             (BSEC)   */
#define LORA_PL_OFF_BME_TEMP     12  /* i16, degC x100            (BSEC)   */
#define LORA_PL_OFF_BME_PRESS    14  /* u16, hPa x10              (BSEC)   */
#define LORA_PL_OFF_BME_STAB     16  /* u16, 0 = warming up, 1 = stable     */
#define LORA_PL_OFF_VBATT        18
#define LORA_PL_OFF_NODEID       20
#endif /* LORA_LINK_H */
