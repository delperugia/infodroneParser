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
    auto decoration = std::make_shared< DecorationStep >();
    auto display    = std::make_shared< DisplayStep >();
    decoration->setNext( display );
    //
    ParserPcapng parser(
        [ decoration ]( Record & record ) { decoration->process( record ); } );
    //
    if ( ! parser.parseFile( argv[ 1 ] ) )
    {
        std::cerr << "Error opening the pcap file" << std::endl;
        return EX_NOINPUT;
    }
    //
    return EX_OK;
}
