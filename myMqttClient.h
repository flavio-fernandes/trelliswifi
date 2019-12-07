#ifndef _MY_MQTT_CLIENT_H

#define _MY_MQTT_CLIENT_H

// Board: ESP32

#include <Esp.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

static const unsigned int defaultMqttReconnect = 30;

typedef struct {
  bool lastMqttConnected;
  unsigned int reconnectTicks;  // do not mqtt connect while this is > 0
} MqttState;

extern MqttState mqttState;



#endif  // define _MY_MQTT_CLIENT_H
