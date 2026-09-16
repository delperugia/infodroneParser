#include <iostream>
#include <sysexits.h>

#include "parser.hpp"
#include "processing.hpp"

int main( int argc, char * argv[] )
{
    if ( argc != 2 )
    {
        std::cerr << "Usage: " << argv[ 0 ] << " <pcap-file>" << std::endl;
        return EX_USAGE;
    }
    //
    // Build the processing layer
    auto decoration = std::make_shared< DecorationStep >();
    auto display    = std::make_shared< DisplayStep >();
    decoration->setNext( display );
    //
    // The parser, linked to the processing
    ParserPcapng parser(
        [ decoration ]( Record & record ) { decoration->process( record ); } );
    //
    // Process
    int result = parser.parseFile( argv[ 1 ] );
    //
    switch( result )
    {
        case EX_NOINPUT:  std::cerr << "Error opening the file"                    << std::endl; break;
        case EX_DATAERR:  std::cerr << "PCAPNG file is not a 802.11 radio capture" << std::endl; break;
        case EX_PROTOCOL: std::cerr << "Error processing file"                     << std::endl; break;
        case EX_OK:                                                                              break;
        default:          std::cout << "Unknown result"                            << std::endl; break;
    }
    //
    exit( result );
    return EX_OK;
}
