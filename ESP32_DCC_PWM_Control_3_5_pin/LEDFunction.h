#pragma once

#ifndef _LEDFUNCTION_H_
#include "Arduino.h"
#include <string>

void SetupLEDFunction();
void LoopLEDFunction();

void SendMorze(const std::string& str);


#endif //_LEDFUNCTION_H_