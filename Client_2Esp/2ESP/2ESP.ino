#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TEMP_SERVER1 "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_PRESS_SERVER1 "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
#define CHARACTERISTIC_UUID_TEMP_SERVER2 "ee026418-ab66-4a49-bc25-3f7c2e8f1881"
#define CHARACTERISTIC_UUID_PRESS_SERVER2 "6d3f910b-d335-421e-90cf-49ab9027a533"

const char* MAC_SERVER1 = "10:51:DB:AD:DF:02";
const char* MAC_SERVER2 = "10:51:DB:AD:D6:E2";

BLEAdvertisedDevice* myDevice1 = nullptr;
BLEAdvertisedDevice* myDevice2 = nullptr;
bool doConnect1 = false, doConnect2 = false;
bool connected1 = false, connected2 = false;
BLEClient* pClient1 = nullptr;
BLEClient* pClient2 = nullptr;

unsigned long lastConnectionCheck = 0;
const unsigned long connectionCheckInterval = 3000;

void notifyCallback_1(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("[Received from Server 1] ");
  Serial.write(pData, length);
  Serial.println();
}

void notifyCallback_2(BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("[Received from Server 2] ");
  Serial.write(pData, length);
  Serial.println();
}

bool connectToServer(BLEClient*& pClient, BLEAdvertisedDevice* device, const char* tempUUID, const char* pressUUID, void (*notifyCallback)(BLERemoteCharacteristic*, uint8_t*, size_t, bool), int serverNum) {
  pClient = BLEDevice::createClient();
  if (!pClient->connect(device)) {
    Serial.printf("[Server %d] Connection failed\n", serverNum);
    return false;
  }

  delay(1000);
  BLERemoteService* pService = pClient->getService(SERVICE_UUID);
  if (!pService) {
    Serial.printf("[Server %d] Service not found\n", serverNum);
    pClient->disconnect();
    return false;
  }

  BLERemoteCharacteristic* tempChar = pService->getCharacteristic(tempUUID);
  if (tempChar && tempChar->canNotify()) {
    tempChar->registerForNotify(notifyCallback);
  }

  BLERemoteCharacteristic* pressChar = pService->getCharacteristic(pressUUID);
  if (pressChar && pressChar->canNotify()) {
    pressChar->registerForNotify(notifyCallback);
  }

  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    String mac = advertisedDevice.getAddress().toString().c_str();
    mac.toUpperCase();

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      if (mac == MAC_SERVER1 && !myDevice1) {
        myDevice1 = new BLEAdvertisedDevice(advertisedDevice);
        doConnect1 = true;
      } else if (mac == MAC_SERVER2 && !myDevice2) {
        myDevice2 = new BLEAdvertisedDevice(advertisedDevice);
        doConnect2 = true;
      }

      if (myDevice1 && myDevice2) {
        BLEDevice::getScan()->stop();
      }
    }
  }
};

void resetConnection(int serverNum) {
  if (serverNum == 1) {
    if (pClient1) { pClient1->disconnect(); delete pClient1; pClient1 = nullptr; }
    delete myDevice1; myDevice1 = nullptr; connected1 = false; doConnect1 = false;
  } else {
    if (pClient2) { pClient2->disconnect(); delete pClient2; pClient2 = nullptr; }
    delete myDevice2; myDevice2 = nullptr; connected2 = false; doConnect2 = false;
  }
}

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32_Client");

  BLEScan* scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  scan->setInterval(1349);
  scan->setWindow(449);
  scan->setActiveScan(true);
  scan->start(15, false);
}

void loop() {
  if (doConnect1 && myDevice1) {
    connected1 = connectToServer(pClient1, myDevice1, CHARACTERISTIC_UUID_TEMP_SERVER1, CHARACTERISTIC_UUID_PRESS_SERVER1, notifyCallback_1, 1);
    if (!connected1) resetConnection(1);
    doConnect1 = false;
  }

  if (doConnect2 && myDevice2) {
    connected2 = connectToServer(pClient2, myDevice2, CHARACTERISTIC_UUID_TEMP_SERVER2, CHARACTERISTIC_UUID_PRESS_SERVER2, notifyCallback_2, 2);
    if (!connected2) resetConnection(2);
    doConnect2 = false;
  }

  if (millis() - lastConnectionCheck > connectionCheckInterval) {
    lastConnectionCheck = millis();
    if (connected1 && !pClient1->isConnected()) {
      resetConnection(1);
      BLEDevice::getScan()->start(10, false);
    }
    if (connected2 && !pClient2->isConnected()) {
      resetConnection(2);
      BLEDevice::getScan()->start(10, false);
    }
  }

  static unsigned long lastScan = 0;
  if ((millis() - lastScan > 10000) && (!connected1 || !connected2)) {
    BLEDevice::getScan()->start(10, false);
    lastScan = millis();
  }

  delay(100);
}
