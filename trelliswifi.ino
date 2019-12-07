#include "common.h"

#include "TickerScheduler.h"

// Board used: Adafruit HUZZAH ESP32
// https://www.adafruit.com/product/3405

// FWDs
static void tickerFun20ms();
static void tickerFun100ms();
static void tickerFun250ms();
static void tickerFun1sec();
static void tickerFun10min();
const uint8_t numberOfTickers = 5;

TickerScheduler ts(numberOfTickers);
State state;

static float batterySensorValue = 3.7;

// ----------------------------------------

void initTicker() {
  // ref: https://github.com/Toshik/TickerScheduler/blob/master/TickerScheduler.h
  //      https://buildmedia.readthedocs.org/media/pdf/arduino-esp8266/docs_to_readthedocs/arduino-esp8266.pdf
  //      https://github.com/esp8266/Arduino/tree/master/libraries
  // bool add(uint8_t i, uint32_t period, tscallback_t, void *, boolean shouldFireNow = false);
  const uint32_t oneSec = 1000;
  const uint32_t tenMin = oneSec * 60 * 10;

  ts.add(0, 20, [&](void *) { tickerFun20ms(); }, nullptr, false);
  ts.add(1, 100, [&](void *) { tickerFun100ms(); }, nullptr, false);
  ts.add(2, 250, [&](void *) { tickerFun250ms(); }, nullptr, false);
  ts.add(3, oneSec, [&](void *) { tickerFun1sec(); }, nullptr, false);
  ts.add(4, tenMin, [&](void *) { tickerFun10min(); }, nullptr, false);
}

void initPins() {
  pinMode(A13, INPUT);
  randomSeed(analogRead(A13));
}

void initGlobals() {
    memset(&state, 0, sizeof(state));

    state.initIsDone = false;
#ifdef DEBUG
    Serial.printf("sizeof long: %zu\nsizeof longlong: %zu\nsizeof int: %zu\n",
		  sizeof(long), sizeof(long long), sizeof(int));
#endif
}

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
#endif

    // stage 1
    initGlobals();
    initPins();
    _updateBatteryCache();

    // stage 2
    initTrellis();

    // stage 3
    initMyMqtt();
    initTicker();
    initCmdOpHandlers();

#ifdef DEBUG
    Serial.println("Init finished");
    Serial.print("sizeof(int) == "); Serial.println(sizeof(int), DEC);
    Serial.print("sizeof(unsigned long) == "); Serial.println(sizeof(unsigned long), DEC);
    Serial.print("sizeof(unsigned long long) == "); Serial.println(sizeof(unsigned long long), DEC);
#endif
    state.initIsDone = true;

}

void _updateBatteryCache() {
  batterySensorValue = (float) analogRead(A13);

#ifdef DEBUG
  float batteryVoltage;
  const bool isLow = isBatteryLow(&batteryVoltage);
  Serial.printf("Battery read is (raw %.0f) %.2f volts%s\n",
		batterySensorValue,
		batteryVoltage,
		isLow ? " (low)" : "");
#endif
}

bool isBatteryLow(float* batteryVoltagePtr=nullptr) {
  // ref: https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/power-management#measuring-battery-4-8
  // ref: http://cuddletech.com/?p=1030
  const float batteryVoltage = (batterySensorValue / 4095) * 2 * 3.3 * 1.1;
  if (batteryVoltagePtr) *batteryVoltagePtr = batteryVoltage;
  // Dec/25/19: At 3.45v, I see about 60 mins left before it goes dark.
  return batteryVoltage < 3.45; // something between 3.9 (full) and 3.2 (depleted).
}

static void tickerFun20ms() {
  lightsFastTick();
}

static void tickerFun100ms() {
  buttons100msTick();
  lights100msTick();
}

static void tickerFun250ms() {

}

static void _tickerFun5sec() {
  lights5secTick();
}

static void _tickerFun1min() {
  mqtt1MinTick();
  lights1minTick();
}

static void tickerFun1sec() {
  static int fiveSec = 0;
  static int oneMin = 0;

  buttons1secTick();
  lights1secTick();
  mqtt1SecTick();

  ++fiveSec %= 5; if (!fiveSec) _tickerFun5sec();
  ++oneMin %= 60; if (!oneMin) _tickerFun1min();
}

static void tickerFun10min() {
  _updateBatteryCache();

#ifdef DEBUG
  Serial.println("10 minutes tick");
#endif

  mqtt10MinTick();
}

void loop() {
  myMqttLoop();
  ts.update();
}
