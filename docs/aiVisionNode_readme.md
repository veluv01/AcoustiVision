# AI Vision Node for Distributed Monitoring

This project transforms a Seeed Studio XIAO ESP32-C3 and a Grove Vision AI V2 module into a dedicated, wireless **AI Vision Sensor Node**. Its sole purpose is to detect the number of people in its field of view and stream this count over Bluetooth Low Energy (BLE).

This node is designed to be one part of a larger, distributed environmental monitoring system, where a central hub (like a Raspberry Pi) can subscribe to its data stream and combine it with data from other sensors (such as the `AcousticSensor` node).

## Features

*   **Person Detection:** Uses the Grove Vision AI V2 SenseCraft AI's `PeopleNet` model to count people in real-time.
*   **Wireless Streaming:** Acts as a BLE peripheral (GATT Server), broadcasting the person count once per second.
*   **Robust & Stable:** The code is optimized for reliability, correctly handling the AI module's boot-up sequence and BLE connection states.
*   **Low Power (Idle):** Uses a non-blocking architecture, allowing the CPU to be idle between inference cycles.
*   **Decoupled:** Designed to run independently, making the overall sensor network more resilient.

## Hardware Requirements

*   **Seeed Studio XIAO ESP32-C3**
*   **Grove Vision AI V2 Module** The module must have PeopleNet Detection model flashed using SenseCraft AI.
*   **Direct Connection:** The XIAO ESP32-C3 should be plugged directly into the female headers of the Grove Vision AI V2 module. This automatically handles Power, Ground, and **I2C** connections.
*   **USB-C Cable**: For programming and power. A stable power source (e.g., a powered USB hub or a 2A wall adapter) is recommended.

## Software & Library Dependencies

*   **Arduino IDE (2.x recommended)**
*   **ESP32 Board Support Package**: Must be installed in the Arduino IDE.
    1.  Go to `File > Preferences`.
    2.  Add the URL for the ESP32 boards to "Additional Board Manager URLs". The official URL can be found on the Espressif GitHub.
    3.  Go to `Tools > Board > Boards Manager...`, search for "esp32", and install the package by Espressif Systems.
*   **Required Arduino Libraries**:
    1.  **`Seeed_Arduino_SSCMA`**: Install via `Tools > Manage Libraries...`. This is the driver for the Grove AI V2 module.
    2.  **`BLE` (ESP32 Built-in)**: The required BLE libraries (`BLEDevice.h`, etc.) are included **automatically** with the ESP32 board package. **Do not** install the separate `ArduinoBLE` library, as it will cause conflicts.

## Setup and Installation

1.  **Hardware Assembly:** Firmly plug the XIAO ESP32-C3 into the headers on the Grove Vision AI V2 module.
2.  **Board Selection:** In the Arduino IDE, select `Tools > Board > esp32 > XIAO_ESP32C3`.
3.  **Code:** Copy the complete code from the `AINode_ESP32C3_Corrected.ino` sketch into your Arduino IDE.
4.  **Compile & Upload:** Connect the device via USB-C and upload the sketch.
5.  **Verification (Optional):** Open the Arduino **Serial Monitor** at **115200 baud**. You will see startup messages confirming that the AI module has initialized and that BLE advertising has started. Once a client connects, it will log the person count every second.

## BLE Service Specification

The device advertises with the name **`VisionSensor`**. A client can connect and subscribe to notifications to receive the data stream.

*   **Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
    *   This is the main "folder" for our sensor data.

*   **Characteristic UUID:** `beb5483e-36e1-4688-b7f5-ea07361b26a8`
    *   **Name:** Person Count
    *   **Data Type:** `uint8_t` (a single unsigned byte)
    *   **Value:** `0` to `255`, representing the number of people detected.
    *   **Properties:** `READ`, `NOTIFY`
    *   **Usage:** A client should subscribe to this characteristic to receive a notification every second with the updated person count.

## How to View the Data

You can use any standard BLE scanner application to view the data stream.

1.  **Recommended App:** **nRF Connect for Mobile** (for Android or iOS).
2.  **Scan:** Open the app and find the device named `VisionSensor`.
3.  **Connect:** Tap the "Connect" button.
4.  **Find the Service:** Locate the service with the UUID `4fafc201-...`.
5.  **Subscribe:** Find the characteristic with the UUID `beb5483e-...` and tap the "Subscribe" icon (looks like three downward arrows `↓↓↓`).
6.  **Observe:** The value will now update once per second, showing the raw byte value (e.g., `0x01` for one person, `0x03` for three people).
