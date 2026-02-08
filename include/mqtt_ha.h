#ifndef MQTT_HA_H
#define MQTT_HA_H

#include <Arduino.h>

// Initialize WiFi and MQTT connections
void initNetwork();

// Call this in loop() to maintain MQTT connection
void networkLoop();

// Publish a snap event to Home Assistant
void publishSnapEvent();

// Check if MQTT is connected
bool isMqttConnected();

#endif // MQTT_HA_H
