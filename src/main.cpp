#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ShiftRegister74HC595.h>

// Network credentials Here
const char* ssid     = "Vodafone-A44386461";
const char* password = "Dory63maya64rossi";

// Web server on port 80
WebServer server(80);

// Pins for shift register
#define DATA_PIN 4   // DS
#define CLOCK_PIN 5  // SHCP
#define LATCH_PIN 18 // STCP

// Number of shift registers (6 for 6 displays)
const int numRegisters = 6;

// Initialize shift register library
ShiftRegister74HC595<6> sr(DATA_PIN, CLOCK_PIN, LATCH_PIN);

// Player scores
int scores[2] = {301, 301};

// Segment mapping for numbers 0-9
const uint8_t segmentMap[] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

void updateDisplays() {
  // Update Player 1 displays
  int score1 = scores[0];
  sr.set(0, segmentMap[score1 / 100]);          // Hundreds
  sr.set(1, segmentMap[(score1 / 10) % 10]);   // Tens
  sr.set(2, segmentMap[score1 % 10]);          // Units

  // Update Player 2 displays
  int score2 = scores[1];
  sr.set(3, segmentMap[score2 / 100]);         // Hundreds
  sr.set(4, segmentMap[(score2 / 10) % 10]);  // Tens
  sr.set(5, segmentMap[score2 % 10]);         // Units

  // Push data to the shift registers
  sr.updateRegisters();
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Serve the HTML file
  server.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      server.send(500, "text/plain", "Failed to open file");
      return;
    }
    server.streamFile(file, "text/html");
    file.close();
  });

  // Handle score updates
  server.on("/updateScore", HTTP_POST, []() {
    if (server.hasArg("player") && server.hasArg("score")) {
      int player = server.arg("player").toInt();
      int score = server.arg("score").toInt();
      if (player >= 0 && player < 2) {
        scores[player] -= score;
        if (scores[player] < 0) scores[player] = 0;
        updateDisplays();
        server.send(200, "text/plain", "Score updated");
      } else {
        server.send(400, "text/plain", "Invalid player");
      }
    } else {
      server.send(400, "text/plain", "Missing arguments");
    }
  });

  // Start the server
  server.begin();

  // Initialize displays with starting scores
  updateDisplays();
}

void loop() {
  server.handleClient();
}