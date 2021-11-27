#ifndef __TRELLIS_LIGHT_UNIT
#define __TRELLIS_LIGHT_UNIT

#include <inttypes.h>

struct LightUnit_t;
typedef void(IterateCallback)(struct LightUnit_t &unit);
typedef void(DoneCallback)(const struct LightUnit_t &unit);

typedef int LightUnitId;
static const LightUnitId minDynamicId = 512;

typedef struct LightUnitAnimation_t
{
  uint32_t frames;        // total number of frames this is part of
  uint32_t step;          // the frame index for this
  uint32_t speed;         // overriden by pulse. how many refreshes between each frame (in 100 ms units)
  uint64_t expiration;    // how many steps until animation stops (0 => never)
  LightUnitId dependsOn;  // will expire if entry it depends on is gone (0 => no-dep)
  bool randomPixels;      // pick a random pixelMask
  bool sameRandomColor;   // overriden by randomColor. Pick a random color for pixelMask
  bool randomColor;       // overriden by rainbowColor. Pick a random color for each pixel in mask
  bool rainbowColor;      // pick rainbow color for pixelMask (or pixel, when used with randomColor)
  bool keepPixelWhenDone; // upon expiration, leave pixelMask alone?
  bool blink;             // set color to 0 on every other frame
  bool pulse;             // overriden by rainbowColor, randomColor, sameRandomColor.
                          // if true, will apply modify brightness to color
} LightUnitAnimation;

typedef struct LightUnitState_t
{
  bool iterated;            // has it been iterated?
  bool pulseGoingUp;        // pulse helper
  uint32_t pulseBrightness; // pulse helper
  uint64_t age;             // increases on every tick
  uint64_t tempPixels;
  uint64_t tempCounter; // helper used for blink and randomPixels
} LightUnitState;

typedef struct LightUnit_t
{
  LightUnitId id;
  uint64_t pixelMask; // overriden by randomPixels
  uint32_t color;     // overriden by sameRandomColor
  int8_t brightness;  // overriden by pulse. Adjusts color (0->ignored, 1->dark full->255)
  LightUnitAnimation animation;
  LightUnitState state;
  IterateCallback *iterateCallback; // if set, called when we are about to iterate unit
  DoneCallback *doneCallback;       // if set, called when unit is removed
} LightUnit;

LightUnitId addLightUnit(const LightUnit &lightUnit);
void setLightUnit(LightUnitId id, const LightUnit &lightUnit, bool rmBeforeAdd = true, bool quiet = false);
void rmLightUnit(LightUnitId id);
void rmLightUnits();
bool lightUnitExists(LightUnitId id, LightUnit *lightUnitPtr = nullptr);
void * /*LightUnit**/ getFirstLightUnit();
void * /*LightUnit**/ getNextLightUnit(LightUnitId id);
// uint32_t lightUnitsSize();  // moved to common.h
bool equivalentLightUnits(const LightUnit &left, const LightUnit &right);
void dumpLightUnit(const LightUnit &lightUnit, const char *msg = 0);

#endif // __TRELLIS_LIGHT_UNIT
