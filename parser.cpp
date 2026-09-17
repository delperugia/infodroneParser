#include <PcapFileDevice.h>

#include <algorithm>
#include <cstddef>

#include "parser.hpp"

namespace
{

constexpr std::size_t kRadiotapFixedHeaderSize   = 8;
constexpr std::size_t kManagementHeaderSize      = 24;
constexpr std::size_t kBeaconFixedParametersSize = 12;

// Format the OEM part of a MAC address as a string
std::string formatAddress( uint8_t oui_1, uint8_t oui_2, uint8_t oui_3 )
{
    char work[20];
    snprintf( work, sizeof(work), "%02x:%02x:%02x", oui_1, oui_2, oui_3 );
    //
    return work;
}

// Reads the RadioTap layer in the record
// Returns false on error
bool readRadiotapSignal(
    const std::uint8_t * data,
    std::size_t          length,
    std::size_t &        ieee80211Offset,
    Record &             record )
{
    if ( length < kRadiotapFixedHeaderSize || data[ 0 ] != 0 ) // minimal size and radiotap version 0
        return false;
    //
    const std::size_t radiotapLength = data[ 2 ] | ( static_cast< std::uint16_t >( data[ 3 ] ) << 8 );
    if ( radiotapLength < kRadiotapFixedHeaderSize || radiotapLength > length )
        return false;
    //
    // Read present flags until the continue bit is not set
    std::size_t   presentOffset = 4;
    std::uint32_t presentFirst  = 0; // only the first flag word is used later to find Antenna (bit 5)
    do
    {
        if ( presentOffset + 4 > radiotapLength )
            return false;
        //
        const std::uint32_t currentPresent =
            ( static_cast< std::uint32_t >( data[ presentOffset     ] )       ) |
            ( static_cast< std::uint32_t >( data[ presentOffset + 1 ] ) <<  8 ) |
            ( static_cast< std::uint32_t >( data[ presentOffset + 2 ] ) << 16 ) |
            ( static_cast< std::uint32_t >( data[ presentOffset + 3 ] ) << 24 );
        //
        if ( presentOffset == 4 )           // first present flag
            presentFirst = currentPresent;
        //
        presentOffset += 4;
        if ( ( currentPresent & 0x80000000U ) == 0 ) // continue not set, no more present flags
            break;
    } while ( true );
    //
    // Loop reading IE (we just read the 5 first)
    // The dBm antenna signal field is bit 5 in the present flag:
    //   0:TSFT, 1:Flags, 2:Rate, 3:Channel, 4:FHSS, 5:dBm Antenna Signal
    constexpr std::size_t fieldAlignment[] = { 8, 1, 1, 2, 2, 1 };
    constexpr std::size_t fieldSize[]      = { 8, 1, 1, 4, 2, 1 };
    std::size_t           fieldOffset      = presentOffset;
    for ( std::size_t bit = 0; bit <= 5; ++bit )
    {
        if ( ( presentFirst & ( 1U << bit ) ) == 0 )
            continue;
        //
        fieldOffset = ( fieldOffset + fieldAlignment[ bit ] - 1 ) & ~( fieldAlignment[ bit ] - 1 );
        if ( fieldOffset + fieldSize[ bit ] > radiotapLength )
            return false;
        //
        if ( bit == 5 )
            record.signalStrengthDbm = static_cast< std::int8_t >( data[ fieldOffset ] );
        //
        fieldOffset += fieldSize[ bit ];
    }
    //
    ieee80211Offset = radiotapLength;
    return true;
}

// Reads the 802.11 Beacon/Wireless Management in the record
// Returns false on error
bool parseBeacon(
    const std::uint8_t * data,
    std::size_t          length,
    std::size_t          ieee80211Offset,
    Record &             record )
{
    if ( ( ieee80211Offset + kManagementHeaderSize + kBeaconFixedParametersSize ) > length ) // minimal size
        return false;
    //
    const     std::uint16_t frameControl      = data[ ieee80211Offset ] | ( static_cast< std::uint16_t >( data[ ieee80211Offset + 1 ] ) << 8 );
    constexpr std::uint16_t kManagementBeacon = 0x0080;
    //
    if ( ( frameControl & 0x00fc ) != kManagementBeacon )
        return false;
    //
    record.transmitterAddress = formatAddress(
        data[ ieee80211Offset + 10 ], data[ ieee80211Offset + 11 ], data[ ieee80211Offset + 12 ] ); // First 3 byes of the MAC address (the OEM)
    //
    // Loop over Tag to retrieve thoses we want
    const std::size_t managementHeaderSize = kManagementHeaderSize + ( ( frameControl & 0x8000 ) != 0 ? 4 : 0 );
    std::size_t       offset               = ieee80211Offset + managementHeaderSize + kBeaconFixedParametersSize;
    if ( offset > length )
        return false;
    //
    while ( offset + 2 <= length )
    {
        const std::uint8_t elementId      = data[ offset ];
        const std::size_t  elementLength  = data[ offset + 1 ];
        //
        offset += 2;
        if ( elementLength > length - offset )
            return false;
        //
        const std::uint8_t * element = data + offset;
        //
        if ( elementId == 0 ) // SSID
            record.ssid = std::string( reinterpret_cast< const char * >( element ), elementLength );
        else if ( elementId == 7 && elementLength >= 2 ) // Country
            record.countryCode = std::string( reinterpret_cast< const char * >( element ), 2 );
        //
        offset += elementLength;
    }
    //
    return true;
}

} // namespace

// Parses a raw packet. Extract RadioTap 802.11 elements, passes them to the processing.
void ParserBase::handleRaw( unsigned long frameNumber, pcpp::RawPacket & rawPacket )
{
    pcpp::Packet parsedPacket( &rawPacket );
    Record       record;
    //
    record.frameNumber = frameNumber;
    //
    // Radiotap are not understood by PcapPlusPlus, they appear as generic payload
    if ( parsedPacket.getFirstLayer() == nullptr ||
         parsedPacket.getFirstLayer()->getProtocol() != pcpp::GenericPayload ) // future: really identify Radiotap
        return;
    //
    const auto * data            = rawPacket.getRawData();
    const auto   length          = rawPacket.getRawDataLen();
    std::size_t  ieee80211Offset = 0; // will be set to the start of the Beacon frame
    //
    if ( ! readRadiotapSignal( data, length, ieee80211Offset, record ) ||
         ! parseBeacon( data, length, ieee80211Offset, record ) )
    {
        record.errorDump = std::vector<uint8_t>( data, data + length );
    }
    //
    m_recordHandler( record ); // and process the extracted data
}

// Opens, reads a PCAPNG file
bool ParserPcapng::parseFile( const std::string & filepath )
{
    pcpp::PcapNgFileReaderDevice reader( filepath );
    if ( ! reader.open() )
        return false;
    //
    unsigned long   frameNumber = 0;
    pcpp::RawPacket rawPacket;
    //
    while ( reader.getNextPacket( rawPacket ) )
        handleRaw( ++frameNumber, rawPacket ); // modifies rawPacket
    //
    reader.close();
    //
    return true;
}
