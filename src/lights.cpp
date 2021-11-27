#include "lightUnit.h"
#include "buttons.h"
#include "common.h"
#include "animations.h"
#include "tickerScheduler.h"

// FWD
static void refreshLights();
static void lightsFastTick();
static void lights100msTick();
static void lights1minTick();

// Light Units handling -- statics
static int pixelColorCacheVersion = 0;
static uint32_t pixelColorCache[64] = {0};
static uint32_t currRefreshTick = 0;
static const uint32_t cacheDirtyBit = 1 << 31;

// Create a matrix of trellis panels, using addressed soldered in
Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {
    {Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F)},
    {Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x31)}};

// Pass this matrix to the multitrellis object
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM / 4, X_DIM / 4);

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
static uint32_t Wheel(byte WheelPos)
{
  if (WheelPos < 85)
  {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  else
  {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}
static uint32_t Wheel()
{
  static byte currRainbowShade = 0;
  currRainbowShade += 9; // let the byte wrap
  return Wheel(currRainbowShade);
}

static void pressedButtonAnimation()
{
  if (!state.buttons.pressed)
    return; // noop

  const uint32_t animationColor = Wheel();
  for (int i = 0; i < Y_DIM * X_DIM; ++i)
  {
    if (getFlag(state.buttons.pressed, i))
    {
      const uint32_t &pressedCounter = state.buttons.buttonPressedCounter[i];
      if (pressedCounter < minPressThreshold)
      {
        trellis.setPixelColor(i, 0x10); // blue while not yet down long enough
      }
      else if (pressedCounter < longPressThreshold)
      {
        trellis.setPixelColor(i, animationColor);
      }
      else if (pressedCounter < maxPressThreshold)
      {
        // 0xff == blue 0xff00 == green 0xff0000 == red
        trellis.setPixelColor(i, 0xff00ff);
      }
      else
      {
#ifdef DEBUG
        Serial.print("Warning: Button ");
        Serial.print(i + 1, DEC);
        Serial.println(" is stuck!");
#endif
        // Note: at this point, we will quietly pretend the button was
        //       never pressed and add a bit to the aborted flag. This
        //       will affect the aborted button until that get cleared
        //       via the notification's call.
        clearFlag(state.buttons.pressed, i);
        clearFlag(state.buttons.pendingPressEvent, i);
        clearFlag(state.buttons.pendingLongPressEvent, i);
        setFlag(state.buttons.abortedPendingPressEvent, i);
        trellis.setPixelColor(i, 0xff0000);
      }
    }
  }
}

static uint64_t unpressedButtonUpdate()
{
  if (!state.buttons.changedState)
    return 0; // noop

  uint64_t unpressedMask = 0;
  for (int i = 0; i < Y_DIM * X_DIM; ++i)
  {
    if (getFlag(state.buttons.changedState, i) &&
        !getFlag(state.buttons.pressed, i) &&
        !getFlag(state.buttons.abortedPendingPressEvent, i))
    {
      trellis.setPixelColor(i, 0);
      pixelColorCache[i] |= cacheDirtyBit;
      setFlag(unpressedMask, i);

#ifdef DEBUG
      static char buff[17];
      Serial.print("button ");
      snprintf(buff, sizeof(buff), "%02u", i + 1);
      Serial.print(buff);
      Serial.print(" released after ");
      snprintf(buff, sizeof(buff), "%3u", state.buttons.buttonPressedCounter[i]);
      Serial.print(buff);
      Serial.print(" x 100 ms");
      Serial.println("");
#endif
    }
  }
  return unpressedMask;
}

// HACK ALERT! No idea but it really needs the line below in order to
// compile :(
extern TrellisCallback keyPressCallback(keyEvent evt);

void initTrellis(TickerScheduler &ts)
{
  if (!trellis.begin())
  {
    Serial.println("failed to begin trellis");
    while (1)
      ;
  }

  // Init animation
  for (int i = 0; i < Y_DIM * X_DIM; ++i)
  {
    trellis.setPixelColor(i, Wheel(map(i, 0, X_DIM * Y_DIM, 0, 255))); // Addressed with keynum
    trellis.show();
    delay(6);
  }

  for (int y = 0; y < Y_DIM; ++y)
  {
    for (int x = 0; x < X_DIM; ++x)
    {
      // Activate rising and falling edges on all keys
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, keyPressCallback);
    }
  }
  clearLights(true);

  // Init tickers
  const uint32_t oneSec = 1000;
  const uint32_t oneMin = oneSec * 60;

  ts.sched(lightsFastTick, 20);
  ts.sched(lights100msTick, 100);
  ts.sched(lights1minTick, oneMin);
}

static void lightsFastTick()
{
  trellis.read();

  if (state.buttons.pressed || state.buttons.changedState)
  {
    pressedButtonAnimation();
    const uint64_t unpressedMask = unpressedButtonUpdate();
    trellis.show();

    // Done processing changes
    if (state.buttons.changedState)
    {

      if (unpressedMask)
      {
#ifdef DEBUG
        Serial.print("Buttons released: ");
#endif
        for (int i = 0; i < Y_DIM * X_DIM; ++i)
        {
          if (getFlag(unpressedMask, i) &&
              !getFlag(state.buttons.abortedPendingPressEvent, i) &&
              state.buttons.buttonPressedCounter[i] >= minPressThreshold)
          {
            setFlag(state.buttons.pendingPressEvent, i);
            if (state.buttons.buttonPressedCounter[i] >= longPressThreshold)
            {
              setFlag(state.buttons.pendingLongPressEvent, i);
            }
#ifdef DEBUG
            Serial.print(i + 1, DEC);
            Serial.print(" ");
#endif
          }
        }
#ifdef DEBUG
        Serial.println("");
#endif
      }

      state.buttons.changedState = 0;

      // If no buttons are pressed, reset the pressed counter on all
      if (state.buttons.pressed == 0)
      {
        for (int i = 0; i < Y_DIM * X_DIM; ++i)
          state.buttons.buttonPressedCounter[i] = 0;
      }
    }
  }
}

static void lights100msTick()
{
  refreshLights();
}

static void lights1minTick()
{
  if (isBatteryLow())
    startAnimationLowBattery();
}

void clearLights(bool callTrellisShow)
{
  for (int i = 0; i < Y_DIM * X_DIM; ++i)
    trellis.setPixelColor(i, 0);
  if (callTrellisShow)
    trellis.show();
}

// Light Units handling
static uint32_t applyBrightness(const void * /*LightUnit**/ lightUnitPtr,
                                LightUnitState &unitState,
                                uint32_t color)
{
  const LightUnit &lightUnit =
      *reinterpret_cast<const LightUnit *>(lightUnitPtr);
  uint32_t brightness = (uint32_t)lightUnit.brightness;
  static const uint32_t brightnessIncr = 12;
  if (lightUnit.animation.pulse)
  {
    if (unitState.pulseGoingUp)
    {
      if (unitState.pulseBrightness >= (250 - brightnessIncr))
        unitState.pulseGoingUp = !unitState.pulseGoingUp;
      else
        unitState.pulseBrightness += brightnessIncr;
    }
    else
    {
      if (unitState.pulseBrightness <= (2 + brightnessIncr))
        unitState.pulseGoingUp = !unitState.pulseGoingUp;
      else
        unitState.pulseBrightness -= brightnessIncr;
    }
    brightness = unitState.pulseBrightness;
  }
  // ref: https://github.com/adafruit/Adafruit_Seesaw/blob/fe3634ce7af7451330fff65b150960aa32d581bf/seesaw_neopixel.cpp#L190
  if (color != 0 && brightness != 0)
  {
    // Apply brightness
    uint32_t blue = ((color & 0xff) * brightness) >> 8;
    uint32_t green = (((color >> 8) & 0xff) * brightness) >> 8;
    uint32_t red = (((color >> 16) & 0xff) * brightness) >> 8;

    return (blue & 0xff) | ((green & 0xff) << 8) | ((red & 0xff) << 16);
  }
  return color;
}

static void lightUnitIterate(const void * /*LightUnit**/ lightUnitPtr,
                             LightUnitState &unitState, bool isExpired)
{
  const LightUnit &lightUnit =
      *reinterpret_cast<const LightUnit *>(lightUnitPtr);
  uint32_t color = lightUnit.color;

  if (isExpired && !lightUnit.animation.keepPixelWhenDone)
    color = 0;
  else if (lightUnit.animation.rainbowColor)
    color = Wheel();
  else if (lightUnit.animation.randomColor)
    color = 1; // anything but zero is good here
  else if (lightUnit.animation.sameRandomColor)
    color = random(1, 0x00ffffff);

  if (lightUnit.animation.blink && ++unitState.tempCounter % 2 == 0)
    color = 0;
  else
    color = applyBrightness(lightUnitPtr, unitState, color);

  uint64_t pixels = lightUnit.pixelMask;
  if (lightUnit.animation.randomPixels)
  {
    // Last one includes all pixels
    if (isExpired && !lightUnit.animation.keepPixelWhenDone)
      pixels = ~0ULL;
    else
    {
      // Note: random() returns 31 bits. So, we will shift and xor it with previous value.
      pixels = random(0, 0x7fffffffUL);
      pixels ^= unitState.tempPixels * 2;
      unitState.tempPixels *= 0x100000000;
      unitState.tempPixels ^= pixels;
      // Override color to 0 on every 3rd frame
      if (!lightUnit.animation.blink)
        ++unitState.tempCounter; // incr only if needed
      if (unitState.tempCounter % 3 == 0)
        color = 0;
    }
  }

  for (int i = 0; pixels && i < 64; ++i)
  {
    if (!clearFlag(pixels, i))
      continue;
    if (color != 0 && lightUnit.animation.randomColor)
    {
      color = lightUnit.animation.rainbowColor ? Wheel() : random(1, 0x00ffffff);
    }
    if (pixelColorCache[i] == color)
      continue;

    // If button for this pixel is being pressed, simply make cached value dirty.
    // And do not mess with the actual pixel.
    if (getFlag(state.buttons.pressed | state.buttons.abortedPendingPressEvent, i))
    {
      pixelColorCache[i] = color | cacheDirtyBit;
      continue;
    }

    trellis.setPixelColor(i, color); // prep trellis
    pixelColorCache[i] = color;      // update cache
    ++pixelColorCacheVersion;        // bump cache
  }
}

void lightUnitFinalIteration(void * /*LightUnit**/ lightUnitPtr,
                             bool callTrellisShow)
{
  const int origCacheVersion = pixelColorCacheVersion;
  LightUnit &lightUnit = *reinterpret_cast<LightUnit *>(lightUnitPtr);
  lightUnitIterate(lightUnitPtr, lightUnit.state, true /*isExpired*/);
  if (callTrellisShow &&
      origCacheVersion != pixelColorCacheVersion)
    trellis.show();
}

static void refreshLights()
{
  const int origCacheVersion = pixelColorCacheVersion;
  LightUnit *unitPtr = reinterpret_cast<LightUnit *>(getFirstLightUnit());
  while (unitPtr != nullptr)
  {
    LightUnit &unit = *unitPtr;
    const LightUnitId currId = unit.id;
    const LightUnitAnimation &animation = unit.animation;
    LightUnitState &unitState = unit.state;

    const bool isExpired = (animation.expiration && unitState.age++ >= animation.expiration) ||
                           (animation.dependsOn && !lightUnitExists(animation.dependsOn));
    if (isExpired ||
        (!unitState.iterated && animation.step == 0 && animation.speed > 1) ||
        (currRefreshTick % animation.speed == 0 &&
         (int)(currRefreshTick % animation.frames) == (int)animation.step))
    {
      if (unit.iterateCallback)
        unit.iterateCallback(unit);
      lightUnitIterate(unitPtr, unitState, isExpired);
      unitState.iterated = true;
      if (isExpired)
        rmLightUnit(currId);
    }
    unitPtr = reinterpret_cast<LightUnit *>(getNextLightUnit(currId));
  }

  // New version means we need to refresh trellis
  if (origCacheVersion != pixelColorCacheVersion)
    trellis.show();
  ++currRefreshTick;
}

uint64_t getActivePixels()
{
  uint64_t result = 0;
  for (int i = 0; i < 64; ++i)
  {
    // Note: mask out dirty bit from cache value
    if ((pixelColorCache[i] | cacheDirtyBit) != cacheDirtyBit)
      setFlag(result, i);
  }
  return result;
}
