// Common.cpp
#include "Common.h"

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE webCommandMux = portMUX_INITIALIZER_UNLOCKED;
volatile SemaphoreHandle_t timerSemaphore;

volatile uint32_t isrCounter = 0;
volatile int64_t isrBootTime = 0;
volatile uint32_t isrLoop = 0;

hw_timer_t* phaseTimer = NULL;
