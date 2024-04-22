// DCCWebCommandParser.h

#include "Arduino.h"
#include <string>

#define MAX_NR_OF_HEXA_BYTES 6

class WebCommandParser {
private:
    uint8_t result[MAX_NR_OF_HEXA_BYTES];
    uint8_t result_nr = 0;
    uint8_t chan = 0;

public:
    WebCommandParser();
    WebCommandParser(const std::string& command);
    uint8_t GetChannel() const { return chan; }
    uint8_t GetCommandSize() const { return result_nr; }
    bool GetDirectionAndSpeed(bool& forward, uint8_t& speed) const;
    const uint8_t* GetCommand() const { return result; }
    bool IsAlertStop() const;
    bool IsConfigFirstStep(uint8_t& firstStep) const;

private:
    int ParseWebCommand(const std::string& command);
};
