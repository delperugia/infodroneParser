#pragma once

#include <functional>
#include <Packet.h>
#include <string>

#include "record.hpp"

//-------------------------------------------------------------------
// Parses a raw packet, pass it to the processing
class ParserBase
{
protected:
    using RecordHandler = std::function< void( Record & ) >;
    //
    RecordHandler m_recordHandler;
    //
public:
    ParserBase( RecordHandler handler ) : m_recordHandler( handler ) {}
    virtual ~ParserBase() = default;
    //
    void handleRaw( unsigned long frameNumber, pcpp::RawPacket & rawPacket );
};

//-------------------------------------------------------------------
// PCAPNG reader (then passing parsed packets to the processing)
class ParserPcapng : public ParserBase
{
public:
    using ParserBase::ParserBase;
    //
    bool parseFile( const std::string & filepath );
};
