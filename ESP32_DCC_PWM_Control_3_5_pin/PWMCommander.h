#pragma once

#include <string>

#ifndef _PWMCOMMANDER_H_

void SetupPWMCommander();
void LoopPWMCommander(std::string& lastWebCommand);
void SetPWMCommand(bool direction, uint16_t pwmValue);
void SetPWMCommand(uint16_t pwmValue);

#endif //_PWMCOMMANDER_H_