#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1_SERVER1 "ee026418-ab66-4a49-bc25-3f7c2e8f1881" // Temp Server 1
#define CHARACTERISTIC_UUID_2_SERVER1 "6d3f910b-d335-421e-90cf-49ab9027a533" // Pressure Server 1
#define CHARACTERISTIC_UUID_1_SERVER2 "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Temp Server 2
#define CHARACTERISTIC_UUID_2_SERVER2 "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e" // Pressure Server 2

BLEAdvertisedDevice* myDevice1 = nullptr;
BLEAdvertisedDevice* myDevice2 = nullptr;
bool Server1 = 0;
bool Server2 = 0;
bool doConnect1 = false, doConnect2 = false;
bool connected1 = false, connected2 = false;
BLEClient* pClient1 = nullptr;
BLEClient* pClient2 = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic_1_1 = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic_1_2 = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic_2_1 = nullptr;
BLERemoteCharacteristic* pRemoteCharacteristic_2_2 = nullptr;

void notifyCallback_1(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("[Server 1] Received: ");
    Serial.println(String((char*)pData));
}

void notifyCallback_2(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("[Server 2] Received: ");
    Serial.println(String((char*)pData));
}

bool connectToServer(BLEClient*& pClient, BLEAdvertisedDevice* myDevice, const char* tempUUID, const char* pressUUID, void (*notifyCallback)(BLERemoteCharacteristic*, uint8_t*, size_t, bool), int serverNum) {
    pClient = BLEDevice::createClient();
    if (!pClient->connect(myDevice)) {
        Serial.printf("[Server %d] Connection failed.\n", serverNum);
        return false;
    }
    Serial.printf("[Server %d] Connected!\n", serverNum);
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (!pRemoteService) {
        Serial.printf("[Server %d] Service not found.\n", serverNum);
        pClient->disconnect();
        return false;
    }
    BLERemoteCharacteristic* tempChar = pRemoteService->getCharacteristic(tempUUID);
    BLERemoteCharacteristic* pressChar = pRemoteService->getCharacteristic(pressUUID);
    if (tempChar && tempChar->canNotify()) tempChar->registerForNotify(notifyCallback);
    if (pressChar && pressChar->canNotify()) pressChar->registerForNotify(notifyCallback);
    return true;
}

// class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
//     void onResult(BLEAdvertisedDevice advertisedDevice) {
//         if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
//             // if (!myDevice1) {
//             //     myDevice1 = new BLEAdvertisedDevice(advertisedDevice);
//             //     doConnect1 = true;
//             //     Serial.printf("ADV device 1\n");
//             // } 
//                else if (!myDevice2) {
//                 myDevice2 = new BLEAdvertisedDevice(advertisedDevice);
//                 doConnect2 = true;
//                 Serial.printf("ADV device 2\n");
//             }
//             BLEDevice::getScan()->stop();
//         }
//     }
// };

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.print("Found device: ");
      Serial.println(advertisedDevice.toString().c_str());
      // Check if this is a new device (by comparing addresses)
      if (!myDevice1) {
        myDevice1 = new BLEAdvertisedDevice(advertisedDevice);
        doConnect1 = true;
      } else if (!myDevice2 && (advertisedDevice.getAddress().toString() != myDevice1->getAddress().toString())) {
        myDevice2 = new BLEAdvertisedDevice(advertisedDevice);
        doConnect2 = true;
      }
      // Only stop scanning if both devices have been found
      if (myDevice1 != nullptr && myDevice2 != nullptr) {
        BLEDevice::getScan()->stop();
      }
    }
  }
};


void setup() {
    Serial.begin(115200);
    BLEDevice::init("ESP32_Client");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void loop() {
    if (doConnect1 && myDevice1) {
        connected1 = connectToServer(pClient1, myDevice1, CHARACTERISTIC_UUID_1_SERVER1, CHARACTERISTIC_UUID_2_SERVER1, notifyCallback_1, 1);
        doConnect1 = false;
        Serial.printf("Connected! to device 1\n");
    }
    if (doConnect2 && myDevice2) {
        connected2 = connectToServer(pClient2, myDevice2, CHARACTERISTIC_UUID_1_SERVER2, CHARACTERISTIC_UUID_2_SERVER2, notifyCallback_2, 2);
        doConnect2 = false;
        Serial.printf("Connected! Connected! to device 2\n");
    }
    delay(1000);
}
