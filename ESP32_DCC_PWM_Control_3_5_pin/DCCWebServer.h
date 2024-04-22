
/*
* DCCWebServer.h
*/

#pragma once

#include <Arduino.h>
#include <string>

void SetupDCCWebServer();
void LoopDCCWebServer(std::string& command);
