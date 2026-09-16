cmake_minimum_required( VERSION 3.15 )
project(
    infodroneParser
    DESCRIPTION "Extract InfoDrone packets from .pcap files"
    LANGUAGES   CXX
)

set( CMAKE_CXX_STANDARD          20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

find_package( PkgConfig REQUIRED )

# --- libpcap ---
find_library( PCAP_LIBRARY     NAMES pcap   )
find_path   ( PCAP_INCLUDE_DIR NAMES pcap.h )

if ( NOT PCAP_LIBRARY OR NOT PCAP_INCLUDE_DIR )
    message( FATAL_ERROR "libpcap introuvable (paquet libpcap-dev)" )
endif()

# --- libtins ---
pkg_check_modules(TINS libtins)
if ( NOT TINS_FOUND )
    find_library( TINS_LIBRARY     NAMES tins        )
    find_path   ( TINS_INCLUDE_DIR NAMES tins/tins.h )
    if ( NOT TINS_LIBRARY OR NOT TINS_INCLUDE_DIR )
        message( FATAL_ERROR "libtins introuvable" )
    endif()
    set( TINS_LIBRARIES    ${TINS_LIBRARY}     )
    set( TINS_INCLUDE_DIRS ${TINS_INCLUDE_DIR} )
endif()

#--------------------------------------------------------------------
add_executable            ( infodroneParser main.cpp parser.cpp processing.cpp     )
target_include_directories( infodroneParser PRIVATE
    ${PCAP_INCLUDE_DIR}
    ${TINS_INCLUDE_DIRS}
)
target_link_libraries     ( infodroneParser PRIVATE
    ${PCAP_LIBRARY}
    ${TINS_LIBRARIES}
)

target_compile_options    ( infodroneParser PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wformat=2
        -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow
        -Wswitch -Wswitch-default -Wundef -Werror
        -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fvisibility=hidden
    >
)
