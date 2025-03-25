#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define SERVICE_UUID          "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1 "ee026418-ab66-4a49-bc25-3f7c2e8f1881" // Temperature
#define CHARACTERISTIC_UUID_2 "6d3f910b-d335-421e-90cf-49ab9027a533" // Pressure

static BLEAdvertisedDevice* myDevice;
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic_1;
static BLERemoteCharacteristic* pRemoteCharacteristic_2;
BLEClient* pClient;

// Callback function for receiving notifications
void notifyCallback_1(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String temperatureValue = String((char*)pData);
  Serial.print("Received Temperature: ");
  Serial.println(temperatureValue);
}

void notifyCallback_2(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String pressureValue = String((char*)pData);
  Serial.print("Received Pressure: ");
  Serial.println(pressureValue);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    Serial.println("Connected to server!");
  }
  void onDisconnect(BLEClient* pClient) {
    Serial.println("Disconnected from server!");
    connected = false;
    doScan = true;
  }
};

bool connectToServer() {
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  
  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect to server.");
    return false;
  }
  Serial.println("Connected to BLE server!");
  
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find our service UUID");
    pClient->disconnect();
    return false;
  }
  
  pRemoteCharacteristic_1 = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_1);
  pRemoteCharacteristic_2 = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_2);
  
  if (pRemoteCharacteristic_1 == nullptr || pRemoteCharacteristic_2 == nullptr) {
    Serial.println("Failed to find characteristics.");
    pClient->disconnect();
    return false;
  }
  
  if (pRemoteCharacteristic_1->canNotify()) {
    pRemoteCharacteristic_1->registerForNotify(notifyCallback_1);
  }
  if (pRemoteCharacteristic_2->canNotify()) {
    pRemoteCharacteristic_2->registerForNotify(notifyCallback_2);
  }
  
  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to BLE Server");
    } else {
      Serial.println("Failed to connect to BLE Server");
    }
    doConnect = false;
  }
  delay(1000);
}
