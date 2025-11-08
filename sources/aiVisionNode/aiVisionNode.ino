/**
 * @file AIVisionNode_ESP32C3_.ino
 * @brief Dedicated AI Vision Node for a distributed environmental monitoring system.
 *
 * This firmware runs on a Seeed Studio XIAO ESP32-C3 board connected to a
 * Grove Vision AI V2 module. Its sole purpose is to perform person detection
 * and stream the resulting count over Bluetooth Low Energy (BLE).
 *
 * ARCHITECTURE:
 * 1.  HARDWARE: The Grove AI module is connected via the I2C bus.
 * 2.  AI INFERENCE: The SSCMA library is used to command the AI module. A non-blocking
 *     timer in the main loop calls the AI.invoke() function once per second.
 * 3.  DATA PARSING: The code iterates through the "boxes" returned by the AI module
 *     and counts only the detections with a class ID of 0 ("person").
 * 4.  BLE COMMUNICATION: The ESP32 acts as a BLE peripheral (GATT Server), advertising
 *     a custom service. When a central device (like a Raspberry Pi or smartphone)
 *     connects and subscribes, this node sends a BLE notification with the updated
 *     person count once per second.
 * 5.  STABILITY: A small delay is included in the main loop to ensure the ESP32's
 *     underlying FreeRTOS and BLE stack have sufficient processing time, preventing
 *     missed notifications.
 */

// --- Library Includes ---
#include <Wire.h>                  // Standard library for I2C communication.
#include <Seeed_Arduino_SSCMA.h>   // Seeed's library for controlling the Grove AI module.
#include <BLEDevice.h>             // Core components of the native ESP32 BLE stack.
#include <BLEServer.h>             // Components for creating a BLE peripheral/server.
#include <BLEUtils.h>              // Utility functions for the BLE stack.
#include <BLE2902.h>               // Specifically for the BLE Descriptor (0x2902) required to enable notifications.

// --- AI Module Configuration ---
SSCMA AI;                          // Create a global instance of the SSCMA library object.
const unsigned long AI_REQUEST_INTERVAL = 1000; // Poll the AI module once per second (1Hz).
unsigned long last_ai_request_time = 0; // Tracks the timestamp of the last AI poll.
int people_count = 0;              // Global variable to hold the latest valid person count.
const int PERSON_CLASS_ID = 0;     // The class ID for "person" in the PeopleNet model.

// --- BLE Configuration (using native ESP32 BLE API) ---
// These UUIDs (Universally Unique Identifiers) are custom values. You can generate
// your own at sites like uuidgenerator.net. They uniquely identify your service and characteristics.
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_PPL "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer = NULL;                   // Pointer to the global BLE server object.
BLECharacteristic *pCharacteristicPeople = NULL; // Pointer to our "people count" characteristic.
bool deviceConnected = false;                // Flag to track the BLE connection status.
bool oldDeviceConnected = false;             // Used to detect changes in the connection state.

// --- Server Callback Class for Connect/Disconnect Events ---
/**
 * @class MyServerCallbacks
 * @brief Handles events when a BLE client connects or disconnects.
 *
 * This class inherits from the library's BLEServerCallbacks and allows us to
 * run custom code when the connection status changes, updating our 'deviceConnected' flag.
 */
class MyServerCallbacks: public BLEServerCallbacks {
    // This function is called the moment a client connects.
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client Connected");
    }

    // This function is called the moment a client disconnects.
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client Disconnected");
    }
};

/**
 * @brief Arduino setup() function. Runs once at startup.
 */
void setup() {
  Serial.begin(115200);
  Serial.println("AI Vision Node Booting (Corrected BLE)...");

  // --- Initialize AI Module (I2C) ---
  Wire.begin(); // Initialize the I2C bus.
  delay(3000);  // CRITICAL: Wait for the Grove AI module's internal computer to boot up.
  if (AI.begin()) {
    Serial.println("Grove AI Module initialized successfully on I2C.");
  } else {
    Serial.println("FATAL: Failed to initialize Grove AI Module. Halting.");
    while (1); // Stop execution if the AI module is not found.
  }

  // --- BLE Server Setup ---

  // 1. Initialize the BLE device and set its public name.
  BLEDevice::init("AIVisionNode");

  // 2. Create the BLE Server.
  pServer = BLEDevice::createServer();
  // 3. Register our callback class to handle connection events.
  pServer->setCallbacks(new MyServerCallbacks());

  // 4. Create the BLE Service using its unique UUID.
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 5. Create a BLE Characteristic within the service.
  pCharacteristicPeople = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_PPL,
                      BLECharacteristic::PROPERTY_READ |   // Allows a client to read the value.
                      BLECharacteristic::PROPERTY_NOTIFY // Allows a client to subscribe for updates.
                    );

  // 6. Add a 0x2902 descriptor. This is REQUIRED by the BLE standard for notifications.
  // It's a special handle that the client (phone/RPi) writes to in order to
  // enable or disable the stream of notifications from this characteristic.
  pCharacteristicPeople->addDescriptor(new BLE2902());

  // 7. Start the service.
  pService->start();

  // 8. Configure and start advertising.
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); // Tell the world which service we offer.
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("BLE Advertising as 'VisionSensor'. Ready to connect.");
}

/**
 * @brief Arduino loop() function. Runs repeatedly.
 */
void loop() {
  // --- Handle Connection State Changes ---
  // This logic ensures that if a client disconnects, the device will automatically
  // become discoverable again by restarting the advertising process.
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // Give the BLE stack a moment to clean up resources.
      pServer->startAdvertising();
      Serial.println("Restarted advertising");
      oldDeviceConnected = deviceConnected;
  }
  // This logic detects a new connection.
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }

  // --- Main Logic: Poll AI and Notify once per second ---
  if (millis() - last_ai_request_time >= AI_REQUEST_INTERVAL) {
    last_ai_request_time = millis(); // Reset the timer for the next interval.

    // Ask the AI module to perform an inference.
    if (AI.invoke() == 0) { // A return code of 0 means success.
      
      // Correctly iterate through the results to count only persons.
      int current_person_count = 0;
      for (const auto& box : AI.boxes()) {
        // 'box.target' holds the class ID of the detected object.
        if (box.target == PERSON_CLASS_ID) {
          current_person_count++;
        }
      }
      people_count = current_person_count;
    }

    // Print the result to the local serial monitor for debugging.
    Serial.print("People Detected: ");
    Serial.println(people_count);

    // --- Send BLE Notification (only if a client is connected) ---
    if (deviceConnected) {
      // Set the new value for the characteristic. We cast to uint8_t as it's a single byte.
      pCharacteristicPeople->setValue((uint8_t)people_count);
      
      // Send the notification. This pushes the new value to any subscribed client.
      pCharacteristicPeople->notify();
      
      Serial.println("  -> Sent BLE Notification."); // Confirmation message.
    }
  }

  // --- CRITICAL DELAY ---
  // This short delay is essential. It yields processing time to the ESP32's
  // underlying tasks, including the Bluetooth stack, allowing it to handle
  // connections and send the notification packets reliably. Without this,
  // the 'loop()' can starve the BLE task, causing notifications to be missed.
  delay(10);
}
