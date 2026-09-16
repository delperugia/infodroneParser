#include <Packet.h>
#include <PcapFileDevice.h>
#include <libopendroneid/opendroneid.h>

#include <algorithm>

#include "parser.hpp"

namespace
{
constexpr std::size_t kRadiotapFixedHeaderSize = 8;
constexpr std::size_t kManagementHeaderSize = 24;
constexpr std::size_t kBeaconFixedParametersSize = 12;

bool readRadiotapSignal(const std::uint8_t *data, std::size_t length,
                        std::size_t &ieee80211Offset, Record &record)
{
  if (length < kRadiotapFixedHeaderSize || data[0] != 0)
    return false;

  const std::size_t radiotapLength = data[2] | (static_cast<std::size_t>(data[3]) << 8);
  if (radiotapLength < kRadiotapFixedHeaderSize || radiotapLength > length)
    return false;

  std::size_t presentOffset = 4;
  std::uint32_t present = 0;
  do
  {
    if (presentOffset + 4 > radiotapLength)
      return false;

    const std::uint32_t currentPresent = static_cast<std::uint32_t>(data[presentOffset]) |
                                         (static_cast<std::uint32_t>(data[presentOffset + 1]) << 8) |
                                         (static_cast<std::uint32_t>(data[presentOffset + 2]) << 16) |
                                         (static_cast<std::uint32_t>(data[presentOffset + 3]) << 24);
    if (presentOffset == 4)
      present = currentPresent;
    presentOffset += 4;
    if ((currentPresent & 0x80000000U) == 0)
      break;
  } while (true);

  // The dBm antenna signal field is bit 5. Its preceding standard fields are
  // sufficient to locate it; later radiotap fields are not needed here.
  constexpr std::size_t fieldAlignment[] = {8, 1, 1, 2, 2, 1};
  constexpr std::size_t fieldSize[] = {8, 1, 1, 4, 2, 1};
  std::size_t fieldOffset = presentOffset;
  for (std::size_t bit = 0; bit <= 5; ++bit)
  {
    if ((present & (1U << bit)) == 0)
      continue;

    fieldOffset = (fieldOffset + fieldAlignment[bit] - 1) & ~(fieldAlignment[bit] - 1);
    if (fieldOffset + fieldSize[bit] > radiotapLength)
      return false;

    if (bit == 5)
      record.signalStrengthDbm = static_cast<std::int8_t>(data[fieldOffset]);
    fieldOffset += fieldSize[bit];
  }

  ieee80211Offset = radiotapLength;
  return true;
}

void decodeRemoteId(const std::uint8_t *data, std::size_t length, Record &record)
{
  ODID_UAS_Data uasData;
  odid_initUasData(&uasData);
  if (odid_message_process_pack(&uasData, data, length) < 0)
    return;

  for (std::size_t i = 0; i < ODID_BASIC_ID_MAX_MESSAGES; ++i)
  {
    if (uasData.BasicIDValid[i] == 0)
      continue;

    const char *id = uasData.BasicID[i].UASID;
    const auto *end = std::find(id, id + ODID_ID_SIZE, '\0');
    record.remoteIds.emplace_back(id, end);
  }
}

void parseBeacon(const std::uint8_t *data, std::size_t length,
                 std::size_t ieee80211Offset, Record &record)
{
  if (ieee80211Offset + kManagementHeaderSize + kBeaconFixedParametersSize > length)
    return;

  const std::uint16_t frameControl = data[ieee80211Offset] |
                                     (static_cast<std::uint16_t>(data[ieee80211Offset + 1]) << 8);
  constexpr std::uint16_t kManagementBeacon = 0x0080;
  if ((frameControl & 0x00fc) != kManagementBeacon)
    return;

  const std::size_t managementHeaderSize = kManagementHeaderSize +
                                           ((frameControl & 0x8000) != 0 ? 4 : 0);
  std::size_t offset = ieee80211Offset + managementHeaderSize + kBeaconFixedParametersSize;
  if (offset > length)
    return;

  while (offset + 2 <= length)
  {
    const std::uint8_t elementId = data[offset];
    const std::size_t elementLength = data[offset + 1];
    offset += 2;
    if (elementLength > length - offset)
      return;

    const std::uint8_t *element = data + offset;
    if (elementId == 0) // SSID
      record.ssid = std::string(reinterpret_cast<const char *>(element), elementLength);
    else if (elementId == 7 && elementLength >= 2) // Country
      record.countryCode = std::string(reinterpret_cast<const char *>(element), 2);
    else if (elementId == 221 && elementLength >= 5 &&
             element[0] == 0xfa && element[1] == 0x0b && element[2] == 0xbc && element[3] == 0x0d)
      decodeRemoteId(element + 5, elementLength - 5, record);

    offset += elementLength;
  }
}
} // namespace

// pcpp::Packet parsing alters rawPacket: we could pass here a reference to the
// raw packet to avoid a copy, but this would introduce potential side effect.
void ParserBase::handleRaw( pcpp::RawPacket rawPacket)
{
  pcpp::Packet parsedPacket(&rawPacket);
  Record       record;

  // PcapPlusPlus currently exposes 802.11/radiotap captures as raw data.
  // Obtain that data through the parsed packet so it remains the packet source.
  const auto *packet = parsedPacket.getRawPacket();
  const auto *data = packet->getRawData();
  const auto length = static_cast<std::size_t>(packet->getRawDataLen());
  std::size_t ieee80211Offset = 0;
  if (readRadiotapSignal(data, length, ieee80211Offset, record))
    parseBeacon(data, length, ieee80211Offset, record);

  m_handler(record);
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
