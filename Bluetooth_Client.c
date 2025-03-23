#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <SoftwareSerial.h>
 
// Define the RX and TX pins for the SoftwareSerial connection
// (Connect module TX to Arduino pin 10 and module RX to Arduino pin 11)
// Note: The Arduino Nano operates at 5V, so use a voltage divider on the Arduino TX (pin 11)
// to step the voltage down to 3.3V if necessary.
SoftwareSerial BTSerial(10, 11); // (RX, TX)
 
 #define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

Adafruit_BMP280 bmp; // I2C
//Adafruit_BMP280 bmp(BMP_CS); // hardware SPI
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);
void setup() {
  // Initialize hardware serial for communication with the computer
  Serial.begin(9600);
  //while (!Serial); // wait for serial monitor to open
  //Serial.println("Bluetooth Module Test");
    delay(4000); // give some time for the module to initialize
 
  // Initialize software serial for Bluetooth communication
  // Set this baud rate to the module's default (commonly 9600 for many CC2541 modules)
  BTSerial.begin(9600);
    //delay(4000); // give some time for the module to initialize
 
 // give some time for the module to initialize
 
  // Test the module by sending the "AT" command
  //BTSerial.println("AT+NAME");
  //Serial.println("Sent AT command to Bluetooth module.");
    Serial.println(F("BMP280 test"));
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin(0X76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}
 
void loop() {
 
  // If data comes from the Bluetooth module, send it to the Serial Monitor
  if (BTSerial.available()) {
    char c = BTSerial.read();
    Serial.write(c);
  }
 
  // If data is typed in the Serial Monitor, send it to the Bluetooth module
  if (Serial.available()) {
    char c = Serial.read();
    BTSerial.write(c);
  }
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature()); 
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! */
    Serial.println(" m");
    Serial.println();
    delay(2000);
}