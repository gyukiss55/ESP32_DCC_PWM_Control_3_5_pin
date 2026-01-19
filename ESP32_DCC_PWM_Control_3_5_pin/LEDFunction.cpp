// LEDFunction.cpp




#include "Arduino.h"

#include "LEDFunction.h"

#include <string>
#include <vector>
// 4+1 bit control

#define ONBOARD_LED_PIN  2
#define LED_PERIOD 200

struct MorzeMap {
	char letter;
	const char* signMorze;
};

MorzeMap MorzeABC[] = {
	'0', "01111",
	'1', "00111",
	'2', "00011",
	'3', "00001",
	'4', "00000",
	'5', "10000",
	'6', "11000",
	'7', "11100",
	'8', "11110",
	'9', "11111",
	'A', "01",
	'B', "1000",
	'C', "1010",
	'D', "100",
	'E', "0",
	'F', "0010",
	'G', "110",
	'H', "0000",
	'I', "00",
	'J', "0111",
	'K', "101",
	'L', "0100",
	'M', "11",
	'N', "10",
	'O', "111",
	'P', "0110",
	'Q', "1101",
	'R', "010",
	'S', "000",
	'T', "1",
	'U', "001",
	'V', "0001",
	'W', "011",
	'X', "1001",
	'Y', "1011",
	'Z', "1100"
};

static std::vector<int> currentLedCommand;
static int currentLedCommandIndex = 0;
static int64_t nextTimeStamp = 0;
static bool ledState = false;


void SendMorze(const std::string& str)
{
	if (!ledState)
		return;

	std::vector<int> ledCommand;

	for (char c : str) {
		int index = -1;
		if (c >= '0' && c <= '9') {
			index = c - '0';
		}
		if (c >= 'A' && c <= 'Z') {
			index = c - 'A' + 10;
		}
		if (c >= 'a' && c <= 'z') {
			index = c - 'a' + 10;
		}
		if (index >= 0) {
			std::string m = MorzeABC[index].signMorze;
			for (char c2 : m) {
				if (c2 == '0') {
					ledCommand.push_back(1);
					ledCommand.push_back(1);
					ledCommand.push_back(0);
					ledCommand.push_back(3);
				}
				else {
					ledCommand.push_back(1);
					ledCommand.push_back(4);
					ledCommand.push_back(0);
					ledCommand.push_back(4);
				}
			}
			ledCommand.push_back(0);
			ledCommand.push_back(8);

		}
		else {
			ledCommand.push_back(0);
			ledCommand.push_back(16);
		}
	}
	ledCommand.push_back(0);
	ledCommand.push_back(32);
	currentLedCommand = ledCommand;
	currentLedCommandIndex = 0;
	nextTimeStamp = 0;
}

void SetupLEDFunction()
{
	Serial.println("SetupLEDFunction");
	pinMode(ONBOARD_LED_PIN, OUTPUT);
	digitalWrite(ONBOARD_LED_PIN, LOW);
	ledState = true;
}


void LoopLEDFunction()
{
	if (!ledState)
		return;
	int64_t ts = esp_timer_get_time();
	if (nextTimeStamp < ts) {
		if (currentLedCommandIndex < currentLedCommand.size()) {
			int stateLed = currentLedCommand[currentLedCommandIndex];
			int duration = currentLedCommand[currentLedCommandIndex + 1];
			currentLedCommandIndex += 2;
			if (currentLedCommandIndex >= currentLedCommand.size())
				currentLedCommandIndex = 0;
			if (stateLed)
				digitalWrite(ONBOARD_LED_PIN, HIGH);
			else
				digitalWrite(ONBOARD_LED_PIN, LOW);
			nextTimeStamp = ts + (int64_t)duration * LED_PERIOD * 1000LL;
			//Serial.print("LED i:");
			//Serial.print(currentLedCommandIndex);
			//Serial.print(" s:");
			//Serial.print(stateLed);
			//Serial.print(" d:");
			//Serial.print(duration);
			//Serial.print(" t:");
			//Serial.print(ts);
			//Serial.print(" tn:");
			//Serial.println(nextTimeStamp);
		}
	}
}
