#include <Packet.h>
#include <PcapFileDevice.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

#include "parser.hpp"

namespace {

constexpr size_t kRadiotapMinimumLength = 8;
constexpr size_t kManagementHeaderLength = 24;
constexpr size_t kBeaconFixedParametersLength = 12;
constexpr size_t kRemoteIdMessageLength = 25;

uint16_t readLe16(const uint8_t* data)
{
  return static_cast<uint16_t>(data[0]) |
         (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t readLe32(const uint8_t* data)
{
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

int32_t readLeI32(const uint8_t* data)
{
  return static_cast<int32_t>(readLe32(data));
}

std::string readText(const uint8_t* data, size_t length)
{
  const auto end = std::find(data, data + length, uint8_t{0});
  return {reinterpret_cast<const char*>(data), static_cast<size_t>(end - data)};
}

double decodeAltitude(uint16_t value)
{
  return value * 0.5 - 1000.0;
}

void decodeMessage(Record& record, const uint8_t* message)
{
  record.protocolVersion = message[0] & 0x0f;

  switch (message[0] >> 4)
  {
  case 0: // Basic ID
    record.basicIds.push_back({static_cast<uint8_t>(message[1] >> 4),
                               static_cast<uint8_t>(message[1] & 0x0f),
                               readText(message + 2, 20)});
    break;

  case 1: // Location/Vector
  {
    const uint8_t flags = message[1];
    const uint8_t encodedDirection = message[2];
    const uint8_t encodedSpeed = message[3];
    const bool eastWestDirection = (flags & 0x02) != 0;
    const bool speedMultiplier = (flags & 0x01) != 0;

    Record::Location location{
      static_cast<uint8_t>(flags >> 4),
      encodedDirection == 255 ? 361.0 : encodedDirection + (eastWestDirection ? 180.0 : 0.0),
      speedMultiplier ? 63.75 + encodedSpeed * 0.75 : encodedSpeed * 0.25,
      static_cast<int8_t>(message[4]) * 0.5,
      readLeI32(message + 5) * 1e-7,
      readLeI32(message + 9) * 1e-7,
      decodeAltitude(readLe16(message + 13)),
      decodeAltitude(readLe16(message + 15)),
      static_cast<uint8_t>((flags >> 2) & 0x01),
      decodeAltitude(readLe16(message + 17)),
      static_cast<uint8_t>(message[19] & 0x0f),
      static_cast<uint8_t>(message[19] >> 4),
      static_cast<uint8_t>(message[20] >> 4),
      static_cast<uint8_t>(message[20] & 0x0f),
      static_cast<uint8_t>(message[23] & 0x0f),
      readLe16(message + 21) * 0.1,
    };
    record.location = location;
    break;
  }

  case 2: // Authentication
  {
    Record::Authentication authentication{
      static_cast<uint8_t>(message[1] >> 4), static_cast<uint8_t>(message[1] & 0x0f),
      std::nullopt, std::nullopt, std::nullopt, {}};
    if (authentication.page == 0)
    {
      authentication.lastPage = message[2];
      authentication.length = message[3];
      authentication.timestampSecondsSince2019 = readLe32(message + 4);
      authentication.data.assign(message + 8, message + kRemoteIdMessageLength);
    }
    else
      authentication.data.assign(message + 2, message + kRemoteIdMessageLength);
    record.authentications.push_back(std::move(authentication));
    break;
  }

  case 3: // Self ID
    record.selfId = Record::SelfId{message[1], readText(message + 2, 23)};
    break;

  case 4: // System
    record.system = Record::System{
      static_cast<uint8_t>(message[1] & 0x03),
      static_cast<uint8_t>((message[1] >> 2) & 0x07),
      readLeI32(message + 2) * 1e-7, readLeI32(message + 6) * 1e-7,
      readLe16(message + 10), static_cast<uint16_t>(message[12] * 10),
      decodeAltitude(readLe16(message + 13)), decodeAltitude(readLe16(message + 15)),
      static_cast<uint8_t>(message[17] >> 4), static_cast<uint8_t>(message[17] & 0x0f),
      decodeAltitude(readLe16(message + 18)), readLe32(message + 20)};
    break;

  case 5: // Operator ID
    record.operatorId = Record::OperatorId{message[1], readText(message + 2, 20)};
    break;
  }
}

bool decodeRemoteIdBeacon(const pcpp::Packet& parsedPacket, Record& record)
{
  const auto* rawPacket = parsedPacket.getRawPacket();
  const auto* data = rawPacket->getRawData();
  const size_t length = rawPacket->getRawDataLen();
  if (length < kRadiotapMinimumLength)
    return false;

  const size_t radiotapLength = readLe16(data + 2);
  if (radiotapLength < kRadiotapMinimumLength || radiotapLength + kManagementHeaderLength +
      kBeaconFixedParametersLength > length)
    return false;

  const uint8_t* frame = data + radiotapLength;
  const size_t frameLength = length - radiotapLength;
  if ((frame[0] & 0x0c) != 0 || (frame[0] >> 4) != 8) // management Beacon
    return false;

  std::memcpy(record.sourceMac.data(), frame + 10, record.sourceMac.size());
  size_t offset = kManagementHeaderLength + kBeaconFixedParametersLength;
  bool found = false;
  while (offset + 2 <= frameLength)
  {
    const uint8_t elementId = frame[offset];
    const size_t elementLength = frame[offset + 1];
    offset += 2;
    if (offset + elementLength > frameLength)
      return false;

    const uint8_t* element = frame + offset;
    offset += elementLength;
    if (elementId != 221 || elementLength < 5)
      continue;

    // Pre-ASTM Parrot Drone ID: OUI 90:03:B7, type 09. Its payload only
    // carries the Basic ID's type byte followed by the UAS ID.
    if (element[0] == 0x90 && element[1] == 0x03 && element[2] == 0xb7 && element[3] == 0x09)
    {
      record.basicIds.push_back({static_cast<uint8_t>(element[4] >> 4),
                                 static_cast<uint8_t>(element[4] & 0x0f),
                                 readText(element + 5, elementLength - 5)});
      found = true;
      continue;
    }

    if (elementLength < 8 || element[0] != 0xfa || element[1] != 0x0b ||
        element[2] != 0xbc || element[3] != 0x0d)
      continue;

    record.messageCounter = element[4];
    const uint8_t* pack = element + 5;
    const size_t packLength = elementLength - 5;
    if (packLength < 3 || (pack[0] >> 4) != 15 || pack[1] != kRemoteIdMessageLength ||
        pack[2] == 0 || pack[2] > 9 || 3 + pack[2] * kRemoteIdMessageLength > packLength)
      continue;

    for (size_t i = 0; i < pack[2]; ++i)
      decodeMessage(record, pack + 3 + i * kRemoteIdMessageLength);
    found = true;
  }
  return found;
}

} // namespace

// pcpp::Packet parsing alters rawPacket: we could pass here a reference to the
// raw packet to avoid a copy, but this would introduce potential side effect.
void ParserBase::handleRaw( pcpp::RawPacket rawPacket)
{
  pcpp::Packet parsedPacket(&rawPacket);
  Record       record;

  if (decodeRemoteIdBeacon(parsedPacket, record))
    m_handler(record);
  else
    ; // future
}

bool ParserPcapng::parseFile(const std::string &filepath)
{
  pcpp::PcapNgFileReaderDevice reader(filepath);
  if (!reader.open())
    return false;

  pcpp::RawPacket rawPacket;

  while (reader.getNextPacket(rawPacket))
    handleRaw(rawPacket);
  //
  reader.close();
  //
  return true;
}
