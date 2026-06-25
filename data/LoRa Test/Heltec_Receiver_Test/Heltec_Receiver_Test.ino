#include <RadioLib.h>

// Heltec WiFi LoRa 32 V3 - Internal SX1262 Pin Definitions
const int PIN_NSS = 8;
const int PIN_DIO1 = 14;
const int PIN_NRST = 12;
const int PIN_BUSY = 13;

// Heltec V3 on-board LED (Active High)
const int LED_PIN = 35; 

// Create the radio module instance using Heltec V3 pin mapping
SX1262 radio = new Module(PIN_NSS, PIN_DIO1, PIN_NRST, PIN_BUSY);

void setup() {
  Serial.begin(115200); // 115200 is standard for ESP32/Heltec logs
  while (!Serial);      // Wait for serial monitor to open

  // Initialize the onboard LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Turn off LED initially

  // Initialize SX1262
  Serial.print(F("[Heltec V3] Initializing LoRa receiver... "));
  
  // MATCHING TRANSMITTER SETTINGS EXACTLY:
  // Carrier frequency: 915.0 MHz
  // Bandwidth: 125.0 kHz
  // Spreading Factor: 9
  // Coding Rate: 7 (CR 4/7)
  // Sync Word: 0x12 (Private network)
  // Output Power: 10 dBm
  // Current Limit: 100 mA
  int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 10, 100);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("Initialization success!"));
  } else {
    Serial.print(F("Initialization failed, code "));
    Serial.println(state);
    while (true); // Freeze if hardware setup fails
  }
}

void loop() {
  Serial.print(F("[Heltec V3] Listening for incoming packets... "));

  // String to store received data
  String str;

  // Receive packet (This is a blocking call, it waits until a packet is received)
  int state = radio.receive(str);

  if (state == RADIOLIB_ERR_NONE) {
    // Packet was successfully received!
    Serial.println(F("Success!"));

    // Blink the onboard LED for visual feedback
    digitalWrite(LED_PIN, HIGH);
    
    // Print the data of the packet
    Serial.print(F("[Data]:\t\t"));
    Serial.println(str);

    // Print RSSI (Received Signal Strength Indicator)
    Serial.print(F("[RSSI]:\t\t"));
    Serial.print(radio.getRSSI());
    Serial.println(F(" dBm"));

    // Print SNR (Signal-to-Noise Ratio)
    Serial.print(F("[SNR]:\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

    delay(200); // Brief flash duration
    digitalWrite(LED_PIN, LOW);

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // Timeout happens if you use non-blocking, but in basic receive() it usually waits.
    Serial.println(F("Timeout."));
    
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // Packet was received, but is corrupted
    Serial.println(F("CRC error! Packet corrupted."));

  } else {
    // Some other error occurred
    Serial.print(F("Failed, code "));
    Serial.println(state);
  }
}