#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

// Define service and characteristic UUIDs for both servers
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_1_SERVER1  "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Temp Server 1
#define CHARACTERISTIC_UUID_2_SERVER1  "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e" // Pressure Server 1
#define CHARACTERISTIC_UUID_1_SERVER2  "ee026418-ab66-4a49-bc25-3f7c2e8f1881"// Temp Server 2
#define CHARACTERISTIC_UUID_2_SERVER2  "6d3f910b-d335-421e-90cf-49ab9027a533"// Pressure Server 2

// Variables for storing the advertised server devices and their connection states
BLEAdvertisedDevice* myDevice1 = nullptr;
BLEAdvertisedDevice* myDevice2 = nullptr;
bool doConnect1 = false, doConnect2 = false;
bool connected1 = false, connected2 = false;
BLEClient* pClient1 = nullptr;
BLEClient* pClient2 = nullptr;

// Timing control for connection status check
unsigned long lastConnectionCheck = 0;
const unsigned long connectionCheckInterval = 3000; // in milliseconds

// Callback for receiving notifications from Server 1
void notifyCallback_1(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String value = "";
  for (size_t i = 0; i < length; i++) value += (char)pData[i];
  Serial.print("[Server 1] Received: ");
  Serial.println(value);
}

// Callback for receiving notifications from Server 2
void notifyCallback_2(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  String value = "";
  for (size_t i = 0; i < length; i++) value += (char)pData[i];
  Serial.print("[Server 2] Received: ");
  Serial.println(value);
}

// Function to connect to a BLE server and subscribe to characteristics
bool connectToServer(BLEClient*& pClient, BLEAdvertisedDevice* myDevice, const char* tempUUID, const char* pressUUID, void (*notifyCallback)(BLERemoteCharacteristic*, uint8_t*, size_t, bool), int serverNum) {
  pClient = BLEDevice::createClient();

  Serial.printf("[Server %d] Attempting to connect...\n", serverNum);
  if (!pClient->connect(myDevice)) {
    Serial.printf("[Server %d] Failed to connect.\n", serverNum);
    return false;
  }
  Serial.printf("[Server %d] Connected!\n", serverNum);
  // Add a delay to allow the server to finish initializing
   delay(1000);  // Try increasing to 1000 if still flaky

  // Discover the service
  Serial.printf("[Server %d] Discovering service...\n", serverNum);
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (!pRemoteService) {
    Serial.printf("[Server %d] Service not found. Disconnecting.\n", serverNum);
    pClient->disconnect();
    return false;
  }

  // Discover characteristics
  Serial.printf("[Server %d] Getting temperature characteristic...\n", serverNum);
  BLERemoteCharacteristic* tempChar = pRemoteService->getCharacteristic(tempUUID);
  if (!tempChar) {
    Serial.printf("[Server %d] Temperature characteristic not found.\n", serverNum);
  } else if (tempChar->canNotify()) {
    tempChar->registerForNotify(notifyCallback);
    Serial.printf("[Server %d] Subscribed to temperature notifications.\n", serverNum);
  }

  Serial.printf("[Server %d] Getting pressure characteristic...\n", serverNum);
  BLERemoteCharacteristic* pressChar = pRemoteService->getCharacteristic(pressUUID);
  if (!pressChar) {
    Serial.printf("[Server %d] Pressure characteristic not found.\n", serverNum);
  } else if (pressChar->canNotify()) {
    pressChar->registerForNotify(notifyCallback);
    Serial.printf("[Server %d] Subscribed to pressure notifications.\n", serverNum);
  }
  
  return true;
}

// Custom callback class to process advertised BLE devices
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.print("Found device: ");
      Serial.println(advertisedDevice.toString().c_str());

      // Assign to first available slot, avoiding duplicates
      if (!myDevice1) {
        myDevice1 = new BLEAdvertisedDevice(advertisedDevice);
        doConnect1 = true;
      } else if (!myDevice2 && (advertisedDevice.getAddress().toString() != myDevice1->getAddress().toString())) {
        myDevice2 = new BLEAdvertisedDevice(advertisedDevice);
        doConnect2 = true;
      }

      // Stop scanning once both devices are discovered
      if (myDevice1 != nullptr && myDevice2 != nullptr) {
        BLEDevice::getScan()->stop();
      }
    }
  }
};

void setup() {
  Serial.begin(115200); // Start serial communication

  BLEDevice::init("ESP32_Client"); // Initialize BLE client device

  // Start scanning for devices
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // Use active scan to get more details
  pBLEScan->start(5, false); // Scan for 5 seconds (non-continuous)
}

void loop() {
  // Attempt connection to Server 1 if flagged
  if (doConnect1 && myDevice1) {
    connected1 = connectToServer(pClient1, myDevice1, CHARACTERISTIC_UUID_1_SERVER1, CHARACTERISTIC_UUID_2_SERVER1, notifyCallback_1, 1);
    doConnect1 = false;
    delay(1000); // Allow time before starting connection to second server
  }

  // Attempt connection to Server 2 if flagged
  if (doConnect2 && myDevice2) {
    connected2 = connectToServer(pClient2, myDevice2, CHARACTERISTIC_UUID_1_SERVER2, CHARACTERISTIC_UUID_2_SERVER2, notifyCallback_2, 2);
    doConnect2 = false;
  }

  // Periodically check connection status every 3 seconds
  if (millis() - lastConnectionCheck > connectionCheckInterval) {
    lastConnectionCheck = millis();

    // If Server 1 is disconnected, trigger reconnection
    if (connected1 && !pClient1->isConnected()) {
      Serial.println("[Server 1] Disconnected.");
      connected1 = false;
      doConnect1 = true;
    }

    // If Server 2 is disconnected, trigger reconnection
    if (connected2 && !pClient2->isConnected()) {
      Serial.println("[Server 2] Disconnected.");
      connected2 = false;
      doConnect2 = true;
    }
  }

  delay(100); // Small delay to keep loop responsive
}
