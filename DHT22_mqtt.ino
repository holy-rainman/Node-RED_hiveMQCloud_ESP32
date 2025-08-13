//================================================ WiFi + MQTT
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ======== WiFi (Wokwi) ========
const char* ssid     = "Wokwi-GUEST";
const char* password = "";

// ======== HiveMQ Cloud (TLS) ========
const char* mqtt_server = "YOUR_hiveMQ_URL.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;          // TLS port
const char* mqtt_user   = "YOUR_CLUSTER_NAME";
const char* mqtt_pass   = "YOUR_CLUSTER_PW";

// ======== Topics ========
const char* sub_topic   = "rumah/LED1";
const char* topic_temp  = "rumah/DHT22/temp";
const char* topic_humid = "rumah/DHT22/humid";

// ======== Hardware ========
#define LEDpin1 2  // GPIO2 (internal LED on some boards)

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ---------- WiFi ----------
void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  espClient.setInsecure(); // For Wokwi / testing
}

// ---------- MQTT callback ----------
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Message ["); Serial.print(topic); Serial.print("] ");
  Serial.println(msg);

  if (msg == "ON")  digitalWrite(LEDpin1, HIGH);
  if (msg == "OFF") digitalWrite(LEDpin1, LOW);
}

// ---------- MQTT reconnect ----------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESPClient-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(sub_topic);
      Serial.print("Subscribed to: "); Serial.println(sub_topic);
    } else {
      Serial.print("failed, rc="); Serial.print(client.state());
      Serial.println("  retry in 5s");
      delay(5000);
    }
  }
}

//================================================ DHT22
#include "DHTesp.h"

DHTesp dht;
int dhtPin = 27;

// ===== Timing =====
unsigned long lastReadTime = 0;
const unsigned long interval = 2000; // 2s

//================================================ Setup & Loop
void setup() {
  pinMode(LEDpin1, OUTPUT);
  digitalWrite(LEDpin1, LOW);

  Serial.begin(9600);
  Serial.println();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  dht.setup(dhtPin, DHTesp::DHT22);
  Serial.println("DHT22 initiated");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();
  if (now - lastReadTime > interval) {
    lastReadTime = now;

    TempAndHumidity newValues = dht.getTempAndHumidity();
    if (dht.getStatus() != 0) {
      Serial.println("DHT22 error status: " + String(dht.getStatusString()));
      return; // Skip this cycle
    }

    Serial.println(" T:" + String(newValues.temperature, 1) + 
                   "°C  H:" + String(newValues.humidity, 1) + "%");

    // Convert to char for MQTT
    char tempStr[8], humidStr[8];
    dtostrf(newValues.temperature, 1, 2, tempStr);
    dtostrf(newValues.humidity, 1, 2, humidStr);

    client.publish(topic_temp, tempStr);
    client.publish(topic_humid, humidStr);

    Serial.print("Published -> Temp: "); Serial.print(tempStr);
    Serial.print(" °C | Humid: "); Serial.print(humidStr); Serial.println(" %");
  }
}
