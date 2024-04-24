

#define _DCCSimpleWebServer_ 1

#define version "1.02"

#include "ESP32_DCC_PWM_Control_3_5_pin.h"

#if defined DCC_Commander
#include "DCCCommander.h"
#else
#include "PWMCommander.h"
#endif
#include "DCCWebServer.h"

void setup () 
{
	Serial.begin(115200);

#if defined DCC_Commander
	SetupDCCCommander ();
	Serial.print("ESP32_DCC_PWM_Control_3_5_pin /DCC  Version ");
#else
	SetupPWMCommander();
	Serial.print("ESP32_DCC_PWM_Control_3_5_pin /PWM  Version ");
#endif
	Serial.println(version);

	SetupDCCWebServer();
}

void loop ()
{
	std::string lastWebCommand;
	LoopDCCWebServer(lastWebCommand);
#if defined DCC_Commander
	LoopDCCCommander (lastWebCommand);
#else
	LoopPWMCommander (lastWebCommand);
#endif

}
