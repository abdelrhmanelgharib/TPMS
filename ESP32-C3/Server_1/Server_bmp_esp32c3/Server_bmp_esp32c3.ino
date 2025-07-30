#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// ESP32-C3 specific configurations
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_TEMP "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_PRESS "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"

// BLE characteristics
BLECharacteristic* tempCharacteristic;
BLECharacteristic* pressCharacteristic;
BLEServer* pServer;

// Connection status
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Server callbacks for connection events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("📱 Client connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("📱 Client disconnected");
    }
};

void setup() {
  Serial.begin(115200);
  
  // ESP32-C3 specific: Add small delay for serial initialization
  delay(1000);
  
  Serial.println("🚗 Starting ESP32-C3 TPMS BLE Server...");
  
  // Initialize BLE Device
  BLEDevice::init("TPMS_Tire_1");
  
  // Set BLE power level (ESP32-C3 specific optimization)
  BLEDevice::setPower(ESP_PWR_LVL_P9); // Maximum power for better range
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create Temperature Characteristic
  tempCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_TEMP,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Add descriptor for notifications
  tempCharacteristic->addDescriptor(new BLE2902());
  tempCharacteristic->setValue("25.0");

  // Create Pressure Characteristic
  pressCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_PRESS,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Add descriptor for notifications
  pressCharacteristic->addDescriptor(new BLE2902());
  pressCharacteristic->setValue("2.1");

  // Start the service
  pService->start();

  // Setup advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  
  // ESP32-C3 specific advertising parameters
  pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections issue
  pAdvertising->setMaxPreferred(0x12);
  
  // Start advertising
  pAdvertising->start();

  Serial.println("🚗 ESP32-C3 BLE Server is now advertising...");
  Serial.print("📍 MAC Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
  Serial.println("⚡ Ready to accept connections!");
}

void loop() {
  static unsigned long lastUpdate = 0;
  static float temperature = 25.0;
  static float pressure = 2.1;
  
  // Update sensor values every 2 seconds
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    
    // Simulate varying sensor readings for ESP32-C3
    temperature = 25.0 + (random(-50, 50) / 100.0); // 24.5°C to 25.5°C
    pressure = 2.1 + (random(-10, 10) / 100.0);     // 2.0 to 2.2 bar
    
    // Convert to strings
    String tempStr = String(temperature, 1);
    String pressStr = String(pressure, 1);
    
    // Update characteristic values
    tempCharacteristic->setValue(tempStr.c_str());
    pressCharacteristic->setValue(pressStr.c_str());
    
    // Send notifications if device is connected
    if (deviceConnected) {
      tempCharacteristic->notify();
      pressCharacteristic->notify();
      
      Serial.printf("📊 Updated - Temp: %s°C, Pressure: %s bar\n", 
                    tempStr.c_str(), pressStr.c_str());
    } else {
      Serial.printf("📊 Data ready - Temp: %s°C, Pressure: %s bar (No client connected)\n", 
                    tempStr.c_str(), pressStr.c_str());
    }
  }
  
  // Handle connection state changes
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Give the bluetooth stack time to get ready
    pServer->startAdvertising(); // Restart advertising
    Serial.println("🔄 Restarting advertising...");
    oldDeviceConnected = deviceConnected;
  }
  
  // Connecting
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // ESP32-C3 specific: Small delay to prevent watchdog issues
  delay(10);
}