#ifndef _MY_WIFI_CNF_H

#define _MY_WIFI_CNF_H

#define DEFAULT_MQTT_TOPIC "/trelliswifi/"
#define DEFAULT_MQTT_PORT  "1883"

// Define this in order to force clear setting upon call
// to wifiConfig_init(). The value should be a valid GPIO
// number, which will be set at input. If it reads as HIGH
// during wifiConfig_init(), then all nvs will get erased.
// #define NV_CLEAR_BUTTON_CHECK 32

typedef struct WifiConfigData_t {
  String wifiSsid;
  String wifiPass;
  String mqttServer;
  uint16_t mqttPort;
  String mqttUsername;
  String mqttPassword;
  String mqttTopic;
} WifiConfigData;

void wifiConfig_init(bool forceNvClear);
const struct WifiConfigData_t& wifiConfig_get();

#endif  // define _MY_WIFI_CNF_H
