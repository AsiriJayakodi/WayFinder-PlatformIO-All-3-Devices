#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "payload_builder.h"
#include <vector>
#include <string>
#include <Wire.h>

// ----- Message Command Definitions -----
enum MessageType {
  PREDEFINED,
  CUSTOM
};

struct MessageCommand {
  MessageType type;
  int predefinedID;   // Valid if type == PREDEFINED
  String customText;  // Valid if type == CUSTOM
};

// ----- User Emergency Messages (sent from user to base) -----
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

// ----- Base Station Messages (sent from base to user) -----
const char* baseMessages[10] = {
  "All clear",
  "Evacuate immediately",
  "Proceed to checkpoint",
  "Remain calm",
  "Await further instructions",
  "Medical team is en route",
  "Rescue team dispatched",
  "Help is arriving",
  "Situation under control",
  "Mission accomplished"
};

// Helper function to return a user emergency message as an Arduino String.
// Expects a 1-based index, converts it to 0-based.
String getMessage(int index) {
  int adjustedIndex = index - 1;
  if (adjustedIndex >= 0 && adjustedIndex < 10) {
    return String(emergencyMessages[adjustedIndex]);
  } else {
    return "Invalid index! Please enter a number between 1 and 10.";
  }
}

// Helper function to return a base station message as an Arduino String.
// Expects a 1-based index, converts it to 0-based.
String getBaseMessage(int index) {
  int adjustedIndex = index - 1;
  if (adjustedIndex >= 0 && adjustedIndex < 10) {
    return String(baseMessages[adjustedIndex]);
  } else {
    return "Invalid index! Please enter a number between 1 and 10.";
  }
}

// ----- Helper Function for RSSI Status -----
String getRSSIStatus(int rssi) {
  if (rssi > -65) return "Excellent";
  else if (rssi > -75) return "Good";
  else if (rssi > -85) return "Fair";
  else return "Weak";
}

// ----- LoRa Module Pin Definitions -----
const int csPin    = 5;
const int resetPin = 14;
const int irqPin   = 2;

// Frequency for the SX1278
const long frequency = 433E6;

// ----- Global Instance of PayloadBuilder -----
PayloadBuilder payloadBuilder;

// ----- FreeRTOS Queue Handle -----
// Updated to hold MessageCommand structures.
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
      
      uint8_t type = payloadBuilder.identify_type_and_check_checksum(payload);

      if (type == 0x01) {  // GPS Payload
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
        int displayMsgID = pMsgData.msgID + 1;
        Serial.print("Message ID: "); Serial.println(displayMsgID);
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
      
      int rssi = LoRa.packetRssi();
      String rssiStatus = getRSSIStatus(rssi);
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm - ");
      Serial.println(rssiStatus);
      Serial.println();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------
// Task 2: Serial Input Task
// Updated to accept both predefined commands and custom messages.
// --------------------------------------------------------
void SerialInputTask(void* pvParameters) {
  for (;;) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() > 0) {
        MessageCommand cmd;
        // If input starts with "C:" treat it as a custom message.
        if (input.startsWith("C:") || input.startsWith("c:")) {
          cmd.type = CUSTOM;
          cmd.customText = input.substring(2);
          cmd.customText.trim();
        } else {
          // Otherwise, treat input as a predefined message number.
          cmd.type = PREDEFINED;
          cmd.predefinedID = input.toInt();
        }
        if (xQueueSend(msgQueue, &cmd, portMAX_DELAY) != pdPASS) {
          Serial.println("Failed to send message command to queue");
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------
// Task 3: LoRa Transmit Task
// Now handles both predefined and custom messages.
// --------------------------------------------------------
void LoRaTransmitTask(void* pvParameters) {
  uint16_t transmissionID = 0;
  MessageCommand cmd;
  for (;;) {
    if (xQueueReceive(msgQueue, &cmd, portMAX_DELAY) == pdPASS) {
      std::vector<uint8_t> txPayload;
      if (cmd.type == PREDEFINED) {
        txPayload = payloadBuilder.create_p_msg_payload(transmissionID, (uint8_t)(cmd.predefinedID - 1));
        String msgText = getBaseMessage(cmd.predefinedID);
        Serial.print("Transmitted base predefined message with msgID: ");
        Serial.print(cmd.predefinedID);
        Serial.print(" - ");
        Serial.println(msgText);
      } else if (cmd.type == CUSTOM) {
        txPayload = payloadBuilder.create_c_msg_payload(transmissionID, std::string(cmd.customText.c_str()));
        Serial.print("Transmitted custom message: ");
        Serial.println(cmd.customText);
      }
      
      LoRa.beginPacket();
      for (uint8_t byte : txPayload) {
        LoRa.write(byte);
      }
      LoRa.endPacket();
      
      transmissionID++;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// --------------------------------------------------------
// Setup: Print base station messages and initialize modules and tasks.
// --------------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  Serial.println("Base Station Starting...");
  
  Serial.println("Base Station Predefined Messages:");
  for (int i = 0; i < 10; i++) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(baseMessages[i]);
  }
  Serial.println("Enter a number (1-10) for a predefined base message or");
  Serial.println("enter a custom message with the prefix \"C:\" to transmit:");
  
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(frequency)) {
    Serial.println("LoRa init failed. Check connections.");
    while (1);
  }
  Serial.println("LoRa init succeeded.");
  
  payloadBuilder.configure_device(0x02, 0x01);
  
  msgQueue = xQueueCreate(10, sizeof(MessageCommand));
  if (msgQueue == NULL) {
    Serial.println("Failed to create message queue.");
    while (1);
  }
  
  xTaskCreatePinnedToCore(LoRaReceiveTask, "LoRaReceiveTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(SerialInputTask, "SerialInputTask", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(LoRaTransmitTask, "LoRaTransmitTask", 4096, NULL, 1, NULL, 0);
}

// --------------------------------------------------------
// loop(): Empty since all operations are handled by tasks.
// --------------------------------------------------------
void loop() {
}
