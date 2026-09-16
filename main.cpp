#include "parser.hpp"
#include "processing.hpp"
#include <iostream>
#include <sysexits.h>

int main( int argc, char * argv[] )
{
    if ( argc != 2 )
    {
        std::cerr << "Usage: " << argv[ 0 ] << " <pcap-file>" << std::endl;
        return EX_USAGE;
    }
    //
    auto decoration = std::make_shared< DecorationStep >();
    auto display    = std::make_shared< DisplayStep >();
    decoration->setNext( display );
    //
    ParserPcapng parser(
        [ decoration ]( Record & record ) { decoration->process( record ); } );
    //
    int result = parser.parseFile( argv[ 1 ] );
    //
    switch( result )
    {
        case EX_NOINPUT:    std::cerr << "Erreur ouverture fichier"     << std::endl; break;
        case EX_DATAERR:    std::cerr << "Fichier sans frames RadioTap" << std::endl; break;
        case EX_PROTOCOL:   std::cerr << "Erreur de lecture"            << std::endl; break;
        case EX_OK:         std::cout << "Ok"                           << std::endl; break;
        default:            std::cout << "Unknown"                      << std::endl; break;
    }
    /*
            std::cerr << "Erreur ouverture fichier: " << errbuf << "\n";
std::cerr << "Fichier sans frames RadioTap (linktype=" << linktype
                  << ", attendu DLT_IEEE802_11_RADIO=" << DLT_IEEE802_11_RADIO << ")\n";
    std::cerr << "Erreur de lecture: " << pcap_geterr(handle) << "\n";
    */
    //
    exit( result );
    return EX_OK;
}
