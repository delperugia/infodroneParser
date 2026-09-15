
.pcap library selected using Gemini

configurtion https://pcapplusplus.github.io/docs/install/linux
  time lost by the fact that PcapFileDevice.h is conditionned by Pcap++, which is counter intuitive

first example from https://github.com/seladb/PcapPlusPlus#getting-started
test on a local .pcapng file

rm -rf                build/
cmake      -B         build/
cmake      --build    build/

./build/infodroneParser tests/local.pcapng

To parse Remote ID:
  using https://github.com/opendroneid/opendroneid-core-c

  example https://github.com/sxjack/unix_rid_capture