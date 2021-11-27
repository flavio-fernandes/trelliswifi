#include "common.h"

#include "tickerScheduler.h"

// Board used: Adafruit HUZZAH ESP32
// https://www.adafruit.com/product/3405

// FWDs
static void updateBatteryCache();

TickerScheduler ts;
State state;

static float batterySensorValue = 3.7;

void initPins()
{
  pinMode(A13, INPUT);
  randomSeed(analogRead(A13));
}

void initGlobals()
{
  memset(&state, 0, sizeof(state));
  state.initIsDone = false;
}

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("Init starting");
#endif

  // stage 1
  initGlobals();
  initPins();
  updateBatteryCache();

  // stage 2
  initTrellis(ts);
  initButtons(ts);

  // stage 3
  initMyMqtt(ts);
  initCmdOpHandlers();

#ifdef DEBUG
  Serial.println("Init finished");
  Serial.print("sizeof(int) == ");
  Serial.println(sizeof(int), DEC);
  Serial.print("sizeof(unsigned long) == ");
  Serial.println(sizeof(unsigned long), DEC);
  Serial.print("sizeof(unsigned long long) == ");
  Serial.println(sizeof(unsigned long long), DEC);
#endif

  const uint32_t oneSec = 1000;
  const uint32_t tenMin = oneSec * 60 * 10;
  ts.sched(updateBatteryCache, tenMin);

  state.initIsDone = true;
}

static void updateBatteryCache()
{
  batterySensorValue = (float)analogRead(A13);

#ifdef DEBUG
  float batteryVoltage;
  const bool isLow = isBatteryLow(&batteryVoltage);
  Serial.printf("Battery read is (raw %.0f) %.2f volts%s\n",
                batterySensorValue,
                batteryVoltage,
                isLow ? " (low)" : "");
#endif
}

bool isBatteryLow(float *batteryVoltagePtr)
{
  // ref: https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/power-management#measuring-battery-4-8
  // ref: http://cuddletech.com/?p=1030
  const float batteryVoltage = (batterySensorValue / 4095) * 2 * 3.3 * 1.1;
  if (batteryVoltagePtr)
    *batteryVoltagePtr = batteryVoltage;
  // Dec/25/19: At 3.45v, I see about 60 mins left before it goes dark.
  return batteryVoltage < 3.45; // something between 3.9 (full) and 3.2 (depleted).
}

void loop()
{
  myMqttLoop();
  ts.update();
}
