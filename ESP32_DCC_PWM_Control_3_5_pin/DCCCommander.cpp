// DCCCommander.cpp


#define _DCCSimpleWebServer_ 1

#include "Arduino.h"
//#define TIMER_BASE_CLK APB_CLK_FREQ

#include "ESP32_DCC_PWM_Control_3_5_pin.h"
#include "DCCWebCommandParser.h"
#include "Common.h"

#include "DCCCommander.h"

#if defined DCC_Commander

//#include "ESP32TimerInterrupt.h"

#define PARAMNAME_CHANNEL "ch"
#define PARAMNAME_DCCVALUE "dcc"


 // Stop button is attached to PIN 0 (IO0)
#define BTN_STOP_ALARM      0

// 4+1 bit control

#if defined DCC_2_PWM
#define DCC_RAIL1_OUT_PIN  17
#define DCC_RAIL2_OUT_PIN  16
#endif

#if defined DCC_4_PWM
#define DCC_RAIL1_OUT_PIN  18
#define DCC_RAIL2_OUT_PIN   5
#define DCC_RAIL3_OUT_PIN  17
#define DCC_RAIL4_OUT_PIN  16
#endif

#define DCC_RAILEN_OUT_PIN  4
//#define DCC_POCKET_OUT_PIN  2
#define DCC_RAIL_IN_PIN    15

#define DCC_1HALFBITTIME    58
#define DCC_0HALFBITTIME    100
#define MEASURED_TIMES_BUFFER_LEN    256

#define MAX_NR_OF_HEXA_BYTES 6

#define MAX_NR_OF_CHANNEL 4
#define CHANNEL_MASK 0x03

const uint8_t channelMask = CHANNEL_MASK;

volatile uint8_t isrChannel = 0;
volatile uint16_t isrStatus = 0;        // 0 - preamble ('1'x12), 1,3,5 - leading ('0'x1), 2 - address, 4 - command, 6 - CRC, 8 - close ('1'x1)
volatile uint16_t isrPhase = 0;
volatile uint8_t isrPhaseLimit = 12 * 2;
volatile uint8_t isrMask = 0;
volatile bool isrBit = true;

volatile bool isrFirst = false;
volatile bool isrDisableControl = false;

volatile uint8_t isrPacket[MAX_NR_OF_CHANNEL][7]; // 4 channel x max 1 + 6 byte packet (1. byte packet length (0,3...6))  ( 1 - 2 addr + 1 - 2 - 3 command + 1 CRC)
volatile uint8_t isrPacketRequest[MAX_NR_OF_CHANNEL][7]; // 4 channel x max 1 + 6 byte packet (1. byte packet length (0,3...6))  ( 1 - 2 addr + 1 - 2 - 3 command + 1 CRC)

volatile uint32_t lastIsrAt = 0;


void IRAM_ATTR OnTimerDCC() {

    portENTER_CRITICAL_ISR(&timerMux);
    isrCounter++;
    digitalWrite(DCC_RAILEN_OUT_PIN, false);

    isrBootTime = esp_timer_get_time();
    isrPhase++;
    if (isrPhase == isrPhaseLimit) {
        if (!(isrStatus & 1)) {
            isrStatus++;
            isrPhase = 0;
            isrPhaseLimit = 2;
            if ((isrStatus >> 1) <= isrPacket[isrChannel][0])
                isrBit = false;
            else {
                isrBit = true;
                if (isrDisableControl)
                    isrDisableControl = false;
            }
        }
        else  if ((isrStatus >> 1) <= isrPacket[isrChannel][0]) {
            isrStatus++;
            isrPhase = 0;
            isrMask = 0x80;
            isrPhaseLimit = 2 * 8;
        }
        else if (isrDisableControl) {  //read back CV
            isrStatus++;
            isrPhase = 0;
            isrPhaseLimit = 2 * 32;
        }
        else {
            //digitalWrite(DCC_RAIL2_OUT_PIN, true);//debug
            
            isrStatus = 0;
            isrPhaseLimit = 2 * 18;
            isrBit = true;

            for (uint8_t i = 0; i < MAX_NR_OF_CHANNEL; ++i) {
                if (isrPacketRequest[i][0] == 1) {// delete channel packet
                    isrPacket[i][0] = 0;
                    isrPacketRequest[i][0] = 0;
                } if (isrPacketRequest[i][0] >= 2 && isrPacketRequest[i][0] <= 5) {
                    uint8_t crc = 0;
                    for (uint8_t j = 0; j <= isrPacketRequest[i][0]; ++j) {
                        isrPacket[i][j] = isrPacketRequest[i][j];
                        if (j > 0)
                            crc = crc ^ isrPacket[i][j];
                    }
                    isrPacket[i][isrPacket[i][0] + 1] = crc;
                    isrPacketRequest[i][0] = 0;
                }
            }

            uint8_t tch = isrChannel;
            for (uint8_t i = 0; i < MAX_NR_OF_CHANNEL; ++i) {
                ++isrChannel;
                isrChannel = isrChannel & channelMask;
                if (isrPacket[isrChannel][0] != 0)
                    break;
            }
            if (tch == isrChannel) {
                if (isrPacket[isrChannel][0] == 0) {
                    isrPacket[isrChannel][0] = 2;
                    isrPacket[isrChannel][1] = 0xff;
                    isrPacket[isrChannel][2] = 0x00;
                    isrPacket[isrChannel][3] = 0xff;
                }
                ++isrChannel;
                isrChannel = isrChannel & channelMask;
                isrPacket[isrChannel][0] = 2;
                isrPacket[isrChannel][1] = 0xff;
                isrPacket[isrChannel][2] = 0x00;
                isrPacket[isrChannel][3] = 0xff;
            }

        }
    }

    if ((!(isrStatus & 1)) && (isrStatus != 0) && ((isrStatus >> 1) <= isrPacket[isrChannel][0] + 1)) {
         if (!(isrPhase & 1)) {
            isrBit = isrPacket[isrChannel][(isrStatus >> 1)] & isrMask;
            isrMask = isrMask >> 1;
         }
    }

    portEXIT_CRITICAL_ISR(&timerMux);

    if (isrStatus == 0){
        //digitalWrite(DCC_POCKET_OUT_PIN, false);
    }
    else {
        //digitalWrite(DCC_POCKET_OUT_PIN, true);
    }

    if ((!(isrStatus & 1)) && ((isrStatus  >> 1) > isrPacket[isrChannel][0] + 1))
    {
        digitalWrite(DCC_RAILEN_OUT_PIN, true);
    }
    else
    {
        digitalWrite(DCC_RAILEN_OUT_PIN, false);
    }

    isrLoop++;
    if (isrLoop == 120000)
        isrLoop = 0;
    if (isrLoop == 0)
        xSemaphoreGiveFromISR(timerSemaphore, NULL);

    if ((!(isrStatus & 1)) && ((isrStatus >> 1) > isrPacket[isrChannel][0] + 1)) {
            digitalWrite(DCC_RAILEN_OUT_PIN, false);
#if defined DCC_2_PWM            
            digitalWrite(DCC_RAIL2_OUT_PIN, false);
            digitalWrite(DCC_RAIL1_OUT_PIN, true);
#endif
#if defined DCC_4_PWM
            digitalWrite(DCC_RAIL2_OUT_PIN, false);
            digitalWrite(DCC_RAIL3_OUT_PIN, false);
            digitalWrite(DCC_RAIL1_OUT_PIN, true);
            digitalWrite(DCC_RAIL4_OUT_PIN, true);
#endif
    }
    else {
        if (isrPhase & 1) {
            digitalWrite(DCC_RAILEN_OUT_PIN, false);
#if defined DCC_2_PWM            
            digitalWrite(DCC_RAIL1_OUT_PIN, false);
            digitalWrite(DCC_RAIL2_OUT_PIN, true);
#endif
#if defined DCC_4_PWM
            digitalWrite(DCC_RAIL1_OUT_PIN, false);
            digitalWrite(DCC_RAIL4_OUT_PIN, false);
            digitalWrite(DCC_RAIL2_OUT_PIN, true);
            digitalWrite(DCC_RAIL3_OUT_PIN, true);
#endif
            //digitalWrite(DCC_RAILEN_OUT_PIN, true);
        }
        else {
            digitalWrite(DCC_RAILEN_OUT_PIN, false);
#if defined DCC_2_PWM            
            digitalWrite(DCC_RAIL2_OUT_PIN, false);
            digitalWrite(DCC_RAIL1_OUT_PIN, true);
#endif
#if defined DCC_4_PWM
            digitalWrite(DCC_RAIL2_OUT_PIN, false);
            digitalWrite(DCC_RAIL3_OUT_PIN, false);
            digitalWrite(DCC_RAIL1_OUT_PIN, true);
            digitalWrite(DCC_RAIL4_OUT_PIN, true);
#endif
            //digitalWrite(DCC_RAILEN_OUT_PIN, true);
        }
    }
    timerWrite(phaseTimer, 0); // new
    if (isrBit)
        // timerAlarmWrite(phaseTimer, DCC_1HALFBITTIME, true); // old
        timerAlarm(phaseTimer, DCC_1HALFBITTIME, true, 0); // new
    else
        // timerAlarmWrite(phaseTimer, DCC_0HALFBITTIME, true); // old
        timerAlarm(phaseTimer, DCC_0HALFBITTIME, true, 0); // new

   //digitalWrite(DCC_RAIL2_OUT_PIN, false);//debug
   digitalWrite(DCC_RAILEN_OUT_PIN, true);

}

static std::string asyncCommand;
static bool changedAsyncCommand = false;

void CallBackAsyncWebServer(const std::string& com)
{
    changedAsyncCommand = true;
    asyncCommand = com;
}

void SetupDCCCommander()
{
    // Set BTN_STOP_ALARM to input mode
    pinMode(BTN_STOP_ALARM, INPUT);

    //pinMode (DCC_RAIL_IN_PIN, INPUT);
    //attachInterrupt(DCC_RAIL_IN_PIN, Ext_INT1_ISR, CHANGE);

    for (uint8_t i = 0; i < MAX_NR_OF_CHANNEL;++i) {
        isrPacket[i][0] = 0;
        isrPacketRequest[i][0] = 0;
    }
    isrChannel = 0;
    isrPacket[isrChannel][0] = 2;
    isrPacket[isrChannel][1] = 0x02;
    isrPacket[isrChannel][2] = 0x04;
    isrPacket[isrChannel][3] = isrPacket[isrChannel][1]  ^ isrPacket[isrChannel][2];
    ++isrChannel;
    isrChannel = isrChannel & channelMask;
    isrPacket[isrChannel][0] = 2;
    isrPacket[isrChannel][1] = 0xff;
    isrPacket[isrChannel][2] = 0x00;
    isrPacket[isrChannel][3] = isrPacket[isrChannel][1]  ^ isrPacket[isrChannel][2];
    isrChannel = 0;

    pinMode(DCC_RAILEN_OUT_PIN, OUTPUT);
    digitalWrite(DCC_RAILEN_OUT_PIN, false);

    pinMode(DCC_RAIL1_OUT_PIN, OUTPUT);
    digitalWrite(DCC_RAIL1_OUT_PIN, false);
    pinMode(DCC_RAIL2_OUT_PIN, OUTPUT);
    digitalWrite(DCC_RAIL2_OUT_PIN, false);
#if defined DCC_4_PWM
    pinMode(DCC_RAIL3_OUT_PIN, OUTPUT);
    digitalWrite(DCC_RAIL3_OUT_PIN, false);
    pinMode(DCC_RAIL4_OUT_PIN, OUTPUT);
    digitalWrite(DCC_RAIL4_OUT_PIN, false);
#endif
    //pinMode(DCC_POCKET_OUT_PIN, OUTPUT);
    //digitalWrite(DCC_POCKET_OUT_PIN, false);

    //isrCRC1 = isrAddr1 ^ isrComm1;
    //isrCRC2 = isrAddr2 ^ isrComm2;

    // Create semaphore to inform us when the phaseTimer has fired
    timerSemaphore = xSemaphoreCreateBinary();

    // Use 1st phaseTimer of 4 (counted from zero).
    // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
    // info).

    // phaseTimer = timerBegin(0, 80, true); // old
    phaseTimer = timerBegin(1000000); // new - 1 usec resolution

    // Attach OnTimer function to our phaseTimer.
    // timerAttachInterrupt(phaseTimer, &OnTimerDCC, true); // old
    timerAttachInterrupt(phaseTimer, &OnTimerDCC); // new

    // Set alarm to call OnTimer function every second (value in microseconds).
    // Repeat the alarm (third parameter)
    // timerAlarmWrite(phaseTimer, 1000000, true); // old
    timerAlarm(phaseTimer, 1000000, true, 0); // new

    // Start an alarm
    // timerAlarmEnable(phaseTimer); // old
    timerStart(phaseTimer); // new

}

void DumpStatusDCC()
{
    // If Timer has fired
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {
        uint32_t isrCount = 0, isrTime = 0;
        int64_t  bootTime = 0;
        // Read the interrupt count and time
        portENTER_CRITICAL(&timerMux);
        isrCount = isrCounter;
        isrTime = lastIsrAt;
        bootTime = isrBootTime;
        portEXIT_CRITICAL(&timerMux);
        // Print it
        Serial.print("OnTimer no. ");
        Serial.print(isrCount);
        Serial.print(" at ");
        Serial.print(isrTime);
        Serial.print(" ms ");
        Serial.print((int32_t)bootTime);
        Serial.println(" us");
    }

}

void ReadWebCommandDCC(std::string& command)
{
    if (command.length() == 0)
        return;
    WebCommandParser parser(command);
    if (parser.GetCommandSize() > 0) {
        portENTER_CRITICAL(&timerMux);
        uint8_t chan = parser.GetChannel();
        isrPacketRequest[chan][0] = parser.GetCommandSize();
        for (uint8_t j = 1; j <= parser.GetCommandSize(); ++j) {
            isrPacketRequest[chan][j] = parser.GetCommand()[j - 1];
        }
        portEXIT_CRITICAL(&timerMux);

    }
}

void LoopDCCCommander(std::string& command)
{
    ReadWebCommandDCC(command);
    DumpStatusDCC();

}

#endif //DCC_Commander
