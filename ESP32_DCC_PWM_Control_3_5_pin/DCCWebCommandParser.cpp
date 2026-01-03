// DCCWebCommandParser.cpp

#include "Arduino.h"

#include <string>

#include "DCCWebCommandParser.h"



inline bool IsHexa(char ch) {
    if (ch >= '0' && ch <= '9' ||
        ch >= 'A' && ch <= 'F' ||
        ch >= 'a' && ch <= 'f')
        return true;
    return false;
}

inline uint8_t HexaCharToValue(char ch) {
    if (ch >= '0' && ch <= '9')
        return (ch - '0');
    if (ch >= 'A' && ch <= 'F')
        return (ch - 'A' + 10);
    if (ch >= 'a' && ch <= 'f')
        return (ch - 'a' + 10);
    return 0xff;
}

bool StringToArrayOfBytes(const std::string& str, size_t pos, uint8_t* bytes, uint8_t& numberOfBytes)
{
    bool ret = false;
    uint8_t tmp = 0;
    numberOfBytes = 0;
    for (size_t i = pos; i < str.size() && IsHexa(str[i]); i += 2) {
        tmp = HexaCharToValue(str[i]);
        bool breakFlag = true;
        if ((i + 1) < str.size() && IsHexa(str[i + 1])) {
            tmp = HexaCharToValue(str[i + 1]) + (tmp << 4);
            breakFlag = false;
        }
        bytes[numberOfBytes] = tmp;
        numberOfBytes++;
        ret = true;
        if (breakFlag || numberOfBytes == MAX_NR_OF_HEXA_BYTES)
            break;
    }
    return ret;
}

int WebCommandParser::ParseWebCommand(const std::string& command)
{
    result_nr = 0;
    chan = 0;

    StringToArrayOfBytes(command, command.find("ch=") + 3, result, result_nr);
    chan = result[0];
    StringToArrayOfBytes(command, command.find("dcc=") + 4, result, result_nr);
    Serial.print("result_nr:");
    Serial.print(result_nr);
    Serial.print("result:");
    Serial.print(result[0], HEX);
    Serial.print(",");
    Serial.print(result[1], HEX);
    if (chan < 4 && result_nr >= 1 && result_nr <= 5) {
        Serial.print("ch:");
        Serial.print(chan, HEX);
        for (uint8_t j = 0; j < result_nr; ++j) {
            Serial.print(",");
            Serial.print(result[j], HEX);
        }
    }
    else {
        Serial.print("Incorrect DCC command!");
        result_nr = 0;
    }
    Serial.println(".");
    return result_nr;
}


WebCommandParser::WebCommandParser() : result_nr(0), chan(0)
{

}

WebCommandParser::WebCommandParser(const std::string & command) : result_nr(0), chan(0)
{
    if (command.length() > 0)
        ParseWebCommand(command);
}

bool WebCommandParser::GetDirectionAndSpeed(bool& forward, uint8_t& speed) const
{
    forward = true;
    speed = 0;
    uint8_t value = result[1];
    if ((result_nr == 2) && ((value & 0xC0) == 0x40)) {
        if ((value & 0x20) == 0)
            forward = false;
        if ((value & 0x0f) > 2) {
            speed = (value & 0x0f) * 2;
            if (value & 0x10)
                speed++;
        }
        return true;
    }
    return false;
}

bool WebCommandParser::IsAlertStop() const
{
    if ((result_nr == 2) && ((result[1] & 0xCF) == 0x41))
        return true;
    return false;
}

bool WebCommandParser::IsConfigFirstStep(uint8_t& firstStep) const
{
    firstStep = 0;
    if ((result_nr == 2) && (result[0] == 55) && ((result[1] & 0xF0) == 0xE0)) {
        firstStep = result[1] & 0xF;
        return true;
    }
    return false;
}

