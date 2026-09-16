
# Prerequisite

Development was done on Ubuntu 26.04.

    sudo apt install cmake g++ libpcap-dev libtins-dev libbsd-dev

# Compilation

    cmake -B      build/ --fresh
    cmake --build build/ -j

# Tests

    ./build/infodroneParser tests/anafi-infodrone.pcapng

or

    cd tests/
    ./tests.sh

# Package

## Building the .deb package

    cmake --build build/ --target package

## Installing the .deb package

Tested on Ubuntu 26.04 and Debian Trixie

    sudo apt update
    sudo apt install ./infodroneparser_1.0.0-1_amd64.deb
