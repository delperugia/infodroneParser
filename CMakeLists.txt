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

# Retrieving Open Drone ID Core C Library ---------------------------
FetchContent_Declare(
    opendroneid_core
    GIT_REPOSITORY https://github.com/opendroneid/opendroneid-core-c.git
    GIT_TAG        master # v2.0 won't work because its CMakeLists.txt is too old
)

set( BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE )
set( BUILD_MAVLINK     OFF CACHE BOOL "" FORCE )
set( BUILD_WIFI        OFF CACHE BOOL "" FORCE )
set( BUILD_TESTS       OFF CACHE BOOL "" FORCE )
set( BUILD_CN          OFF CACHE BOOL "" FORCE )

# opendroneid-core currently declares this target explicitly as SHARED, which
# bypasses BUILD_SHARED_LIBS.  Patch the fetched copy before adding it to this
# build so the executable links the archive instead of carrying an rpath to a
# libopendroneid.so beside it.
if( POLICY CMP0169 )
    # This is needed to modify the dependency source between population and
    # add_subdirectory().  FetchContent_MakeAvailable() adds it immediately.
    cmake_policy( SET CMP0169 OLD )
endif()
FetchContent_GetProperties( opendroneid_core )
if( NOT opendroneid_core_POPULATED )
    FetchContent_Populate( opendroneid_core )
endif()

set( opendroneid_library_cmake
     "${opendroneid_core_SOURCE_DIR}/libopendroneid/CMakeLists.txt" )
file( READ "${opendroneid_library_cmake}" opendroneid_library_cmake_contents )
string( REPLACE
    "add_library(opendroneid SHARED opendroneid.c wifi.c)"
    "add_library(opendroneid STATIC opendroneid.c wifi.c)"
    opendroneid_library_cmake_contents
    "${opendroneid_library_cmake_contents}"
)
if( NOT opendroneid_library_cmake_contents MATCHES
        "add_library\\(opendroneid STATIC opendroneid\\.c wifi\\.c\\)" )
    message( FATAL_ERROR
        "Unable to change opendroneid to a static library; its CMake target declaration changed upstream." )
endif()
file( WRITE "${opendroneid_library_cmake}" "${opendroneid_library_cmake_contents}" )

add_subdirectory( "${opendroneid_core_SOURCE_DIR}" "${opendroneid_core_BINARY_DIR}" )

#--------------------------------------------------------------------
add_executable            ( infodroneParser main.cpp parser.cpp processing.cpp     )
target_include_directories( infodroneParser PRIVATE ${opendroneid_core_SOURCE_DIR} )
target_link_libraries     ( infodroneParser PRIVATE Pcap++ opendroneid             )
