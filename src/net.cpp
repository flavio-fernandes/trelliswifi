#include "common.h"
#include "net.h"
#include "wifiConfig.h"
#include "netConfig.h"
#include "animations.h"
#include "colors.h"
#include "tickerScheduler.h"

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

// huzzah ref: https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/

#define MQTT_SUB_PING "ping"
#define MQTT_XUB_CMD "cmd" // xub: sub and pub

#define MQTT_PUB_BUTTONS "buttons"
#define MQTT_PUB_OPER_STATE_BATTERY "battery"
#define MQTT_PUB_OPER_STATE_UPTIME "uptime"
#define MQTT_PUB_OPER_STATE_MEMORY "memory"
#define MQTT_PUB_OPER_STATE_ETC "etc"

// FWDs
bool checkWifiConnected();
bool checkMqttConnected();
static void mqtt1SecTick();
static void mqtt1MinTick();
static void mqtt10MinTick();

// NOTE: sizeOfMsgBuff needs to fit inside
// Adafruit_MQTT.h MAXBUFFERSIZE and that is
// not including extra space needed for header
// and topic. See:
// https://github.com/adafruit/Adafruit_MQTT_Library/pull/166
static const size_t sizeOfMsgBuff = 256;
static char msgBuff[sizeOfMsgBuff] = {0};
static StaticJsonDocument<sizeOfMsgBuff> msgDoc;

MqttState mqttState;

// Create an WiFiClient class to connect to the MQTT server.
WiFiClient client;

typedef struct MqttConfig_t
{
    Adafruit_MQTT_Subscribe *service_sub_ping;
    Adafruit_MQTT_Subscribe *service_sub_cmd;
    Adafruit_MQTT_Publish *service_pub_cmd;

    Adafruit_MQTT_Publish *service_pub_buttons;
    Adafruit_MQTT_Publish *service_pub_oper_state_battery;
    Adafruit_MQTT_Publish *service_pub_oper_state_uptime;
    Adafruit_MQTT_Publish *service_pub_oper_state_memory;
    Adafruit_MQTT_Publish *service_pub_oper_state_etc;

    Adafruit_MQTT_Client *mqttPtr;
    const char *topicPing;
    const char *topicCmd;
    const char *topicButtons;
    const char *topicOperStateBattery;
    const char *topicOperStateUptime;
    const char *topicOperStateMemory;
    const char *topicOperStateEtc;
} MqttConfig;

static struct MqttConfig_t mqttConfig = {0};

static void initOTA()
{
#ifdef OTA_PORT
    // Port defaults to 8266
    ArduinoOTA.setPort(OTA_PORT);
#endif // #ifdef OTA_PORT

#ifdef OTA_HOSTNAME
    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(OTA_HOSTNAME);
#endif // #ifdef OTA_HOSTNAME

#ifdef OTA_PASS
    // No authentication by default
    ArduinoOTA.setPassword(OTA_PASS);
#elifdef OTA_MD5
    // Password can be set with it's md5 value as well
    ArduinoOTA.setPasswordHash(OTA_MD5);
#endif // #ifdef OTA_PASS

#ifdef DEBUG
    ArduinoOTA.onStart([]()
                       { Serial.println("Start updating"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("Ended updating"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
        {
            Serial.println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            Serial.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            Serial.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            Serial.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            Serial.println("End Failed");
        } });
#endif
}

void initMyMqtt(TickerScheduler &ts)
{
    memset(&mqttState, 0, sizeof(mqttState));
    mqttState.lastMqttConnected = false;

    // Used as sanity: this value better never change
    state.msgBufferSize = sizeOfMsgBuff;

    wifiConfig_init(false /*forceNvClear*/);
    const WifiConfigData &cnf = wifiConfig_get();

#ifdef DEBUG
    Serial.printf("\nConnecting to %s\n", cnf.wifiSsid.c_str());
#endif

    // Set WiFi to station mode
    WiFi.mode(WIFI_STA);
    WiFi.begin(cnf.wifiSsid.c_str(), cnf.wifiPass.c_str());
    initOTA();

    mqttConfig.mqttPtr = new Adafruit_MQTT_Client(&client, cnf.mqttServer.c_str(), cnf.mqttPort,
                                                  cnf.mqttUsername.c_str(), cnf.mqttPassword.c_str());
    String tmp;

    tmp = cnf.mqttTopic + MQTT_SUB_PING;
    mqttConfig.topicPing = strdup(tmp.c_str());
    mqttConfig.service_sub_ping = new Adafruit_MQTT_Subscribe(mqttConfig.mqttPtr, mqttConfig.topicPing);

    tmp = cnf.mqttTopic + MQTT_XUB_CMD;
    mqttConfig.topicCmd = strdup(tmp.c_str());
    mqttConfig.service_sub_cmd = new Adafruit_MQTT_Subscribe(mqttConfig.mqttPtr, mqttConfig.topicCmd);
    mqttConfig.service_pub_cmd = new Adafruit_MQTT_Publish(mqttConfig.mqttPtr, mqttConfig.topicCmd);

    tmp = cnf.mqttTopic + MQTT_PUB_BUTTONS;
    mqttConfig.topicButtons = strdup(tmp.c_str());
    mqttConfig.service_pub_buttons = new Adafruit_MQTT_Publish(mqttConfig.mqttPtr, mqttConfig.topicButtons);

    tmp = cnf.mqttTopic + MQTT_PUB_OPER_STATE_BATTERY;
    mqttConfig.topicOperStateBattery = strdup(tmp.c_str());
    mqttConfig.service_pub_oper_state_battery = new Adafruit_MQTT_Publish(mqttConfig.mqttPtr, mqttConfig.topicOperStateBattery);

    tmp = cnf.mqttTopic + MQTT_PUB_OPER_STATE_UPTIME;
    mqttConfig.topicOperStateUptime = strdup(tmp.c_str());
    mqttConfig.service_pub_oper_state_uptime = new Adafruit_MQTT_Publish(mqttConfig.mqttPtr, mqttConfig.topicOperStateUptime);

    tmp = cnf.mqttTopic + MQTT_PUB_OPER_STATE_MEMORY;
    mqttConfig.topicOperStateMemory = strdup(tmp.c_str());
    mqttConfig.service_pub_oper_state_memory = new Adafruit_MQTT_Publish(mqttConfig.mqttPtr, mqttConfig.topicOperStateMemory);

    tmp = cnf.mqttTopic + MQTT_PUB_OPER_STATE_ETC;
    mqttConfig.topicOperStateEtc = strdup(tmp.c_str());
    mqttConfig.service_pub_oper_state_etc = new Adafruit_MQTT_Publish(mqttConfig.mqttPtr, mqttConfig.topicOperStateEtc);

    // Init tickers
    const uint32_t oneSec = 1000;
    const uint32_t oneMin = oneSec * 60;
    const uint32_t tenMin = oneMin * 10;

    ts.sched(mqtt1SecTick, oneSec);
    ts.sched(mqtt1MinTick, oneMin);
    ts.sched(mqtt10MinTick, tenMin);
}

void myMqttLoop()
{
    yield(); // make esp happy

    if (!state.initIsDone)
        return;
    if (!checkWifiConnected())
        return;
    ArduinoOTA.handle();
    if (!checkMqttConnected())
        return;

    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;

    // Listen for updates on any subscribed MQTT feeds and process them all.
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription()))
    {
        const char *message = 0;

        if (subscription == mqttConfig.service_sub_ping)
        {
            message = (const char *)(mqttConfig.service_sub_ping)->lastread;
            if (strncmp(message, "periodic", 8) == 0)
            {
                // treat periodic pings as a sign that no response is needed
                // feed the reboot dog watch, so it will not take effect for a bit longer
#ifdef DEBUG
                Serial.printf("got ping: %s (feeding the watch dog)\n", message);
#endif
                state.minutes_since_periodic_ping = 0;
            }
            else
            {
#ifdef DEBUG
                Serial.printf("got ping: %s\n", message);
#endif
                // send operstate, just as a sign of pong
                sendOperState();
            }
        }
        else if (subscription == mqttConfig.service_sub_cmd)
        {
            message = (const char *)(mqttConfig.service_sub_cmd)->lastread;

            // if strlen of message is 0, that means we caused it due to publish below... silently ignore it
            if (strlen(message) == 0)
                continue;
            parseMqttCmd(message, MAXBUFFERSIZE);

            // explicitly clear mqtt topic
            /*const*/ uint8_t foo_payload = ~0;
            if (!mqttConfig.service_pub_cmd->publish(&foo_payload, 0 /*bLen*/))
            {
#ifdef DEBUG
                Serial.println("Unable to publish reset of service_sub_cmd");
#endif
                mqtt.disconnect();
            }
        }
        else
        {
#ifdef DEBUG
            Serial.printf("got unexpected msg on subscription: %s\n", subscription->topic);
#endif
        }
    }
}

bool checkWifiConnected()
{
    static bool otaBegun = false;
    static bool lastWifiConnected = false;

    const bool currConnected = WiFi.status() == WL_CONNECTED;
    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;

    if (lastWifiConnected != currConnected)
    {

        if (currConnected)
        {
#ifdef DEBUG
            Serial.println("WiFi connected");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
#endif
            startAnimationFlashlight(15 /*expiration 1.5 secs*/, colorYellow,
                                     true /*pulse*/, false /*blink*/, false /*doneCallback*/);

            // idem potent. If it fails, this is a game stopper...
            if (!mqtt.subscribe(mqttConfig.service_sub_ping) ||
                !mqtt.subscribe(mqttConfig.service_sub_cmd))
            {
                gameOver("Fatal: unable to subscribe to mqtt");
            }

            if (!otaBegun)
            {
                otaBegun = true;
                ArduinoOTA.begin();
#ifdef DEBUG
                Serial.printf("Ready to perform OTA update via port %u\n", OTA_PORT);
#endif
            }
        }
        else
        {
#ifdef DEBUG
            Serial.println("WiFi disconnected");
#endif
            // assume mqtt is not connected
            mqttState.lastMqttConnected = false;
        }

        lastWifiConnected = currConnected;
    }

    return currConnected;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
bool checkMqttConnected()
{
    // not attempt to reconnect if there are reconnect ticks outstanding
    if (mqttState.reconnectTicks > 0)
        return false;

    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;
    const bool currMqttConnected = mqtt.connected();

    // noop?
    if (mqttState.lastMqttConnected && currMqttConnected)
        return true;

    if (currMqttConnected)
    {
#ifdef DEBUG
        Serial.println("MQTT Connected!");
#endif
        // startAnimationFlashlight(1 /*expiration 0.1 secs*/, colorGreen,
        //     true /*pulse*/, false /*blink*/, false /*doneCallback*/);
        state.mqttUpInMinutes = 0;
        sendOperState();
    }
    else
    {
#ifdef DEBUG
        Serial.print("MQTT is connecting... ");
#endif

        // Note: The connect call can block for up to 6 seconds.
        //       when mqtt is out... be aware.
        const int8_t ret = mqtt.connect();
        if (ret != 0)
        {
#ifdef DEBUG
            Serial.println(mqtt.connectErrorString(ret));
#endif
            mqtt.disconnect();

            // do not attempt to connect before a few ticks
            mqttState.reconnectTicks = defaultMqttReconnect;
        }
        else
        {
#ifdef DEBUG
            Serial.println("done.");
#endif
        }
    }

    mqttState.lastMqttConnected = currMqttConnected;
    return currMqttConnected;
}

static void mqtt1SecTick()
{
    if (mqttState.reconnectTicks > 0)
        --mqttState.reconnectTicks;

#ifdef DEBUG
    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;
    if (!mqtt.connected())
    {
        Serial.print("mqtt1SecTick");
        Serial.print(" reconnectTics: ");
        Serial.print(mqttState.reconnectTicks, DEC);
        Serial.print(" mqtt_connected: ");
        Serial.print(mqttState.lastMqttConnected ? "yes" : "no");
        Serial.println("");
    }
#endif
}

static void mqtt1MinTick()
{
    // If it has been too long since we got a periodic ping, it is time for a game over.
    // Note that periodic pings are expected to take place as slow as 10 minutes, so we
    // shall be generous here.
    const WifiConfigData &cnf = wifiConfig_get();
    if (cnf.needPeriodicPings && state.minutes_since_periodic_ping > 21)
    {
        gameOver("Too long without a periodic ping event");
        return;
    }

    state.minutes_since_periodic_ping += 1;
    state.uptimeInMinutes += 1;

    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;
    if (mqtt.connected())
        state.mqttUpInMinutes += 1;
}

static void mqtt10MinTick()
{
#ifdef DEBUG
    Serial.println("mqtt10MinTick -- sending gratuitous state");
#endif

    // grauitous
    if (!sendOperState())
        gameOver("failed to sendOperState on mqtt10MinTick");
}

static void bumpMsgCounters(size_t lastMsgSize = 0)
{
    // sanity: make sure the msgBufferSize is untouched
    if (state.msgBufferSize != sizeOfMsgBuff)
    {
        snprintf(msgBuff, sizeOfMsgBuff,
                 "msgBufferSize corrupted: %zu is not expected %zu",
                 state.msgBufferSize, sizeOfMsgBuff);
        gameOver(msgBuff);
        return;
    }

    if (state.largestMsgSize < lastMsgSize)
        state.largestMsgSize = lastMsgSize;
    ++state.numberOfMsgsSent;
}

static inline void buffToDoc(const char *const valueName)
{
    msgDoc[valueName] = String(msgBuff);
}

static bool sendCommon(const char *const eventName, Adafruit_MQTT_Publish* pubPtr)
{
    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;
    const size_t memoryUsage = msgDoc.memoryUsage();
#ifdef DEBUG
    serializeJson(msgDoc, Serial);
    Serial.printf(" Memory used: %zu\n", memoryUsage);
#endif
    serializeJson(msgDoc, msgBuff, sizeOfMsgBuff);
    if (! (*pubPtr).publish(msgBuff))
    {
#ifdef DEBUG
        Serial.printf("Unable to publish %s\n", eventName);
#endif
        mqtt.disconnect();
        return false;
    }
    bumpMsgCounters(memoryUsage);
    return true;
}

bool sendOperState()
{
    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;
    if (!mqtt.connected())
        return false;

    // battery
    float batteryVoltage;
    const bool batteryLow = isBatteryLow(&batteryVoltage);
    msgDoc.clear();
    snprintf(msgBuff, sizeOfMsgBuff, "%.2f", batteryVoltage);
    buffToDoc("volts");
    snprintf(msgBuff, sizeOfMsgBuff, "%s", batteryLow ? "yes" : "no");
    buffToDoc("isLow");
    if (!sendCommon(MQTT_PUB_OPER_STATE_BATTERY, mqttConfig.service_pub_oper_state_battery))
        return false;

    // uptime
    msgDoc.clear();
    // minutes in a year: 525600. Let's wrap counter to 7 digits to make sure we have enough buffer space
    static const uint32_t minutesWrap = 9999666UL;
    snprintf(msgBuff, sizeOfMsgBuff, "%" PRIu32, state.uptimeInMinutes % minutesWrap);
    buffToDoc("up");
    snprintf(msgBuff, sizeOfMsgBuff, "%" PRIu32, state.mqttUpInMinutes % minutesWrap);
    buffToDoc("mqttUp");
    snprintf(msgBuff, sizeOfMsgBuff, "%" PRIu32, state.minutes_since_periodic_ping % minutesWrap);
    buffToDoc("dog");
    if (!sendCommon(MQTT_PUB_OPER_STATE_UPTIME, mqttConfig.service_pub_oper_state_uptime))
        return false;

    // memory
    msgDoc.clear();
    snprintf(msgBuff, sizeOfMsgBuff, "%zu", state.largestMsgSize);
    buffToDoc("maxMsg");
    // ~/arduinoPackages/esp32/hardware/esp32/1.0.4/cores/esp32/Esp.h
    snprintf(msgBuff, sizeOfMsgBuff, "%" PRIu32, ESP.getFreeHeap() / 1024);
    buffToDoc("freeKb");
    snprintf(msgBuff, sizeOfMsgBuff, "%" PRIu32, ESP.getMinFreeHeap() / 1024);
    buffToDoc("minFreeKb");
    snprintf(msgBuff, sizeOfMsgBuff, "%" PRIu32, ESP.getMaxAllocHeap() / 1024);
    buffToDoc("maxAllocKb");
    if (!sendCommon(MQTT_PUB_OPER_STATE_MEMORY, mqttConfig.service_pub_oper_state_memory))
        return false;

    // etc
    const WifiConfigData &cnf = wifiConfig_get();
    const bool dogWatch = cnf.needPeriodicPings;
    msgDoc.clear();
    snprintf(msgBuff, sizeOfMsgBuff, "0x%016llx", getActivePixels());
    buffToDoc("pixelsOn");
    snprintf(msgBuff, sizeOfMsgBuff, "%" PRIu32, lightUnitsSize());
    buffToDoc("lightUnitsSize");
    snprintf(msgBuff, sizeOfMsgBuff, "%s", dogWatch ? "yes" : "no");
    buffToDoc("watchDog");
    if (!sendCommon(MQTT_PUB_OPER_STATE_ETC, mqttConfig.service_pub_oper_state_etc))
        return false;

    return true;
}

bool sendButtonEvent()
{
    Adafruit_MQTT_Client &mqtt = *mqttConfig.mqttPtr;
    if (!mqtt.connected())
        return false;

    msgDoc.clear();

    // padded: "%016llx"  variable: "0x%llx"
    snprintf(msgBuff, sizeOfMsgBuff, "0x%016llx",
             state.buttons.pendingPressEvent ^ state.buttons.pendingLongPressEvent);
    buffToDoc("p");

    snprintf(msgBuff, sizeOfMsgBuff, "0x%016llx", state.buttons.pendingLongPressEvent);
    buffToDoc("l");

    snprintf(msgBuff, sizeOfMsgBuff, "0x%016llx", state.buttons.abortedPendingPressEvent);
    buffToDoc("x");

    return sendCommon(MQTT_PUB_BUTTONS, mqttConfig.service_pub_buttons);
}

bool isMqttConnected()
{
    return mqttState.lastMqttConnected;
}

void nvClearRequest()
{
    // Only honor that request if we have started in less than 2 minutes
    if (state.uptimeInMinutes < 2)
    {
        wifiConfig_init(true /*forceNvClear*/);
        gameOver("Bug: should never make it this far after nv clear");
    }
}
