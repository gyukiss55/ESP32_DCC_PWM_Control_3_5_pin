

#define _DCCSimpleWebServer_ 1

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
#else
	SetupPWMCommander();
#endif

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
