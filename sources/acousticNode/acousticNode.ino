/**
 * @file AcousticMonitor_Arduino_BLE.ino
 * @brief Main application file for the A-Weighted SPL Meter with BLE support.
 * 
 * This sketch initializes the microphone hardware, collects audio samples in the
 * background using DMA and interrupts, feeds the data to the SPL_Meter class for
 * processing, and broadcasts the results via Bluetooth Low Energy (BLE).
 */

#include <SilabsMicrophoneAnalog.h>
#include <ArduinoBLE.h>
#include "SPL_Meter.h"

// =============================================================================
// --- HARDWARE CONFIGURATION for Seeed Studio XIAO MG24 ---
// =============================================================================
#define MIC_DATA_PIN PC9
#define MIC_PWR_PIN PC8

// The number of samples to collect in each buffer. This value was increased
// to 256 to provide the CPU with enough time to complete all DSP calculations,
// preventing system instability under heavy load (e.g., loud sounds).
#define NUM_SAMPLES 256  // This MUST match the value in SPL_Meter.h

// =============================================================================
// --- BLE CONFIGURATION ---
// =============================================================================
// Custom UUID for Environmental Sensing Service (or use standard 0x181A)
#define BLE_SERVICE_UUID "19B10000-E8F2-537E-4F6C-D104768A1214"
// Characteristic UUID for SPL reading
#define BLE_SPL_CHAR_UUID "19B10001-E8F2-537E-4F6C-D104768A1214"

// BLE update interval in milliseconds (how often to send notifications)
#define BLE_UPDATE_INTERVAL 500  // 500ms = 2 updates per second

// =============================================================================
// --- Global Objects ---
MicrophoneAnalog micAnalog(MIC_DATA_PIN, MIC_PWR_PIN);
SPL_Meter splMeter;

// BLE Service and Characteristic
BLEService splService(BLE_SERVICE_UUID);
BLEFloatCharacteristic splCharacteristic(BLE_SPL_CHAR_UUID, BLERead | BLENotify);

// --- Buffers for Microphone Library ---
// These buffers are used directly by the microphone library's DMA controller.
// 'mic_buffer' is actively being written to by the DMA, while 'mic_buffer_local'
// holds the last completed buffer for safe processing.
uint32_t mic_buffer[NUM_SAMPLES];
uint32_t mic_buffer_local[NUM_SAMPLES];

// This flag signals the main loop that a new buffer of data is ready.
// It is declared 'volatile' because it is modified in an interrupt and read
// in the main loop, which prevents the compiler from making unsafe optimizations.
volatile bool data_ready_flag = false;

// Timing variables for BLE updates
unsigned long lastBleUpdate = 0;
float currentDbaSpl = 0.0;
bool bleConnected = false;

/**
 * @brief Interrupt Callback Function.
 * This function is called automatically by the microphone library from an
 * interrupt context whenever the DMA has finished filling a complete buffer.
 * It must be as fast as possible.
 */
void mic_samples_ready_cb() {
  // Quickly copy the completed buffer to our local buffer for processing.
  memcpy(mic_buffer_local, mic_buffer, NUM_SAMPLES * sizeof(uint32_t));
  data_ready_flag = true;  // Signal the main loop to start processing.
}

/**
 * @brief Initialize BLE functionality.
 * Sets up the BLE service, characteristic, and starts advertising.
 */
void setupBLE() {
  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);  // Halt if BLE initialization fails
  }

  // Set device name (visible during scanning)
  BLE.setLocalName("SPL_Meter");
  BLE.setDeviceName("A-Weighted SPL Meter");

  // Set the advertised service
  BLE.setAdvertisedService(splService);

  // Add characteristic to service
  splService.addCharacteristic(splCharacteristic);

  // Add service to BLE stack
  BLE.addService(splService);

  // Set initial value
  splCharacteristic.writeValue(0.0f);

  // Start advertising
  BLE.advertise();

  Serial.println("BLE initialized and advertising...");
  Serial.println("Device name: SPL_Meter");
}

/**
 * @brief Handle BLE connection events and notifications.
 */
void handleBLE() {
  // Poll for BLE events
  BLEDevice central = BLE.central();

  // Check if a new device connected
  if (central) {
    if (!bleConnected) {
      bleConnected = true;
      Serial.print("Connected to central: ");
      Serial.println(central.address());
    }

    // Update BLE characteristic at specified interval
    unsigned long currentMillis = millis();
    if (currentMillis - lastBleUpdate >= BLE_UPDATE_INTERVAL) {
      lastBleUpdate = currentMillis;
      
      // Update the characteristic value
      splCharacteristic.writeValue(currentDbaSpl);
      
      // Print to serial for debugging
      Serial.print("BLE Update - SPL: ");
      Serial.print(currentDbaSpl, 2);
      Serial.println(" dBA");
    }
  } else if (bleConnected) {
    // Device disconnected
    bleConnected = false;
    Serial.println("Disconnected from central");
  }
}

/**
 * @brief Arduino setup() function. Runs once at startup.
 */
void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial to be ready.

  Serial.println("=================================");
  Serial.println("A-Weighted SPL Meter with BLE");
  Serial.println("=================================");

  // Initialize our SPL Meter's internal DSP components.
  splMeter.begin();
  Serial.println("SPL Meter initialized...");

  // Initialize BLE
  setupBLE();

  // Initialize the microphone hardware library.
  micAnalog.begin(mic_buffer, NUM_SAMPLES);
  Serial.println("Microphone library initialized...");

  // Start continuous, non-blocking sampling. The hardware will now collect
  // audio data in the background and call our callback function when ready.
  micAnalog.startSampling(mic_samples_ready_cb);
  Serial.println("Sampling started...");
  Serial.println("=================================");
}

/**
 * @brief Arduino loop() function. Runs repeatedly.
 */
void loop() {
  // Handle BLE events (connection, disconnection, notifications)
  handleBLE();

  // This is an efficient, event-driven loop that does nothing but wait for
  // the interrupt to signal that new data is available.
  if (data_ready_flag) {
    data_ready_flag = false;  // Reset the flag immediately.

    // A new buffer is ready; pass it to our SPL_Meter object for processing.
    splMeter.process(mic_buffer_local);

    // Get the final, smoothed result from the SPL_Meter.
    currentDbaSpl = splMeter.getSmoothedDbaSpl();

    // Print the result to the Serial Monitor (less frequent to reduce overhead)
    static unsigned long lastSerialPrint = 0;
    if (millis() - lastSerialPrint >= 1000) {  // Print every 1 second
      lastSerialPrint = millis();
      Serial.print("Smoothed SPL: ");
      Serial.print(currentDbaSpl, 2);  // Print with 2 decimal places.
      Serial.println(" dBA");
    }
  }
}
