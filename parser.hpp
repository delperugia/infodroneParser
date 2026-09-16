#pragma once

#include <functional>
#include <string>

#include "record.hpp"

//-------------------------------------------------------------------
// Parses a raw RadioTap buffer and passes the extracted record to the processing
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
    void handleRaw( unsigned long frameNumber, const uint8_t * data, size_t length );
};

//-------------------------------------------------------------------
// Reads and parse a .pcapng file, checks it is a 802.11 capture, parses
// and process all frames.
class ParserPcapng : public ParserBase
{
public:
    using ParserBase::ParserBase;
    //
    int parseFile( const std::string & filepath );
};
