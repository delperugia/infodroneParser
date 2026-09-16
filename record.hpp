#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Protocol Remote ID (ASTM F3411-22)
struct Record
{
    unsigned long                frameNumber;

    std::optional< std::string > ssid;
    std::optional< std::int8_t > signalStrengthDbm;
    std::optional< std::string > transmitterAddress;
    std::optional< std::string > countryCode;

    std::optional< std::string > errorDump;  // if a parsing error occurred, contains the frame dump; other fields may or not be set
};
