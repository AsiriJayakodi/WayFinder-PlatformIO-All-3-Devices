#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "payload_builder.h"
#include <vector>
#include <string>
#include <Wire.h>

// ----- Predefined Emergency Messages -----
// Using C-strings for Arduino compatibility.
const char* emergencyMessages[10] = {
  "I'm OK",
  "I need water",
  "I need food",
  "I need medical assistance",
  "I'm lost, send help",
  "I am injured",
  "There is a fire nearby",
  "I need shelter",
  "I am trapped, please rescue",
  "Send my location to the rescue team"
};

// Helper function to return a message as an Arduino String.
// Expects a 1-based index, converts it to 0-based.
String getMessage(int index) {
  int adjustedIndex = index - 1;
  if (adjustedIndex >= 0 && adjustedIndex < 10) {
    return String(emergencyMessages[adjustedIndex]);
  } else {
    return "Invalid index! Please enter a number between 1 and 10.";
  }
}

// ----- Helper Function for RSSI Status -----
// This function classifies the RSSI value into one of four statuses.
String getRSSIStatus(int rssi) {
  // Adjust these thresholds as needed:
  if (rssi > -65) return "Excellent";
  else if (rssi > -75) return "Good";
  else if (rssi > -85) return "Fair";
  else return "Weak";
}

// ----- LoRa Module Pin Definitions -----
const int csPin    = 5;    // Chip Select
const int resetPin = 14;   // Reset
const int irqPin   = 2;    // IRQ (DIO0)

// Frequency for the SX1278 (typically 433E6 in many regions)
const long frequency = 433E6;

// ----- Global Instance of PayloadBuilder -----
// (Assumes your library’s class is named PayloadBuilder)
PayloadBuilder payloadBuilder;

// ----- FreeRTOS Queue Handle -----
// This queue will pass an integer (the msgID) from the serial task to the transmit task.
QueueHandle_t msgQueue;

// --------------------------------------------------------
// Task 1: LoRa Receive Task (handles both GPS and predefined/custom messages)
// --------------------------------------------------------
void LoRaReceiveTask(void* pvParameters) {
  for (;;) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      std::vector<uint8_t> payload;
      while (LoRa.available()) {
        payload.push_back(LoRa.read());
      }
      
      // Identify payload type and check checksum using your library function.
      uint8_t type = payloadBuilder.identify_type_and_check_checksum(payload);

      if (type == 0x01) {  // GPS Payload
        // Decode GPS data and general payload details.
        PayloadBuilder::GPSData gpsData = payloadBuilder.decode_gps_payload(payload);
        PayloadBuilder::PayloadDetails details = payloadBuilder.get_payload_details(payload);
        
        Serial.println("---- Received GPS Payload ----");
        Serial.print("Source ID: "); Serial.println(details.sourceID);
        Serial.print("Destination ID: "); Serial.println(details.destinationID);
        Serial.print("Transmission ID: "); Serial.println(details.transmissionID);
        Serial.print("Date/Time: ");
        for (int i = 0; i < 6; i++) {
          Serial.print(details.dateTime[i]);
          Serial.print(" ");
        }
        Serial.println();
        Serial.print("Longitude: "); Serial.println(gpsData.longitude, 6);
        Serial.print("Latitude: "); Serial.println(gpsData.latitude, 6);
        Serial.println("------------------------------");
      }
      else if (type == 0x02) {  // Predefined Message Payload
        PayloadBuilder::PMsgData pMsgData = payloadBuilder.decode_p_msg_payload(payload);
        PayloadBuilder::PayloadDetails details = payloadBuilder.get_payload_details(payload);
        
        Serial.println("---- Received Predefined Message Payload ----");
        Serial.print("Source ID: "); Serial.println(details.sourceID);
        Serial.print("Destination ID: "); Serial.println(details.destinationID);
        Serial.print("Transmission ID: "); Serial.println(details.transmissionID);
        Serial.print("Date/Time: ");
        for (int i = 0; i < 6; i++) {
          Serial.print(details.dateTime[i]);
          Serial.print(" ");
        }
        Serial.println();
        // pMsgData.msgID is stored as 0-based, so add 1 for display.
        int displayMsgID = pMsgData.msgID + 1;
        Serial.print("Message ID: "); Serial.println(displayMsgID);
        // Get the corresponding message text.
        String receivedMsgText = getMessage(displayMsgID);
        Serial.print("Message Text: "); Serial.println(receivedMsgText);
        Serial.println("---------------------------------------------");
      }
      else if (type == 0x03) {  // Custom Message Payload
        PayloadBuilder::CMsgData cMsgData = payloadBuilder.decode_c_msg_payload(payload);
        PayloadBuilder::PayloadDetails details = payloadBuilder.get_payload_details(payload);
        
        Serial.println("---- Received Custom Message Payload ----");
        Serial.print("Source ID: "); Serial.println(details.sourceID);
        Serial.print("Destination ID: "); Serial.println(details.destinationID);
        Serial.print("Transmission ID: "); Serial.println(details.transmissionID);
        Serial.print("Date/Time: ");
        for (int i = 0; i < 6; i++) {
          Serial.print(details.dateTime[i]);
          Serial.print(" ");
        }
        Serial.println();
        Serial.print("Message: "); Serial.println(cMsgData.message.c_str());
        Serial.println("-----------------------------------------");
      }
      else {
        Serial.println("Received unknown payload or checksum error.");
      }
      
      // ----- RSSI Reporting -----
      // Retrieve the RSSI value for this packet and classify it.
      int rssi = LoRa.packetRssi();
      String rssiStatus = getRSSIStatus(rssi);
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm - ");
      Serial.println(rssiStatus);
      Serial.println();
    }
    // Small delay to yield to other tasks.
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------
// Task 2: Serial Input Task
// Waits for user input from the Serial Monitor. When a number is entered,
// it is sent via a queue to the transmit task.
// --------------------------------------------------------
void SerialInputTask(void* pvParameters) {
  for (;;) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() > 0) {
        int msgID = input.toInt();
        // Send the msgID to the queue
        if (xQueueSend(msgQueue, &msgID, portMAX_DELAY) != pdPASS) {
          Serial.println("Failed to send msgID to queue");
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------
// Task 3: LoRa Transmit Task
// Waits on the queue for a msgID from the Serial Input Task. When received,
// it creates a predefined message payload and transmits it over LoRa.
// --------------------------------------------------------
void LoRaTransmitTask(void* pvParameters) {
  uint16_t transmissionID = 0; // Increment with each message sent
  int msgID;
  for (;;) {
    // Wait indefinitely until a msgID is received.
    if (xQueueReceive(msgQueue, &msgID, portMAX_DELAY) == pdPASS) {
      // Create the predefined message payload using your library.
      // Note: The payload builder expects a 0-based msgID, so subtract 1 from the user-entered msgID.
      std::vector<uint8_t> txPayload = payloadBuilder.create_p_msg_payload(transmissionID, (uint8_t)(msgID - 1));
      
      // Transmit the payload via LoRa.
      LoRa.beginPacket();
      for (uint8_t byte : txPayload) {
        LoRa.write(byte);
      }
      LoRa.endPacket();
      
      // Retrieve the corresponding emergency message for display.
      String msgText = getMessage(msgID);
      Serial.print("Transmitted predefined message with msgID: ");
      Serial.print(msgID);
      Serial.print(" - ");
      Serial.println(msgText);
      
      transmissionID++;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------
// Setup: Initialize Serial, LoRa, the PayloadBuilder, queue, tasks, and print predefined messages.
// --------------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for the Serial Monitor
  
  Serial.println("Base Station Starting...");
  
  // Print the predefined emergency messages for reference.
  Serial.println("Predefined Emergency Messages:");
  for (int i = 0; i < 10; i++) {
    Serial.print(i + 1); // Display messages numbered from 1 to 10.
    Serial.print(": ");
    Serial.println(emergencyMessages[i]);
  }
  Serial.println("Enter the number (1-10) of the message you want to transmit:");

  // Initialize LoRa module
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check connections.");
    while (1);
  }
  Serial.println("LoRa init succeeded.");

  // Configure your PayloadBuilder with device IDs.
  // For example: source = 0x02 (base station), destination = 0x01 (target device)
  payloadBuilder.configure_device(0x02, 0x01);

  // Create a queue for sending msgIDs (queue length = 10, size = int)
  msgQueue = xQueueCreate(10, sizeof(int));
  if (msgQueue == NULL) {
    Serial.println("Failed to create message queue.");
    while (1);
  }

  // Create the three tasks.
  // Task: LoRa Receive (runs on core 1)
  xTaskCreatePinnedToCore(
    LoRaReceiveTask,       // Task function
    "LoRaReceiveTask",     // Name of task
    4096,                  // Stack size in words
    NULL,                  // Task input parameter
    1,                     // Priority
    NULL,                  // Task handle
    1                      // Core where the task should run
  );

  // Task: Serial Input (runs on core 0)
  xTaskCreatePinnedToCore(
    SerialInputTask,
    "SerialInputTask",
    2048,
    NULL,
    1,
    NULL,
    0
  );

  // Task: LoRa Transmit (runs on core 0)
  xTaskCreatePinnedToCore(
    LoRaTransmitTask,
    "LoRaTransmitTask",
    4096,
    NULL,
    1,
    NULL,
    0
  );
}

// --------------------------------------------------------
// loop(): Empty since all operations are handled by tasks.
// --------------------------------------------------------
void loop() {
  // Nothing needed here.
}
