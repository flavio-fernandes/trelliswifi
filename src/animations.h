#ifndef __TRELLIS_LIGHT_ANIMATIONS
#define __TRELLIS_LIGHT_ANIMATIONS

#include "colors.h"

void localButtonProcess(uint64_t pressed, uint64_t longPressed);

void startAnimationScanBase(uint64_t expiration=0, uint32_t color=colorHiRed);
inline void startAnimationScan() { startAnimationScanBase(); }
void stopAnimationScan();

void startAnimationCounterBase(uint64_t expiration=0, uint64_t startCounter=0,
			       uint32_t color=colorYellow, uint32_t speed=10 /*1sec*/);
void setAnimationCounterTypeAdd();
void setAnimationCounterTypeShift();
inline void startAnimationCounter1() { startAnimationCounterBase(0); }
inline void startAnimationCounter2() { startAnimationCounterBase(0, 0xFFFFFFFFFFFFFFF0ULL, 0); }
inline void startAnimationCounter3() { startAnimationCounterBase(0, 0xFFFFFFFFFFFFFF00ULL, colorGreen, 0); }
inline void startAnimationCounter4() { startAnimationCounter1(); setAnimationCounterTypeShift(); }
inline void startAnimationCounter5() { startAnimationCounter2(); setAnimationCounterTypeShift(); }
inline void startAnimationCounter6() { startAnimationCounter3(); setAnimationCounterTypeShift(); }
void stopAnimationCounter();

void startAnimationCrazyBase(uint64_t expiration=0);
inline void startAnimationCrazy() { startAnimationCrazyBase(); }
void stopAnimationCrazy();

void startAnimationFlashlight(uint64_t expiration=0, uint32_t color=colorWhite,
			      bool pulse=false, bool blink=false, bool doneCallback=true);
void startAnimationFlashlight1();
void startAnimationFlashlight2();
void startAnimationFlashlight3();
void startAnimationFlashlight4();
void stopAnimationFlashlight();

void startAnimationLowBattery();

#endif // __TRELLIS_LIGHT_ANIMATIONS
