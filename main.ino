/*
 * ESP32-S3 Ultrasonic Object Detection with Webhook (Optimized)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> // HTTPS ke liye zaroori library

// ==================== CONFIGURATION ====================
const char* WIFI_SSID     = "G-745";
const char* WIFI_PASSWORD = "12345678";

const char* WEBHOOK_URL   = "https://hook.eu1.make.com/04vbkyqpr711svnjhf79a2kulbs8lcnm";

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

const float DISTANCE_THRESHOLD_CM = 50.0;
const unsigned long COOLDOWN_MS   = 30000;

// Timing
const unsigned long MEASURE_INTERVAL_MS = 500;
const unsigned long WIFI_CHECK_INTERVAL_MS = 5000;
// =======================================================

unsigned long lastRequestTime = 0;
unsigned long lastMeasureTime = 0;
unsigned long lastWiFiCheckTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi: ");
  
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 30) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi Connected!");
  } else {
    Serial.println("\n[ERROR] WiFi connection failed.");
  }
}

void loop() {
  unsigned long currentTime = millis();

  // WiFi Check
  if (currentTime - lastWiFiCheckTime >= WIFI_CHECK_INTERVAL_MS) {
    lastWiFiCheckTime = currentTime;
    if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  }

  // Sensor Measure
  if (currentTime - lastMeasureTime >= MEASURE_INTERVAL_MS) {
    lastMeasureTime = currentTime;
    
    long duration;
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH, 30000);
    
    float distance = duration * 0.01715;

    if (distance > 2.0 && distance < 400.0) {
      Serial.print("Distance: ");
      Serial.println(distance);

      if (distance < DISTANCE_THRESHOLD_CM) {
        if (currentTime - lastRequestTime >= COOLDOWN_MS || lastRequestTime == 0) {
          sendWebhook();
        }
      }
    }
  }
}

// ==================== UPDATED WEBHOOK FUNCTION ====================
void sendWebhook() {
  Serial.print("[HTTP] Sending secure request... ");
  
  WiFiClientSecure client;
  client.setInsecure(); // SSL certificate skip karne ke liye zaroori line
  
  HTTPClient http;
  
  if (http.begin(client, WEBHOOK_URL)) { // Client pass kiya
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Request Sent! Code: " + String(httpResponseCode));
    } else {
      Serial.println("Failed! Error: " + http.errorToString(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("Unable to connect to server");
  }

  lastRequestTime = millis();
  Serial.println("[INFO] 30-second cooldown started.");
}
