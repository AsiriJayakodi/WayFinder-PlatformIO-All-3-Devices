#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include "MyIoT.h"
#include "payload_builder.h"
#include <U8g2lib.h>
// #include <Arduino.h>
// #include <U8g2lib.h>

// Fonts
#define H_FONT u8g2_font_ncenB08_tr
#define P_FONT u8g2_font_6x10_tr

// Button Pins
#define OK_BTN 33
#define MODE_BTN 32
#define UP_BTN   25
#define DOWN_BTN 27

#define LORA_SS    5   // LoRa SPI CS pin
#define LORA_RST   14   // LoRa reset pin
#define LORA_DIO0  26   // LoRa IRQ pin

// U8G2 for I2C Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// 3x3 Array Initialization
uint8_t states[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
uint8_t selectedRow = 0;  // Tracks which row is active (0, 1, or 2)

// Debounce Handling
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;  // Debounce time

// FreeRTOS handles
SemaphoreHandle_t xSemaphore;
QueueHandle_t loraQueue;  // Queue to handle LoRa message requests

void ButtonTask(void *pvParameters);
void DisplayTask(void *pvParameters);
void LoRaTask(void *pvParameters);


// UI Render Functions
void renderUI();
void renderInbox();
void renderSend();
void renderWelcome();

void setup() {
  Serial.begin(9600);
  u8g2.begin();
  u8g2.clearBuffer();

  // Initialize Buttons with Pull-down Resistors
  pinMode(MODE_BTN, INPUT_PULLDOWN);
  pinMode(UP_BTN, INPUT_PULLDOWN);
  pinMode(DOWN_BTN, INPUT_PULLDOWN);
  pinMode(OK_BTN, INPUT_PULLDOWN);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {  // Set frequency to 433 MHz
    Serial.println("LoRa init failed. Check connections.");
    while (1);
  }
  Serial.println("LoRa init succeeded.");

  // Create a semaphore for shared access between tasks
  xSemaphore = xSemaphoreCreateMutex();
  loraQueue = xQueueCreate(5, sizeof(int));  // Queue can hold 5 integers (message IDs)

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(ButtonTask, "Button Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(DisplayTask, "Display Task", 4096, NULL, 1, NULL, 1);
  xTaskCreate(LoRaTask, "LoRaTask", 2048, NULL, 1, NULL);
}

void loop() {
  // Not needed as FreeRTOS manages tasks
}

void ButtonTask(void *pvParameters) {
  while (1) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (digitalRead(MODE_BTN) == HIGH) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
          selectedRow = (selectedRow + 1) % 3;
          for (uint8_t i = 0; i < 3; i++) {
            states[i][0] = (i == selectedRow) ? 1 : 0;
          }
          xSemaphoreGive(xSemaphore);
        }
        lastDebounceTime = millis();
      }

      if (digitalRead(UP_BTN) == HIGH) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
          if (states[selectedRow][1] < 256) {
            states[selectedRow][1]++;
          }
          xSemaphoreGive(xSemaphore);
        }
        lastDebounceTime = millis();
      }

      if (digitalRead(DOWN_BTN) == HIGH) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
          if (states[selectedRow][1] > 0) {
            states[selectedRow][1]--;
          }
          xSemaphoreGive(xSemaphore);
        }
        lastDebounceTime = millis();
      }

      if (digitalRead(OK_BTN) == HIGH) {
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
          states[selectedRow][2] = !states[selectedRow][2];
          if (selectedRow == 2 && states[selectedRow][2] == 1) {
            int messageID = states[selectedRow][1];
            xQueueSend(loraQueue, &messageID, portMAX_DELAY);
            states[selectedRow][2] = 0;  // Send message to LoRa task
          }
          xSemaphoreGive(xSemaphore);
        }
        lastDebounceTime = millis();
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void DisplayTask(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY)) {
      renderUI();
      xSemaphoreGive(xSemaphore);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Refresh rate
  }
}

void renderUI() {
  if (selectedRow == 0) {
    renderWelcome();
  } else if (selectedRow == 1) {
    renderInbox();
  } else if (selectedRow == 2) {
    renderSend();
  }
}

void renderInbox() {
  u8g2.clearBuffer();
  u8g2.setFont(H_FONT);
  u8g2.drawStr(0, 10, "INBOX");
  u8g2.drawLine(0, 11, 127, 11);

  char buffer[10];
  itoa(states[selectedRow][1], buffer, 10);
  u8g2.setFont(P_FONT);
  u8g2.drawStr(0, 21, "SELECTED MSG:");
  u8g2.drawStr(0, 31, buffer);

  if (states[selectedRow][2] == 1) {
    u8g2.drawStr(0, 41, "OK Pressed...");
  }

  u8g2.sendBuffer();
}

void renderSend() {
  u8g2.clearBuffer();
  u8g2.setFont(H_FONT);
  u8g2.drawStr(0, 10, "SEND");
  u8g2.drawLine(0, 11, 127, 11);

  char buffer[10];
  itoa(states[selectedRow][1], buffer, 10);
  u8g2.setFont(P_FONT);
  u8g2.drawStr(0, 21, "SELECTED P_MSG:");
  u8g2.drawStr(0, 31, buffer);

  if (states[selectedRow][2] == 1) {
    u8g2.drawStr(0, 41, "OK Pressed...");
  }

  u8g2.sendBuffer();
}

void renderWelcome() {
  u8g2.clearBuffer();
  u8g2.setFont(H_FONT);
  u8g2.drawStr(0, 10, "WELCOME");
  u8g2.drawLine(0, 11, 127, 11);
  u8g2.sendBuffer();
}

void LoRaTask(void *pvParameters) {
  int receivedMessageID;
  
  for (;;) {
    if (xQueueReceive(loraQueue, &receivedMessageID, portMAX_DELAY)) {
      // Send the message via LoRa
      Serial.print("Sending LoRa Message with ID: ");
      Serial.println(receivedMessageID);
      
      LoRa.beginPacket();
      LoRa.print("Message ID: ");
      LoRa.print(receivedMessageID);
      LoRa.endPacket();
      
      Serial.println("Message sent!");
    }
  }
}
