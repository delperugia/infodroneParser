#pragma once

#include <functional>
#include <string>

#include "record.hpp"

class ParserBase
{
protected:
    using RecordHandler = std::function< void( Record & ) >;

    RecordHandler m_recordHandler;

public:
    ParserBase( RecordHandler handler ) : m_recordHandler( handler ) {}
    virtual ~ParserBase() = default;

    void handleRaw( unsigned long frameNumber, const uint8_t * data, size_t length );
};

class ParserPcapng : public ParserBase
{
public:
    using ParserBase::ParserBase;

    int parseFile( const std::string & filepath );
};
