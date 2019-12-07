#include "buttons.h"
#include "common.h"
#include "animations.h"

extern const uint32_t minPressThreshold = 2;  // 0.2 seconds
extern const uint32_t longPressThreshold = 24;  // 2.4 seconds
extern const uint32_t maxPressThreshold = 150;  // 15 seconds

TrellisCallback keyPressCallback(keyEvent evt) {
  if (evt.bit.EDGE != SEESAW_KEYPAD_EDGE_RISING &&
      evt.bit.EDGE != SEESAW_KEYPAD_EDGE_FALLING) return 0;

  const int buttonIndex = (int) evt.bit.NUM;
  const uint64_t prevPressed = state.buttons.pressed;
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
     setFlag(state.buttons.pressed, buttonIndex);
  } else clearFlag(state.buttons.pressed, buttonIndex);

  if (prevPressed != state.buttons.pressed) {
    setFlag(state.buttons.changedState, buttonIndex);
  }

#ifdef DEBUG
  static char buff[17];
  Serial.print("button ");
  snprintf(buff, sizeof(buff), "%02u", buttonIndex + 1); Serial.print(buff);
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) Serial.print(" DOWN");
  else Serial.print("   UP");
  Serial.print(". buttonsPressed: 0x");
  // state.buttons.pressed = 0xffffffffffffffffULL;
  snprintf(buff, sizeof(buff), "%016llx", state.buttons.pressed); Serial.print(buff);
  Serial.print(". delta: 0x");
  snprintf(buff, sizeof(buff), "%016llx", state.buttons.pressed ^ prevPressed); Serial.print(buff);
  Serial.println("");
#endif

  return 0;
}

inline static void _bumpCounter(uint32_t& counter) {
  if (counter < 0xfffffffeUL) ++counter;
}

void buttons100msTick() {
  if (state.buttons.pressed) {
    for (int i=0; i < 64; ++i) {
      if (getFlag(state.buttons.pressed, i)) _bumpCounter(state.buttons.buttonPressedCounter[i]);

      // If button is in the aborted mask, baseline its counter to make that known.
      // This is expected to happend when button was stuck before and then it got
      // unstuck but no notifications about it was sent out yet.
      if (getFlag(state.buttons.abortedPendingPressEvent, i) &&
	  state.buttons.buttonPressedCounter[i] < maxPressThreshold) {
	state.buttons.buttonPressedCounter[i] = maxPressThreshold;
      }
    }
  }
}

void buttons1secTick() {
  static int buttonsQuiescedCount = 0;
  if (state.buttons.pressed || state.buttons.pendingPressEvent == 0) {
    buttonsQuiescedCount = 0;
    return;
  }
  if (++buttonsQuiescedCount < 2) return;

  const uint64_t pressed = state.buttons.pendingPressEvent ^ state.buttons.pendingLongPressEvent;
  const uint64_t longPressed = state.buttons.pendingLongPressEvent;
#ifdef DEBUG
  static char buff[17];

  Serial.print("event buttonsPressed     0x");
  snprintf(buff, sizeof(buff), "%016llx", pressed);
  Serial.println(buff);

  Serial.print("event buttonsLongPressed 0x");
  snprintf(buff, sizeof(buff), "%016llx", longPressed);
  Serial.println(buff);

  Serial.print("event buttonsAborted     0x");
  snprintf(buff, sizeof(buff), "%016llx", state.buttons.abortedPendingPressEvent);
  Serial.println(buff);
#endif

  localButtonProcess(pressed, longPressed);
  sendButtonEvent();

  // Reset all values after sending event
  state.buttons.pendingPressEvent = 0;
  state.buttons.pendingLongPressEvent = 0;
  state.buttons.abortedPendingPressEvent = 0;
}
