#pragma once

#include <functional>
#include <string>
#include <Packet.h>

#include "record.hpp"

class ParserBase
{
protected:
  using RecordHandler = std::function<void(const Record &)>;

  RecordHandler m_handler;

public:
  ParserBase(RecordHandler handler) : m_handler(handler) {}
  virtual ~ParserBase() = default;

  void handleRaw(pcpp::RawPacket rawPacket);
};

class ParserPcapng : public ParserBase
{
public:
  using ParserBase::ParserBase;

  bool parseFile(const std::string &filepath);
};
