#ifndef __TRELLIS_LIGHT_ANIMATIONS
#define __TRELLIS_LIGHT_ANIMATIONS

#include "colors.h"

void localButtonProcess(uint64_t pressed, uint64_t longPressed);

void startAnimationScanBase(uint64_t expiration=0, uint32_t color=colorHiRed);
void startAnimationScan() { startAnimationScanBase(); }
void stopAnimationScan();

void startAnimationCounterBase(uint64_t expiration=0, uint64_t startCounter=0,
			       uint32_t color=colorYellow, uint32_t speed=10 /*1sec*/);
void setAnimationCounterTypeAdd();
void setAnimationCounterTypeShift();
void startAnimationCounter1() { startAnimationCounterBase(0); }
void startAnimationCounter2() { startAnimationCounterBase(0, 0xFFFFFFFFFFFFFFF0ULL, 0); }
void startAnimationCounter3() { startAnimationCounterBase(0, 0xFFFFFFFFFFFFFF00ULL, colorGreen, 0); }
void startAnimationCounter4() { startAnimationCounter1(); setAnimationCounterTypeShift(); }
void startAnimationCounter5() { startAnimationCounter2(); setAnimationCounterTypeShift(); }
void startAnimationCounter6() { startAnimationCounter3(); setAnimationCounterTypeShift(); }
void stopAnimationCounter();

void startAnimationCrazyBase(uint64_t expiration=0);
void startAnimationCrazy() { startAnimationCrazyBase(); }
void stopAnimationCrazy();

void startAnimationFlashlight(uint64_t expiration=0, uint32_t color=colorWhite,
			      bool pulse=false, bool blink=false);
void startAnimationFlashlight1();
void startAnimationFlashlight2();
void startAnimationFlashlight3();
void startAnimationFlashlight4();
void stopAnimationFlashlight();

void startAnimationLowBattery();

#endif // __TRELLIS_LIGHT_ANIMATIONS
