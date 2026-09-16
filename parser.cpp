#include <pcap.h>
#include <sysexits.h>
#include <tins/tins.h>

#include "parser.hpp"

void ParserBase::handleRaw( unsigned long frameNumber, const uint8_t * data, size_t length )
{
    bool   ok = false;
    Record record;
    //
    record.frameNumber = frameNumber;
    //
    try
    {
        Tins::RadioTap radiotap( data, static_cast< uint32_t >( length ) );
        //
        // In RadioTap header
        try { record.signalStrengthDbm = radiotap.dbm_signal(); } catch ( const Tins::field_not_present & ) {}
        //
        // In 802.11 frames
        auto * beacon = radiotap.find_pdu< Tins::Dot11Beacon          >();
        auto * mgmt   = radiotap.find_pdu< Tins::Dot11ManagementFrame >();
        //
        if ( beacon != nullptr && mgmt != nullptr )
        {
            // In beacon
            record.ssid = beacon->ssid();
            //
            // In management
            record.transmitterAddress = mgmt->addr2().to_string();
            //
            // Getting tags in management
            const Tins::Dot11::option * country_opt = mgmt->search_option( Tins::Dot11::COUNTRY );
            //
            if ( country_opt != nullptr && country_opt->data_size() > 2 )
                record.countryCode =
                    std::string( reinterpret_cast< const char * >( country_opt->data_ptr() ), 2 );
            //
            ok = true;
        }
    }
    catch ( const Tins::malformed_packet & )
    {
    }
    //
    if ( ! ok )
        record.errorDump = std::vector< uint8_t >( data, data + length );
    //
    m_recordHandler( record );
}

//-------------------------------------------------------------------
int ParserPcapng::parseFile( const std::string & filepath )
{
    char     errbuf[ PCAP_ERRBUF_SIZE ];
    pcap_t * handle = pcap_open_offline( filepath.c_str(), errbuf );
    if ( ! handle )
        return EX_NOINPUT;
    //
    int linkType = pcap_datalink( handle );
    if ( linkType != DLT_IEEE802_11_RADIO )
    {
        pcap_close( handle );
        return EX_DATAERR;
    }
    //
    struct pcap_pkthdr * header;
    const u_char *       data;
    int                  ret;
    long                 frameNumber = 0;
    //
    while ( ( ret = pcap_next_ex( handle, &header, &data ) ) >= 0 )
    {
        if ( ret == 0 )
            continue; // timeout, non applicable here
        //
        handleRaw(
            frameNumber,
            reinterpret_cast< const uint8_t * >( data ),
            static_cast< int >( header->caplen ) );
        //
        frameNumber++;
    }
    //
    pcap_close( handle );
    //
    if ( ret == PCAP_ERROR  )
        return EX_PROTOCOL;
    else
        return EX_OK;
}
