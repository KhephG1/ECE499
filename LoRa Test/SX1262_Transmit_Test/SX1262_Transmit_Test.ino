#include <RadioLib.h>

// Button and LED pins
const int buttonA = 13;
const int buttonB = 12;
const int led = 14;

// SX1262 Pin Definitions
const int PIN_NSS = 5;
const int PIN_DIO1 = 2;
const int PIN_NRST = 15;
const int PIN_BUSY = 4;

// Create the radio module instance
// SX1262 radio = new Module(CS, DIO1, NRST, BUSY);
SX1262 radio = new Module(PIN_NSS, PIN_DIO1, PIN_NRST, PIN_BUSY);

void setup() {
  Serial.begin(9600);
  
  // Initialize Buttons and LED
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonB, INPUT_PULLUP);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH); // LED off (assuming active-low, or on depending on setup)

  // Initialize SX1262
  Serial.print(F("[SX1262] Initializing ... "));
  
  // Carrier frequency: 915.0 MHz (Change to 433.0 or 868.0 depending on your region)
  // Bandwidth: 125.0 kHz
  // Spreading Factor: 9
  // Coding Rate: 7 (CR 4/7)
  // Sync Word: 0x12 (Private network)
  // Output Power: 10 dBm
  // Current Limit: 100 mA
  int state = radio.begin(915.0, 125.0, 9, 7, 0x12, 22, 100);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true); // Freeze program if radio initialization fails
  }
}

void loop() {
  // Check Button A
  if (digitalRead(buttonA) == LOW) {
    digitalWrite(led, LOW); // Visual feedback
    Serial.println(F("[SX1262] Transmitting 'Message A' ..."));
    
    // Transmit the packet
    int state = radio.transmit("Message A");
    
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("Transmission success!"));
    } else {
      Serial.print(F("Transmission failed, code "));
      Serial.println(state);
    }

    delay(1000); // Debounce / cooldown
    digitalWrite(led, HIGH);
  }

  // Check Button B
  else if (digitalRead(buttonB) == LOW) {
    digitalWrite(led, LOW);
    Serial.println(F("[SX1262] Transmitting 'Message B' ..."));
    
    // Transmit the packet
    int state = radio.transmit("Message B");
    
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("Transmission success!"));
    } else {
      Serial.print(F("Transmission failed, code "));
      Serial.println(state);
    }

    delay(1000);
    digitalWrite(led, HIGH);
  }
}