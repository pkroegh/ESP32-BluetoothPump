// Millis() is reset on each sleep cycle. This library provides timing which accounts for millis() reset.

// Ensure this library description is only included once
#ifndef NoSleepyTimer_h
#define NoSleepyTimer_h

#include <Arduino.h>

#define min_to_ms 60000 // Min to milliseconds factor.
#define timeOverflow 4291367296 // Value above which times should reset to 0.

void timePrepareSleep(uint32_t *timeSinceRun, uint32_t *tempStart, uint8_t sleepTime);
uint32_t timeNow(uint32_t *timeSinceRun);

#endif