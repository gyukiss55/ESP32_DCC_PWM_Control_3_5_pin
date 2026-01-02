// PWMCommander.cpp


#define _DCCSimpleWebServer_ 1

#include "Arduino.h"

#define TIMER_BASE_CLK APB_CLK_FREQ


#include "ESP32_DCC_PWM_Control_3_5_pin.h"
#include "DCCWebCommandParser.h"
#include "Common.h"

#include "PWMCommander.h"

// 4+1 bit control

#if defined DCC_Commander
#else

#include "ESP32TimerInterrupt.h"

#if defined DCC_4_PWM
#define PWM_RAIL1_OUT_PIN  18
#define PWM_RAIL2_OUT_PIN   5
#define PWM_RAIL3_OUT_PIN  17
#define PWM_RAIL4_OUT_PIN  16
#else
#define PWM_RAIL1_OUT_PIN  17
#define PWM_RAIL2_OUT_PIN  16
#endif


#define PWM_RAILEN_OUT_PIN  4

#define PWM_PERIOD          100
#define PWM_CHANGE_PERIOD   200000
#define PWM_BREAK_STEP      3

volatile bool isrPWMdirectionNext;
volatile bool isrPWMdirectionNow;
volatile uint16_t isrPWMvalueNext;  // dutyCycle
volatile uint16_t isrPWMvalueNow;
volatile bool isrPWMplus;
volatile bool isrDirectionChange;
volatile uint32_t isrTimerAlarmValue;

bool reqPWMdirection;
uint16_t reqPWMvalue;
uint64_t isrBootTimeLoopLast;
uint8_t firstStep = 1;
const uint8_t speedMap[] = {0,20,23,25,27,30,32,34,36,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,75,80,85,90,95,99};

#include "LEDFunction.h"

void IRAM_ATTR OnTimerPWM() {

    digitalWrite(PWM_RAILEN_OUT_PIN, false);

    portENTER_CRITICAL_ISR(&timerMux);
    isrCounter++;

    isrBootTime = esp_timer_get_time();

    portEXIT_CRITICAL_ISR(&timerMux);

    isrDirectionChange = false;
    if (isrPWMplus) {
        if (isrPWMdirectionNext != isrPWMdirectionNow)
            isrDirectionChange = true;
        isrPWMdirectionNow = isrPWMdirectionNext;
        if (isrPWMvalueNext != isrPWMvalueNow) {
            isrPWMvalueNow = isrPWMvalueNext;
            if (isrPWMvalueNow < 0)
                isrPWMvalueNow = 0;
            if( isrPWMvalueNow >= PWM_PERIOD)
                isrPWMvalueNow = PWM_PERIOD -1;
        }
    }

    isrPWMplus = !isrPWMplus;
 
    isrLoop++;
    if (isrLoop == 120000)
        isrLoop = 0;
    if (isrLoop == 0)
        xSemaphoreGiveFromISR(timerSemaphore, NULL);

    if (!isrPWMplus) {
            digitalWrite(PWM_RAIL2_OUT_PIN, false);
#if defined DCC_4_PWM
            digitalWrite(PWM_RAIL3_OUT_PIN, false);
#endif
            digitalWrite(PWM_RAIL1_OUT_PIN, false);
#if defined DCC_4_PWM
            digitalWrite(PWM_RAIL4_OUT_PIN, false);
#endif
            if (isrPWMvalueNow >= 0 && isrPWMvalueNow < PWM_PERIOD)
                isrTimerAlarmValue = PWM_PERIOD - isrPWMvalueNow;
            else
                isrTimerAlarmValue = PWM_PERIOD;
    }
    else {
        if (isrPWMdirectionNow) {
            digitalWrite(PWM_RAIL1_OUT_PIN, false);
#if defined DCC_4_PWM
            digitalWrite(PWM_RAIL4_OUT_PIN, false);
#endif
            digitalWrite(PWM_RAIL2_OUT_PIN, true);
#if defined DCC_4_PWM
            digitalWrite(PWM_RAIL3_OUT_PIN, true);
#endif
            if (isrPWMvalueNow >= 0 && isrPWMvalueNow < PWM_PERIOD)
                isrTimerAlarmValue = isrPWMvalueNow;
        }
        else {
            digitalWrite(PWM_RAIL2_OUT_PIN, false);
#if defined DCC_4_PWM
            digitalWrite(PWM_RAIL3_OUT_PIN, false);
#endif
            digitalWrite(PWM_RAIL1_OUT_PIN, true);
#if defined DCC_4_PWM
            digitalWrite(PWM_RAIL4_OUT_PIN, true);
#endif
            if (isrPWMvalueNow >= 0 && isrPWMvalueNow < PWM_PERIOD)
                isrTimerAlarmValue = isrPWMvalueNow;
        }
    }

    // timerAlarmWrite(phaseTimer, isrTimerAlarmValue, true); // old
    timerWrite(phaseTimer, 0); // new
    timerAlarm(phaseTimer, isrTimerAlarmValue, true, 0); // new

    if (isrPWMvalueNow > 0)
        digitalWrite(PWM_RAILEN_OUT_PIN, true);
}


void SetupPWMCommander()
{

    pinMode(PWM_RAIL1_OUT_PIN, OUTPUT);
    digitalWrite(PWM_RAIL1_OUT_PIN, false);
    pinMode(PWM_RAIL2_OUT_PIN, OUTPUT);
    digitalWrite(PWM_RAIL2_OUT_PIN, false);
#if defined DCC_4_PWM
    pinMode(PWM_RAIL3_OUT_PIN, OUTPUT);
    digitalWrite(PWM_RAIL3_OUT_PIN, false);
    pinMode(PWM_RAIL4_OUT_PIN, OUTPUT);
    digitalWrite(PWM_RAIL4_OUT_PIN, false);
#endif

    pinMode(PWM_RAILEN_OUT_PIN, OUTPUT);
    digitalWrite(PWM_RAILEN_OUT_PIN, false);

    isrPWMdirectionNext = false;
    isrPWMdirectionNow = false;
    reqPWMdirection = false;

    isrPWMvalueNext = 0;
    isrPWMvalueNow = 0;
    reqPWMvalue = 0;

    isrPWMplus = false;

    // Create semaphore to inform us when the phaseTimer has fired
    timerSemaphore = xSemaphoreCreateBinary();

    // Use 1st phaseTimer of 4 (counted from zero).
    // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
    // info).
    // phaseTimer = timerBegin(0, 80, true); // old
    phaseTimer = timerBegin(1000000); // new

    // Attach OnTimer function to our phaseTimer.
    // timerAttachInterrupt(phaseTimer, &OnTimerPWM, true); // old
    timerAttachInterrupt(phaseTimer, &OnTimerPWM); // new

    // Set alarm to call OnTimer function every second (value in microseconds).
    // Repeat the alarm (third parameter)
    // timerAlarmWrite(phaseTimer, 1000000, true); // old
    timerAlarm(phaseTimer, 1000000, true, 0); // new

    // Start an alarm
    // timerAlarmEnable(phaseTimer); // old
    timerStart(phaseTimer); // new

}

void SetPWMCommand(bool direction, uint16_t pwmValue)
{
    Serial.print("SetPWMCommand");
    Serial.print(" dir:");
    Serial.print(direction);
    Serial.print(" value:");
    Serial.println(pwmValue);

    portENTER_CRITICAL(&timerMux);
    isrPWMdirectionNext = direction;
    isrPWMvalueNext = pwmValue;
    portEXIT_CRITICAL(&timerMux);
}

void GetCurrentPWMDirAndValue(bool& direction, int16_t& pwmValue)
{
    portENTER_CRITICAL(&timerMux);
    direction = isrPWMdirectionNext;
    pwmValue = isrPWMvalueNext;
    portEXIT_CRITICAL(&timerMux);

}

/*
void SetPWMCommand(uint16_t pwmValue)
{
    portENTER_CRITICAL(&timerMux);
    isrPWMvalueNext = pwmValue;
    portEXIT_CRITICAL(&timerMux);
}
*/

void DumpStatus()
{
    // If Timer has fired
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
        uint32_t isrCount = 0, isrTime = 0;
        int64_t  bootTime = 0;
        bool PWMdirection = false;
        uint16_t PWMvalue = 0;
        // Read the interrupt count and time
        portENTER_CRITICAL(&timerMux);
        isrCount = isrCounter;
        bootTime = isrBootTime;
        PWMdirection = isrPWMdirectionNow;
        PWMvalue = isrPWMvalueNow;
        portEXIT_CRITICAL(&timerMux);
        // Print it
        Serial.print("OnTimer no. ");
        Serial.print(isrCount);
        Serial.print(", dir:");
        if (PWMdirection)
            Serial.print("Forward  ");
        else
            Serial.print("Backward ");
        Serial.print(", dutyCycle:");
        Serial.print((int32_t)PWMvalue);
        Serial.print(", boot time:");
        Serial.print((int32_t)bootTime);
        Serial.println(" us");
    }

}
void ReadWebCommand(std::string& command)
{
    bool direction = false;
    uint8_t speedValue = 0;
    uint8_t firstStepTmp = 0;

    WebCommandParser parser(command);
    if (parser.IsConfigFirstStep(firstStepTmp)) {
       if (firstStepTmp > 0) {
          firstStep = firstStepTmp;
          Serial.print("first step:");
          Serial.println(firstStep);
       }
    }
    if (parser.IsAlertStop()) {
        reqPWMvalue = 0;
        reqPWMdirection = true;

        SetPWMCommand(true, 0);
    }
    else if (parser.GetDirectionAndSpeed(direction, speedValue)) {

        size_t pwmIndex = sizeof(speedMap) - 1;
        if (speedValue < pwmIndex && speedValue >= 0)
            pwmIndex = speedValue;

        reqPWMvalue = speedMap[pwmIndex];
        reqPWMdirection = direction;

        Serial.print("SetPWMCommand dir:");
        Serial.print(reqPWMdirection);
        Serial.print("pwm:");
        Serial.println(reqPWMvalue);
    }
}

void UpdateDirAndPWMValue()
{
    int64_t t = esp_timer_get_time();
    if ((t - isrBootTimeLoopLast) > PWM_CHANGE_PERIOD) {
        isrBootTimeLoopLast = t;

        bool dir = false;
        int16_t pwmValue = 0;
        GetCurrentPWMDirAndValue(dir, pwmValue);
        if (reqPWMdirection != dir) {
            if (pwmValue > 0 && pwmValue >= PWM_BREAK_STEP)
                pwmValue -= PWM_BREAK_STEP;
            else if (pwmValue > 0)
                pwmValue--;
            else
                dir = reqPWMdirection;
            SetPWMCommand(dir, pwmValue);
        }
        else {
            if (pwmValue < reqPWMvalue) {
                if (pwmValue == 0 && firstStep > 0)
                    pwmValue = speedMap[firstStep];
                SetPWMCommand(dir, pwmValue + 1);
            }
            else if (pwmValue > reqPWMvalue) {
                SetPWMCommand(dir, pwmValue - 1);
            }
        }
    }
}

void LoopPWMCommander(std::string& command)
{
    LoopLEDFunction();

    if (command.size() > 0) 
        ReadWebCommand(command);

    UpdateDirAndPWMValue();

    DumpStatus();

}

#endif //DCC_Commander