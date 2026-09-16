#include <iostream>

#include "processing.hpp"

//-------------------------------------------------------------------
void PipelineStep::process( Record & record )
{
    if ( nextStep )
        nextStep->process( record );
}

//-------------------------------------------------------------------
// Replace manufacturers MAC OEM prefix by their name
void DecorationStep::process( Record & record )
{
    if ( record.transmitterAddress.has_value() )
    {
        if ( record.transmitterAddress.value().starts_with( "90:3a:e6:" ) )
            record.transmitterAddress = "Parrot_" + record.transmitterAddress.value().substr( 9 );
    }
    //
    PipelineStep::process( record );
}

//-------------------------------------------------------------------
void DisplayStep::process( Record & record )
{
    std::cout << ( record.errorDump.has_value() ? "E" : "#" ) << record.frameNumber << ": ";
    //
    std::cout << ( record.ssid.has_value()               ? record.ssid              .value() : "?"  ) << " ";
    std::cout << ( record.countryCode.has_value()        ? record.countryCode       .value() : "?"  ) << " ";
    std::cout << ( record.transmitterAddress.has_value() ? record.transmitterAddress.value() : "?"  ) << " ";
    std::cout << ( record.signalStrengthDbm.has_value()  ? record.signalStrengthDbm .value() : -128 ) << "dBm ";
    //
    std::cout << "\n";
    //
    PipelineStep::process( record );
}
