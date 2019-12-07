#ifndef _BUTTONS_H

#define _BUTTONS_H

#include "Adafruit_NeoTrellis.h"

extern TrellisCallback keyPressCallback(keyEvent evt);

#define Y_DIM 8 //number of rows of key
#define X_DIM 8 //number of columns of keys

extern const uint32_t minPressThreshold;
extern const uint32_t longPressThreshold;
extern const uint32_t maxPressThreshold;

#endif // _BUTTONS_H
