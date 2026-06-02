#include <Arduino.h>
#include <Wire.h>
#include "ScioSense_ENS160.h"  // ENS160 library
#include <PTSolns_AHTx.h>
#define ENS160_SAMPLING_PERIOD 1000
#define ENS160_CS_PIN 13
ScioSense_ENS160      ens160(ENS160_I2CADDR_0);
PTSolns_AHTx aht;

// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENS160_CS_PIN, OUTPUT);
  digitalWrite(ENS160_CS_PIN, HIGH);
  Serial.begin(115200); 
  while (!Serial) {}
  Serial.println("------------------------------------------------------------");
  Serial.println("ENS160 - Digital air quality sensor");
  Serial.println();
  Serial.println("Sensor readout in standard mode");
  Serial.println();
  Serial.println("------------------------------------------------------------");
  delay(1000);

  Serial.print("ENS160...");
  ens160.begin(); // pass boolean true to enable debug mode
  Serial.println(ens160.available() ? "done." : "failed!");
  //initialize the ENS 160
  if (ens160.available()) {
    // Print ENS160 versions
    Serial.print("\tRev: "); Serial.print(ens160.getMajorRev());
    Serial.print("."); Serial.print(ens160.getMinorRev());
    Serial.print("."); Serial.println(ens160.getBuild());
  
    Serial.print("\tStandard mode ");
    Serial.println(ens160.setMode(ENS160_OPMODE_STD) ? "done." : "failed!");
  }

  //initialize the AHT21
  if (!aht.begin()) { Serial.println("AHT begin failed"); for(;;){Serial.println("AHT21 init failed");} };
}

void loop() {
  //read TVOC and eC02 from the ENS160 and print over serial
   if (ens160.available()) {
    ens160.measure(true);
    ens160.measureRaw(true);
  
    Serial.print("AQI: ");Serial.print(ens160.getAQI());Serial.print(", ");
    Serial.print("TVOC: ");Serial.print(ens160.getTVOC());Serial.print("ppb, ");
    Serial.print("eCO2: ");Serial.print(ens160.geteCO2());Serial.print("ppm, ");
    Serial.print("R HP0: ");Serial.print(ens160.getHP0());Serial.print("Ohm, ");
    Serial.print("R HP1: ");Serial.print(ens160.getHP1());Serial.print("Ohm, ");
    Serial.print("R HP2: ");Serial.print(ens160.getHP2());Serial.print("Ohm, ");
    Serial.print("R HP3: ");Serial.print(ens160.getHP3());Serial.print("Ohm, ");
  }
  //read temperature and humidity from the AHT21 and print over serial
  float t, h;
  AHTxStatus st = aht.readTemperatureHumidity(t, h, 120);
  if (st == AHTX_OK) {
    Serial.print("T_C="); Serial.print(t, 2); Serial.print(", ");
    Serial.print(" RH_="); Serial.println(h, 2);
  } else {
    Serial.print("Error="); Serial.println((int)st);
  }
  ens160.set_envdata(t,h); // set the temperature and humidity data for compensation
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the built-in LED to indicate Uno operation 
  delay(ENS160_SAMPLING_PERIOD); // configures the sampling rate
}