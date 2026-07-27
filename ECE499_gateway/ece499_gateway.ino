#include <RadioLib.h>

// Heltec WiFi LoRa 32 V3 - SX1262 Pin Definitions
const int PIN_NSS   = 8;
const int PIN_DIO1  = 14;
const int PIN_NRST  = 12;
const int PIN_BUSY  = 13;

// On-board LED (Active High)
const int LED_PIN = 35;

// Create radio instance
SX1262 radio = new Module(PIN_NSS, PIN_DIO1, PIN_NRST, PIN_BUSY);

// --------------------------------------------------
// Configuration
// --------------------------------------------------

#define SERIAL_BAUD 115200

// LoRa parameters (MUST match transmitter!)
const float   LORA_FREQ_MHZ     = 915.0;
const float   LORA_BW_KHZ       = 125.0;
const uint8_t LORA_SF           = 9;
const uint8_t LORA_CR           = 7;      // 4/7
const uint8_t LORA_SYNC_WORD    = 0x12;   // Private network
const int8_t  LORA_POWER_DBM    = 10;
const uint8_t LORA_CURRENT_MA   = 100;

// New packet length
const size_t LORA_PAYLOAD_LEN = 22;

// --------------------------------------------------
// Sensor Data Structure
// --------------------------------------------------

struct SensorData {
    uint16_t scdCO2;
    int16_t  scdTemp;        // x100
    int16_t  scdHumidity;    // x100

    uint16_t bmeCO2eq;
    uint16_t bmeVOC;         // x100
    int16_t  bmeHumidity;    // x100
    int16_t  bmeTemp;        // x100
    uint16_t bmePressure;    // x10 (hPa)
    uint16_t  bmeStability;

    uint16_t adcVoltage;     // mV
    uint16_t  deviceID;
};

SensorData sensors = {};

// Receive flag
volatile bool packetReceived = false;

// --------------------------------------------------
// Helper Functions
// --------------------------------------------------

uint16_t readUInt16(const uint8_t* data) {
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

int16_t readInt16(const uint8_t* data) {
    return (int16_t)readUInt16(data);
}

void decodePacket(const uint8_t* payload, SensorData& data) {
    data.scdCO2       = readUInt16(&payload[0]);
    data.scdTemp      = readInt16(&payload[2]);
    data.scdHumidity  = readInt16(&payload[4]);

    data.bmeCO2eq     = readUInt16(&payload[6]);
    data.bmeVOC       = readUInt16(&payload[8]);
    data.bmeHumidity  = readInt16(&payload[10]);
    data.bmeTemp      = readInt16(&payload[12]);
    data.bmePressure  = readUInt16(&payload[14]);
    data.bmeStability = readUInt16(&payload[16]);

    data.adcVoltage   = readUInt16(&payload[18]);
    data.deviceID     = readUInt16(&payload[20]);
}

// --------------------------------------------------
// Interrupt Handler
// --------------------------------------------------

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void onPacketReceived() {
    packetReceived = true;
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("LoRa Gateway Starting...");

    int state = radio.begin(
        LORA_FREQ_MHZ,
        LORA_BW_KHZ,
        LORA_SF,
        LORA_CR,
        LORA_SYNC_WORD,
        LORA_POWER_DBM,
        LORA_CURRENT_MA
    );

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("SX1262 initialized successfully.");
    } else {
        Serial.print("Failed to initialize radio. Error: ");
        Serial.println(state);
        while (true);
    }

    radio.setDio1Action(onPacketReceived);

    state = radio.startReceive();

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Gateway Ready - Listening");
    } else {
        Serial.print("startReceive failed, code: ");
        Serial.println(state);
    }
}

// --------------------------------------------------
// Main Loop
// --------------------------------------------------

void loop() {

    if (!packetReceived)
        return;

    packetReceived = false;

    uint8_t buffer[LORA_PAYLOAD_LEN] = {0};

    int status = radio.readData(buffer, LORA_PAYLOAD_LEN);

    if (status == RADIOLIB_ERR_NONE) {

        decodePacket(buffer, sensors);

        float rssi = radio.getRSSI();
        float snr  = radio.getSNR();

        // Blink LED
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);

        // CSV output
        Serial.printf(
            "%u,%.2f,%.2f,%u,%.2f,%.2f,%.2f,%.1f,%u,%.2f,%.1f,%.1f,%u\n",
            sensors.scdCO2,
            sensors.scdTemp / 100.0f,
            sensors.scdHumidity / 100.0f,
            sensors.bmeCO2eq,
            sensors.bmeVOC / 100.0f,
            sensors.bmeHumidity / 100.0f,
            sensors.bmeTemp / 100.0f,
            sensors.bmePressure / 10.0f,
            sensors.bmeStability,
            sensors.adcVoltage / 100.0f,
            rssi,
            snr,
            sensors.deviceID
        );

    }
    else if (status == RADIOLIB_ERR_CRC_MISMATCH) {
        Serial.println("CRC Error - Corrupted packet");
    }
    else {
        Serial.print("Read failed, code: ");
        Serial.println(status);
    }

    // Resume listening
    radio.startReceive();
}