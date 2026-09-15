#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Protocol Remote ID (ASTM F3411-22)
struct Record
{
  struct BasicId
  {
    uint8_t idType;
    uint8_t uasType;
    std::string uasId;
  };

  struct Location
  {
    uint8_t status;
    double directionDegrees;
    double horizontalSpeedMps;
    double verticalSpeedMps;
    double latitude;
    double longitude;
    double barometricAltitudeM;
    double geometricAltitudeM;
    uint8_t heightReference;
    double heightM;
    uint8_t horizontalAccuracy;
    uint8_t verticalAccuracy;
    uint8_t barometricAccuracy;
    uint8_t speedAccuracy;
    uint8_t timestampAccuracy;
    double timestampSecondsAfterHour;
  };

  struct Authentication
  {
    uint8_t type;
    uint8_t page;
    std::optional<uint8_t> lastPage;
    std::optional<uint8_t> length;
    std::optional<uint32_t> timestampSecondsSince2019;
    std::vector<uint8_t> data;
  };

  struct SelfId
  {
    uint8_t descriptionType;
    std::string description;
  };

  struct System
  {
    uint8_t operatorLocationType;
    uint8_t classificationType;
    double operatorLatitude;
    double operatorLongitude;
    uint16_t areaCount;
    uint16_t areaRadiusM;
    double areaCeilingM;
    double areaFloorM;
    uint8_t euCategory;
    uint8_t euClass;
    double operatorGeometricAltitudeM;
    uint32_t timestampSecondsSince2019;
  };

  struct OperatorId
  {
    uint8_t type;
    std::string id;
  };

  std::array<uint8_t, 6>      sourceMac{};
  uint8_t                     messageCounter{};
  uint8_t                     protocolVersion{};
  std::vector<BasicId>        basicIds;
  std::optional<Location>     location;
  std::vector<Authentication> authentications;
  std::optional<SelfId>       selfId;
  std::optional<System>       system;
  std::optional<OperatorId>   operatorId;
};
