#include "animations.h"
#include "lightUnit.h"
#include "wifiConfig.h"

// The smaller the id, the more likely it will mask
// the pixels of the other ids.
typedef enum animationId_t {
  animationIdCommon = minDynamicId - 1,
  animationIdFlashlight = animationIdCommon,
  animationIdScan = animationIdCommon,
  animationIdCounter = animationIdCommon,
  animationIdCrazy = animationIdCommon,
  animationIdLowBattery = minDynamicId - 2,
} AnimationId;

// short and long press combo button triggers
static const uint64_t nvClearTrigger[] = { 0x0000000000000000ULL, 0x8100000000000081ULL };
static const uint64_t flashLightOn[] =   { 0x0000000000000000ULL, 0x0000000000000080ULL };
static const uint64_t flashLight2On[] =  { 0x0000000000000040ULL, 0x0000000000000080ULL };
static const uint64_t flashLight3On[] =  { 0x0000000000000020ULL, 0x0000000000000080ULL };
static const uint64_t flashLight4On[] =  { 0x0000000000000010ULL, 0x0000000000000080ULL };
static const uint64_t flashLightOff[] =  { 0x0000000000000080ULL, 0x0000000000000000ULL };
static const uint64_t scanOn[] =         { 0x0000000000000000ULL, 0x0000000000008080ULL };
static const uint64_t scanOff[] =        { 0x0000000000008080ULL, 0x0000000000000000ULL };
static const uint64_t crazyOn[] =        { 0x0000000000000000ULL, 0x0000000000808080ULL };
static const uint64_t crazyOff[] =       { 0x0000000000808080ULL, 0x0000000000000000ULL };
static const uint64_t counter1On[] =     { 0x0000000000000000ULL, 0x0000000080808080ULL };
static const uint64_t counter2On[] =     { 0x0000000000000040ULL, 0x0000000080808080ULL };
static const uint64_t counter3On[] =     { 0x0000000000000020ULL, 0x0000000080808080ULL };
static const uint64_t counter4On[] =     { 0x0000000000004000ULL, 0x0000000080808080ULL };
static const uint64_t counter5On[] =     { 0x0000000000002000ULL, 0x0000000080808080ULL };
static const uint64_t counter6On[] =     { 0x0000000000001000ULL, 0x0000000080808080ULL };
static const uint64_t counterOff[] =     { 0x0000000080808080ULL, 0x0000000000000000ULL };

void localButtonProcess(uint64_t pressed, uint64_t longPressed) {
  if (pressed == nvClearTrigger[0] && longPressed == nvClearTrigger[1]) nvClearRequest();
  if (pressed == flashLightOn[0] && longPressed == flashLightOn[1]) startAnimationFlashlight();
  if (pressed == flashLight2On[0] && longPressed == flashLight2On[1]) startAnimationFlashlight2();
  if (pressed == flashLight3On[0] && longPressed == flashLight3On[1]) startAnimationFlashlight3();
  if (pressed == flashLight4On[0] && longPressed == flashLight4On[1]) startAnimationFlashlight4();
  if (pressed == flashLightOff[0] && longPressed == flashLightOff[1]) stopAnimationFlashlight();

  if (pressed == scanOn[0] && longPressed == scanOn[1]) startAnimationScan();
  if (pressed == scanOff[0] && longPressed == scanOff[1]) stopAnimationScan();

  if (pressed == counter1On[0] && longPressed == counter1On[1]) startAnimationCounter1();
  if (pressed == counter2On[0] && longPressed == counter2On[1]) startAnimationCounter2();
  if (pressed == counter3On[0] && longPressed == counter3On[1]) startAnimationCounter3();
  if (pressed == counter4On[0] && longPressed == counter4On[1]) startAnimationCounter4();
  if (pressed == counter5On[0] && longPressed == counter5On[1]) startAnimationCounter5();
  if (pressed == counter6On[0] && longPressed == counter6On[1]) startAnimationCounter6();
  if (pressed == counterOff[0] && longPressed == counterOff[1]) stopAnimationCounter();

  if (pressed == crazyOn[0] && longPressed == crazyOn[1]) startAnimationCrazy();
  if (pressed == crazyOff[0] && longPressed == crazyOff[1]) stopAnimationCrazy();
}


void startAnimationScanBase(uint64_t expiration, uint32_t color) {
  LightUnit lightUnit = {0};
  // add a base unit instance that all others will depend on
  // otherwise, it is a noop
  lightUnit.animation.speed = 18000;  // 1800 seconds (it is a noop entry anyways)
  lightUnit.animation.expiration = expiration;
  lightUnit.animation.dependsOn = (LightUnitId) animationIdScan;
  const LightUnitId baseId = addLightUnit(lightUnit);

  lightUnit.animation.dependsOn = baseId;
  setLightUnit((LightUnitId) animationIdScan, lightUnit);

  lightUnit.animation.speed = 1;   // 0.1 seconds
  lightUnit.animation.expiration = 0;
  lightUnit.animation.frames = 14;
  lightUnit.pixelMask = 0xff;
  for (uint32_t row = 0; row < lightUnit.animation.frames; ++row) {
    // clear previous row
    lightUnit.color = 0; addLightUnit(lightUnit);

    if (row < 7) lightUnit.pixelMask *= 0x100;  // fwd
    else lightUnit.pixelMask /= 0x100;    // backwards

    lightUnit.color = color; addLightUnit(lightUnit);
    ++lightUnit.animation.step;
  }
}
void stopAnimationScan() { rmLightUnit((LightUnitId) animationIdScan); }


void _animationCounterIterateCommon(struct LightUnit_t& lightUnit, bool isAdd) {
  // clear existing pixels by calling expiration iteration
  lightUnit.iterateCallback = nullptr;  // to avoid infinite loop!
  lightUnitFinalIteration(&lightUnit, false /*callTrellisShow*/);

  if (isAdd) {
    ++lightUnit.pixelMask;
    setAnimationCounterTypeAdd();
  } else {
    lightUnit.pixelMask <<= 1;
    if (lightUnit.pixelMask == 0) lightUnit.pixelMask = 1;
    setAnimationCounterTypeShift();
  }
}
void animationCounterIterateAdd(struct LightUnit_t& lightUnit) { _animationCounterIterateCommon(lightUnit, true); }
void animationCounterIterateShift(struct LightUnit_t& lightUnit) {  _animationCounterIterateCommon(lightUnit, false); }
void startAnimationCounterBase(uint64_t expiration, uint64_t startCounter, uint32_t color, uint32_t speed) {
  LightUnit lightUnit = {0};
  lightUnit.animation.expiration = expiration;
  if (color == 0) lightUnit.animation.sameRandomColor = true; else lightUnit.color = color;
  lightUnit.pixelMask = startCounter;
  lightUnit.animation.speed = speed;
  lightUnit.iterateCallback = &animationCounterIterateAdd;
  setLightUnit((LightUnitId) animationIdCounter, lightUnit);
}
static void _setAnimationCounterTypeCommon(bool isAdd) {
  LightUnit lightUnit;
  if (lightUnitExists((LightUnitId) animationIdCounter, &lightUnit)) {
    lightUnit.iterateCallback = isAdd ? &animationCounterIterateAdd : &animationCounterIterateShift;
    setLightUnit(lightUnit.id, lightUnit, false /*rmBeforeAdd*/, true /*quiet*/);
  }
}
void setAnimationCounterTypeAdd() { _setAnimationCounterTypeCommon(true /*isAdd*/); }
void setAnimationCounterTypeShift() { _setAnimationCounterTypeCommon(false /*isAdd*/); }
void stopAnimationCounter() { rmLightUnit((LightUnitId) animationIdCounter); }

void startAnimationCrazyBase(uint64_t expiration) {
  LightUnit lightUnit = {0};
  lightUnit.animation.expiration = expiration;
  lightUnit.animation.randomColor = true;
  lightUnit.animation.randomPixels = true;
  setLightUnit((LightUnitId) animationIdCrazy, lightUnit);
}
void stopAnimationCrazy() { rmLightUnit((LightUnitId) animationIdCrazy); }

void animationFlashlightDone(const struct LightUnit_t& /*unit*/) {
  startAnimationCrazyBase(5 /*5secs*/);
}
void startAnimationFlashlight(uint64_t expiration, uint32_t color, bool pulse, bool blink) {
  LightUnit lightUnit = {0};
  lightUnit.animation.expiration = expiration;
  if (color == 0) lightUnit.animation.sameRandomColor = true; else lightUnit.color = color;
  lightUnit.pixelMask = ~0ULL;
  lightUnit.animation.pulse = pulse;
  lightUnit.animation.blink = blink;
  lightUnit.animation.speed = 10; // 1 second (note: ignored if pulse is true)
  lightUnit.doneCallback = &animationFlashlightDone;
  setLightUnit((LightUnitId) animationIdFlashlight, lightUnit);  // addLightUnit(lightUnit);
}
void startAnimationFlashlight1() { startAnimationFlashlight(); }
void startAnimationFlashlight2()  { startAnimationFlashlight(0 /*expiration*/, 0 /*color*/, false /*pulse*/); }
void startAnimationFlashlight3()  { startAnimationFlashlight(0 /*expiration*/, colorHiRed, true /*pulse*/); }
void startAnimationFlashlight4()  { startAnimationFlashlight(0 /*expiration*/, colorBlue, false /*pulse*/, true /*blink*/); }
void stopAnimationFlashlight() { rmLightUnit((LightUnitId) animationIdFlashlight); }

void startAnimationLowBattery(uint64_t expiration, bool pulse=true) {
  LightUnit lightUnit = {0};
  lightUnit.animation.expiration = expiration;
  lightUnit.color = colorHiRed;
  lightUnit.animation.pulse = pulse;
  lightUnit.animation.speed = 50; // 5 seconds (note: ignored if pulse is true)
  lightUnit.pixelMask = 0x00183c2424243c00ULL;
  setLightUnit((LightUnitId) animationIdLowBattery, lightUnit); //addLightUnit(lightUnit);
}
void startAnimationLowBattery() { startAnimationLowBattery(200 /*20secs*/); }

void startAnimationRGB() {
  LightUnit lightUnit = {0};

  lightUnit.pixelMask = 0x202;
  lightUnit.color = colorGreen;
  lightUnit.animation.frames = 3;
  lightUnit.animation.step = 0;
  lightUnit.animation.speed = 10;   // 1 seconds
  lightUnit.animation.expiration = 210;  // 21 seconds
  setLightUnit(10, lightUnit); // addLightUnit(lightUnit);

  lightUnit.animation.dependsOn = 10; lightUnit.animation.expiration = 0;
  lightUnit.color = colorRed; lightUnit.animation.step = 1; setLightUnit(11, lightUnit); // addLightUnit(lightUnit);
  lightUnit.color = colorBlue; lightUnit.animation.step = 2; setLightUnit(12, lightUnit); // addLightUnit(lightUnit);
}

