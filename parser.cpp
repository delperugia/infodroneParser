#include <iostream>
#include <tins/tins.h>
#include <pcap.h>
#include <sysexits.h>

#include "parser.hpp"

void ParserBase::handleRaw( unsigned long frameNumber, const uint8_t * data, size_t length )
{
    bool   ok = false;
    Record record;
    //
    try {
        Tins::RadioTap radiotap(data, static_cast<uint32_t>(length));

        try { record.signalStrengthDbm = radiotap.dbm_signal(); } catch (const Tins::field_not_present&) {}

        Tins::Dot11ManagementFrame* mgmt = radiotap.find_pdu<Tins::Dot11ManagementFrame>();
        if ( mgmt )
        {
            record.transmitterAddress = mgmt->addr2().to_string();

            auto beacon = radiotap.find_pdu<Tins::Dot11Beacon>();
            if ( beacon )
            {
                record.ssid = beacon->ssid();

                const Tins::Dot11::option* country_opt = mgmt->search_option( Tins::Dot11::COUNTRY );

                if ( country_opt )
                    record.countryCode = std::string( reinterpret_cast<const char*>(country_opt->data_ptr()), std::min<uint32_t>(3, country_opt->data_size()) );
            }
        }

    } catch (const Tins::malformed_packet&) {
    }
    //
    if ( ! ok )
        record.errorDump = "dump todo";
    //
    m_recordHandler( record );
}

int ParserPcapng::parseFile( const std::string & filepath )
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_offline(filepath.c_str(), errbuf);
    if (!handle)
        return EX_NOINPUT;
    //
    int linktype = pcap_datalink(handle);
    if (linktype != DLT_IEEE802_11_RADIO)
    {
        pcap_close(handle);
        return EX_DATAERR;
    }
    //
    struct pcap_pkthdr* header;
    const u_char* data;
    int ret;
    long count = 0;
    //
    while ((ret = pcap_next_ex(handle, &header, &data)) >= 0)
    {
        if (ret == 0) continue; // timeout, non applicable en lecture fichier
        //
        handleRaw( count, reinterpret_cast<const uint8_t*>(data), static_cast<int>(header->caplen) );
        ++count;
    }
    //
    pcap_close(handle);
    //
    if (ret == -1)
        return EX_PROTOCOL;
    else
        return EX_OK;
}
