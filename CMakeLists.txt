cmake_minimum_required( VERSION 3.15 )
project(
    infodroneParser
    DESCRIPTION "Extract InfoDrone packets from .pcap files"
    LANGUAGES   CXX
)

set( CMAKE_CXX_STANDARD          17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

include( FetchContent )

# Retrieving PcapPlusPlus -------------------------------------------
FetchContent_Declare(
    PcapPlusPlus
    GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
    GIT_TAG        v26.07
)

# See https://pcapplusplus.github.io/docs/install/linux
set( PCAPPP_USE_PCAP        OFF CACHE BOOL "" FORCE )
set( PCAPPP_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE )
set( PCAPPP_BUILD_TESTS     OFF CACHE BOOL "" FORCE )
set( PCAPPP_BUILD_TUTORIALS OFF CACHE BOOL "" FORCE )

FetchContent_MakeAvailable( PcapPlusPlus )

#--------------------------------------------------------------------
add_executable       ( infodroneParser main.cpp parser.cpp processing.cpp )
target_link_libraries( infodroneParser PUBLIC Pcap++ )
