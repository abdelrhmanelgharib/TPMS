#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// ESP32-C3 specific configurations for Server 2
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_TEMP "ee026418-ab66-4a49-bc25-3f7c2e8f1881"  // Server 2 Temperature
#define CHARACTERISTIC_PRESS "6d3f910b-d335-421e-90cf-49ab9027a533" // Server 2 Pressure

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
  
  Serial.println("🚙 Starting ESP32-C3 TPMS Server 2...");
  
  // Initialize BLE Device
  BLEDevice::init("TPMS_Tire_2");
  
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
  tempCharacteristic->setValue("28.0");

  // Create Pressure Characteristic
  pressCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_PRESS,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Add descriptor for notifications
  pressCharacteristic->addDescriptor(new BLE2902());
  pressCharacteristic->setValue("2.3");

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

  Serial.println("🚙 ESP32-C3 TPMS Server 2 is now advertising...");
  Serial.print("📍 MAC Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());
  Serial.println("⚡ Ready to accept connections!");
}

void loop() {
  static unsigned long lastUpdate = 0;
  static float temperature = 28.0;  // Different starting temp for Server 2
  static float pressure = 2.3;     // Different starting pressure for Server 2
  
  // Update sensor values every 2 seconds
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    
    // Simulate varying sensor readings for Server 2 (different range)
    temperature = 28.0 + (random(-60, 60) / 100.0); // 27.4°C to 28.6°C
    pressure = 2.3 + (random(-15, 15) / 100.0);     // 2.15 to 2.45 bar
    
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
      
      Serial.printf("📊 Server 2 Updated - Temp: %s°C, Pressure: %s bar\n", 
                    tempStr.c_str(), pressStr.c_str());
    } else {
      Serial.printf("📊 Server 2 Data ready - Temp: %s°C, Pressure: %s bar (No client connected)\n", 
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