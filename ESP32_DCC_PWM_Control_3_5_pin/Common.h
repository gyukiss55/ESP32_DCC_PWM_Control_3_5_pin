// Common.h
#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

extern portMUX_TYPE timerMux;
extern portMUX_TYPE webCommandMux;
extern volatile SemaphoreHandle_t timerSemaphore;

extern volatile uint32_t isrCounter;
extern volatile int64_t isrBootTime;
extern volatile uint32_t isrLoop;

extern hw_timer_t* phaseTimer;
