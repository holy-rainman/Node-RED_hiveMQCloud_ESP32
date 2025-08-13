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
const char* sub_topic = "rahim/LED1";   // expects "led1_1" / "led1_0"

// ======== Hardware ========
#define LEDpin1 2  // current-sourcing LED on GPIO19

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ---------- WiFi ----------
void setup_wifi() 
{ Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  { delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  // For demo/testing (Wokwi): skip certificate validation
  espClient.setInsecure();
}

// ---------- MQTT callback ----------
void callback(char* topic, byte* payload, unsigned int length) 
{ // build payload string safely
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("Message ["); Serial.print(topic); Serial.print("] ");
  Serial.println(msg);

  // Control LED by payload
  if (msg == "ON")  digitalWrite(LEDpin1, HIGH);
  if (msg == "OFF") digitalWrite(LEDpin1, LOW);
}

// ---------- MQTT reconnect ----------
void reconnect() 
{ while (!client.connected()) 
  { Serial.print("Attempting MQTT connection...");
    // Give this client a unique ID
    String clientId = "ESPClient-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    // Connect with username/password
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) 
    { Serial.println("connected");
      client.subscribe(sub_topic);
      Serial.print("Subscribed to: "); Serial.println(sub_topic);
    } 
    else 
    { Serial.print("failed, rc="); Serial.print(client.state());
      Serial.println("  retry in 5s");
      delay(5000);
    }
  }
}

void setup() 
{ Serial.begin(9600);
  pinMode(LEDpin1, OUTPUT);
  digitalWrite(LEDpin1, LOW);  // LED off initially

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() 
{ if (!client.connected()) reconnect();
  client.loop();
}
