#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_BMP280.h>

// Initialize BLE and BMP280
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic_1 = NULL; // Temperature
BLECharacteristic* pCharacteristic_2 = NULL; // Pressure
BLE2902 *pBLE2902_1;
BLE2902 *pBLE2902_2;
Adafruit_BMP280 bmp;

bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1 "ee026418-ab66-4a49-bc25-3f7c2e8f1881" // Temperature
#define CHARACTERISTIC_UUID_2 "6d3f910b-d335-421e-90cf-49ab9027a533" // Pressure

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  Serial.begin(115200);
  if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BMP280 sensor!");
    while (1) delay(10);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  BLEDevice::init("ESP32_BMP280");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic_1 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_1,
                      BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic_2 = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_2,
                      BLECharacteristic::PROPERTY_NOTIFY);

  pBLE2902_1 = new BLE2902();
  pBLE2902_1->setNotifications(true);
  pCharacteristic_1->addDescriptor(pBLE2902_1);

  pBLE2902_2 = new BLE2902();
  pBLE2902_2->setNotifications(true);
  pCharacteristic_2->addDescriptor(pBLE2902_2);

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
  Serial.println("Waiting for client connection...");
}

void loop() {
  if (deviceConnected) {
    float temperature = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0F;

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");

    pCharacteristic_1->setValue(temperature);
    pCharacteristic_1->notify();
    pCharacteristic_2->setValue(pressure);
    pCharacteristic_2->notify();

    delay(1000);
  }
  
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}
