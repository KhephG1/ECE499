#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEA_LEVEL_PRESSURE (1017.5) // Average sea level pressure in Victoria, BC

// Create BME280 object
Adafruit_BME280 BME280; 

void setup() {
  Serial.begin(9600);
  while(!Serial);    // Wait for USB serial port to open
  delay(1000);       // Extra delay
  unsigned status = BME280.begin(0x76); // Initialize the sensor (I2C address 0x76 when SDO is LOW, 0x77 when SDO is HIGH

  if (!status) {
    Serial.println("Could not find a valid BME280 sensor!");
    while (1); // Halt program execution if sensor isn't found
  }
  
  Serial.println("BME280 Sensor found successfully!\n");
}

void loop() {
  printValues();
  delay(2000); // Wait 2 seconds between readings
}

void printValues() {
  // Read and print Temperature
  Serial.print("Temperature = ");
  Serial.print(BME280.readTemperature());
  Serial.println(" *C");

  // Read and print Barometric Pressure
  Serial.print("Pressure    = ");
  Serial.print(BME280.readPressure() / 100.0F); // Convert Pascals (Pa) to Hectopascals (hPa)
  Serial.println(" hPa");

  // Read and print Altitude
  Serial.print("Approx. Alt = ");
  Serial.print(BME280.readAltitude(SEA_LEVEL_PRESSURE));
  Serial.println(" m");

  // Read and print Relative Humidity
  Serial.print("Humidity    = ");
  Serial.print(BME280.readHumidity());
  Serial.println(" %");

  Serial.println(); // Empty line for readability
}