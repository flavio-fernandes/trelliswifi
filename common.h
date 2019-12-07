#ifndef _COMMON_H

#define _COMMON_H

#include <inttypes.h>

// FIXME: turn debug off
//#define DEBUG 1

// FWDs decls... trelliswifi
bool isBatteryLow(float* batteryVoltagePtr);


// FWDs decls... utils
void setFlags(uint64_t& currFlags, uint64_t flags);
bool getFlag(const uint64_t& currFlags, int flagBit);
bool setFlag(uint64_t& currFlags, int flagBit);
bool clearFlag(uint64_t& currFlags, int flagBit);
bool flipFlag(uint64_t& currFlags, int flagBit);
typedef void (*OnOffToggle)();
void parseOnOffToggle(const char* subName, const char* message,
		      OnOffToggle onPtr, OnOffToggle offPtr,
		      OnOffToggle togglePtr);
void gameOver(const char* const msg);

// FWDs decls... lights (aka trellis)
void initTrellis();
void clearLights(bool callTrellisShow);
uint64_t getActivePixels();
uint32_t lightUnitsSize();
void lightUnitFinalIteration(void* /*LightUnit**/ lightUnitPtr,
			     bool callTrellisShow=true);
void lightsFastTick();
void lights100msTick();
void lights1secTick();
void lights5secTick();
void lights1minTick();

// FWDs buttons
void buttons100msTick();
void buttons1secTick();

// FWS decls... mqtt_client
void initMyMqtt();
void myMqttLoop();
void mqtt1SecTick();
void mqtt1MinTick();
void mqtt10MinTick();
void nvClearRequest();

bool sendButtonEvent();
bool sendOperState();
bool isMqttConnected();  // true when mqtt connection is up

// FWS decls... msgHandler
void initCmdOpHandlers();
void parseMqttCmd(const char* msg, size_t msgSize);


typedef struct {
  // Mask with all buttons currently pressed down
  uint64_t pressed;
  // Mask with buttons that changed since last trellisFastTick call
  uint64_t changedState;
  // Counter for every button that goes up while pressed
  uint32_t buttonPressedCounter[64];

  // Masks that keep tabs on the buttons that:
  uint64_t pendingPressEvent;         // was pressed for longer than minPressThreshold
  uint64_t pendingLongPressEvent;     // was pressed for longer than longPressThreshold
  uint64_t abortedPendingPressEvent;  // was pressed for longer than maxPressThreshold
} ButtonsState;

typedef struct {
  bool initIsDone;

  ButtonsState buttons;

  // mqtt related
  uint32_t uptimeInMinutes;
  uint32_t mqttUpInMinutes;
  uint32_t numberOfMsgsSent;
  size_t msgBufferSize;    // constant and used as sanity
  size_t largestMsgSize;   // high watermark
  uint32_t minutes_since_periodic_ping;  // dog
} State;

extern State state;

#endif // _COMMON_H
