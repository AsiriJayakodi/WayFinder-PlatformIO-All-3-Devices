To use your `PayloadBuilder` library in an ESP32 project (PlatformIO or Arduino IDE), follow these steps:

---

### **1️⃣ Include the Library in Your Project**
Make sure your project has both `payload_builder.h` and `payload_builder.cpp` files.

---

### **2️⃣ Initialize the PayloadBuilder in Your Code**
You need to create an instance of `PayloadBuilder`, configure it with a source and destination ID, and then use the functions to generate different payloads.

#### **Example Usage in an ESP32 Sketch (`main.cpp` or `.ino` file):**
```cpp
#include <Arduino.h>
#include "payload_builder.h"

PayloadBuilder payload;

void setup() {
    Serial.begin(115200);
    payload.configure_device(0x01, 0x02); // Set Source ID: 1, Destination ID: 2

    // Create GPS payload
    std::vector<uint8_t> gpsPayload = payload.create_gps_payload(79.9005, 6.9271);
    Serial.print("GPS Payload: ");
    for (uint8_t byte : gpsPayload) {
        Serial.printf("%02X ", byte);
    }
    Serial.println();

    // Create predefined message payload
    std::vector<uint8_t> pMsgPayload = payload.create_p_msg_payload(0x05);
    Serial.print("Predefined Msg Payload: ");
    for (uint8_t byte : pMsgPayload) {
        Serial.printf("%02X ", byte);
    }
    Serial.println();

    // Create custom message payload
    std::vector<uint8_t> cMsgPayload = payload.create_c_msg_payload("Hello ESP32!");
    Serial.print("Custom Msg Payload: ");
    for (uint8_t byte : cMsgPayload) {
        Serial.printf("%02X ", byte);
    }
    Serial.println();

    // Get last payload size
    Serial.print("Last Payload Size: ");
    Serial.println(payload.get_last_payload_size());
}

void loop() {
    // Do nothing in the loop
}
```

---

### **3️⃣ Explanation of How This Works**
1. **Configure the Device:**
   ```cpp
   payload.configure_device(0x01, 0x02);
   ```
   This sets:
   - `sourceID = 0x01`
   - `destinationID = 0x02`

2. **Create Different Payloads:**
   - **GPS Payload (Type 1):**
     ```cpp
     std::vector<uint8_t> gpsPayload = payload.create_gps_payload(79.9005, 6.9271);
     ```
     This constructs a payload containing:
     - Type: `0x01` (GPS)
     - Source ID: `0x01`
     - Destination ID: `0x02`
     - Transmission ID: Random
     - Date & Time: Current
     - Data Length: `8` bytes
     - Longitude & Latitude (4 bytes each)
     - XOR Checksum (calculated automatically)

   - **Predefined Message Payload (Type 2):**
     ```cpp
     std::vector<uint8_t> pMsgPayload = payload.create_p_msg_payload(0x05);
     ```
     - Type: `0x02`
     - Message ID: `0x05`

   - **Custom Message Payload (Type 3):**
     ```cpp
     std::vector<uint8_t> cMsgPayload = payload.create_c_msg_payload("Hello ESP32!");
     ```
     - Type: `0x03`
     - Message: `"Hello ESP32!"` (encoded in bytes)

3. **Print the Payloads in HEX Format:**
   ```cpp
   for (uint8_t byte : gpsPayload) {
       Serial.printf("%02X ", byte);
   }
   ```
   This prints the payload as hexadecimal values.

4. **Get Last Payload Size:**
   ```cpp
   Serial.println(payload.get_last_payload_size());
   ```
   This returns the size of the most recent payload.

---

### **4️⃣ Expected Serial Output (Example)**
```
GPS Payload: 01 01 02 3F 92 24 65 07 E8 01 1E 10 1F 3F 9A 99 99 99 99 99 9A 40 40 
Predefined Msg Payload: 02 01 02 7D 34 07 E8 01 1E 10 1F 05 83
Custom Msg Payload: 03 01 02 2A 45 07 E8 01 1E 10 1F 0C 48 65 6C 6C 6F 20 45 53 50 33 32 21 56
Last Payload Size: 21
```

---

### **5️⃣ How to Use This in LoRa or BLE Communication**
Once you generate the payload, you can transmit it using LoRa or BLE:

#### **Send via LoRa (Example Using RadioLib)**
```cpp
#include <RadioLib.h>

SX1276 radio = new Module(10, 2, 9, 3);

void sendPayload(std::vector<uint8_t> payload) {
    radio.begin();
    radio.transmit(payload.data(), payload.size());
    Serial.println("LoRa Message Sent!");
}
```

#### **Send via BLE (Example Using ESP32 Bluetooth)**
```cpp
BLECharacteristic *pCharacteristic; // Assume you've set up BLE

void sendPayloadBLE(std::vector<uint8_t> payload) {
    pCharacteristic->setValue(payload.data(), payload.size());
    pCharacteristic->notify();
    Serial.println("BLE Message Sent!");
}
```

---

### **6️⃣ Summary**
- **Create an instance of `PayloadBuilder`.**
- **Configure source and destination IDs.**
- **Generate payloads for GPS, predefined messages, or custom messages.**
- **Print or send the payload using LoRa/BLE.**

Let me know if you need modifications or explanations! 🚀

