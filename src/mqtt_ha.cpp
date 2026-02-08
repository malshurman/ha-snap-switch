#include "mqtt_ha.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"

// MQTT Topics
#define MQTT_STATE_TOPIC      "homeassistant/binary_sensor/" DEVICE_NAME "/state"
#define MQTT_DISCOVERY_TOPIC  "homeassistant/binary_sensor/" DEVICE_NAME "/config"
#define MQTT_AVAIL_TOPIC      "homeassistant/binary_sensor/" DEVICE_NAME "/availability"

// Objects
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

// State
static unsigned long lastMqttReconnectAttempt = 0;
static unsigned long snapEventEndTime = 0;
static bool snapEventActive = false;

static void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection FAILED! Continuing without network.");
  }
}

static void sendHADiscovery() {
  String discoveryPayload = "{";
  discoveryPayload += "\"name\":\"Snap Sensor\",";
  discoveryPayload += "\"unique_id\":\"" + String(DEVICE_NAME) + "_snap\",";
  discoveryPayload += "\"state_topic\":\"" + String(MQTT_STATE_TOPIC) + "\",";
  discoveryPayload += "\"availability_topic\":\"" + String(MQTT_AVAIL_TOPIC) + "\",";
  discoveryPayload += "\"payload_on\":\"ON\",";
  discoveryPayload += "\"payload_off\":\"OFF\",";
  discoveryPayload += "\"device_class\":\"sound\",";
  discoveryPayload += "\"device\":{";
  discoveryPayload += "\"identifiers\":[\"" + String(DEVICE_NAME) + "\"],";
  discoveryPayload += "\"name\":\"Snap Detector\",";
  discoveryPayload += "\"model\":\"ESP32 INMP441\",";
  discoveryPayload += "\"manufacturer\":\"DIY\"";
  discoveryPayload += "}}";
  
  mqttClient.publish(MQTT_DISCOVERY_TOPIC, discoveryPayload.c_str(), true);
  Serial.println("Sent HA Discovery config");
}

static bool connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) return false;
  
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(MQTT_SERVER);
  
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setBufferSize(512);  // Discovery payload is ~350 bytes, default 256 is too small
  
  String clientId = String(DEVICE_NAME) + "_" + String(random(0xffff), HEX);
  bool connected = false;
  
  if (strlen(MQTT_USER) > 0) {
    connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD,
                                    MQTT_AVAIL_TOPIC, 0, true, "offline");
  } else {
    connected = mqttClient.connect(clientId.c_str(), MQTT_AVAIL_TOPIC, 0, true, "offline");
  }
  
  if (connected) {
    Serial.println("MQTT connected!");
    mqttClient.publish(MQTT_AVAIL_TOPIC, "online", true);
    mqttClient.publish(MQTT_STATE_TOPIC, "OFF", true);
    sendHADiscovery();
    return true;
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

void initNetwork() {
  connectWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT();
  }
}

void networkLoop() {
  // Reconnect WiFi if needed
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      unsigned long now = millis();
      if (now - lastMqttReconnectAttempt > 5000) {
        lastMqttReconnectAttempt = now;
        connectMQTT();
      }
    } else {
      mqttClient.loop();
    }
  }
  
  // Handle snap event OFF timing
  if (snapEventActive && millis() > snapEventEndTime) {
    if (mqttClient.connected()) {
      mqttClient.publish(MQTT_STATE_TOPIC, "OFF");
      Serial.println("[MQTT] Published snap event: OFF");
    }
    snapEventActive = false;
  }
}

void publishSnapEvent() {
  if (!mqttClient.connected()) return;
  
  mqttClient.publish(MQTT_STATE_TOPIC, "ON");
  Serial.println("[MQTT] Published snap event: ON");
  
  snapEventActive = true;
  snapEventEndTime = millis() + 500;
}

bool isMqttConnected() {
  return mqttClient.connected();
}
