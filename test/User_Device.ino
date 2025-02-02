#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>

// Pin definitions
#define CS_PIN 5
#define RST_PIN 14
#define IRQ_PIN 2

#define GPS_TX_PIN 16
#define GPS_RX_PIN 17

TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // UART1 for GPS
unsigned long previousMillis = 0;
const unsigned long interval = 5000;
float lat = 0, lng = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // GPS Serial port
  gpsSerial.begin(9600, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN);

  // LoRa initialization
  LoRa.setPins(CS_PIN, RST_PIN, IRQ_PIN);
  if (!LoRa.begin(450E6)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);

  Serial.println("LoRa Initialized!");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    get_gps_data();
  }
}

void get_gps_data() {
  Serial.println("Getting GPS data...");
  unsigned long start = millis();
  while (millis() - start < 1000) {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
  }

  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lng = gps.location.lng();
    Serial.print("Latitude: ");
    Serial.println(lat, 6);
    Serial.print("Longitude: ");
    Serial.println(lng, 6);

    String message = String(lat, 4) + "," + String(lng, 4);
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.print("user 1:");
    LoRa.print(lat);
    LoRa.print(lng);
    LoRa.endPacket();
    Serial.print("Sent: ");
    Serial.println(message);
  } else {
    Serial.println(F("Invalid GPS data. No transmission."));
  }
}
