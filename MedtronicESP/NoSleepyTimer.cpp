#include <Arduino.h>
#include "NoSleepyTimer.h"
//************************************************************************************
// Public functions
//************************************************************************************
// Add the sleep time and the runtime of the current cycle.
void timePrepareSleep(uint32_t *timeSinceRun, uint32_t *tempStart, uint8_t sleepTime) {
    *timeSinceRun = *timeSinceRun + (sleepTime * min_to_ms) + millis();
    if (*timeSinceRun > timeOverflow) {
        *tempStart = *timeSinceRun - *tempStart;
        *timeSinceRun = 0;
    }
}
// Return the time since start and current run time.
uint32_t timeNow(uint32_t *timeSinceRun) {
    return (*timeSinceRun + millis());
}