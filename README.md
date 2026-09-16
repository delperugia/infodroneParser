
sudo apt install libpcap-dev libtins-dev


rm -rf                build/
cmake      -B         build/ --fresh
cmake      --build    build/ -j


./build/infodroneParser tests/anafi-infodrone.pcapng
