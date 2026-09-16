#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Protocol Remote ID (ASTM F3411-22)
struct Record
{
  std::optional<std::int8_t> signalStrengthDbm;
  std::optional<std::string> ssid;
  std::optional<std::string> countryCode;
  std::vector<std::string> remoteIds;
};
