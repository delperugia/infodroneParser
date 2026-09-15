#include <iostream>
#include "parser.hpp"
#include "processing.hpp"

int main(int argc, char *argv[])
{
  // todo check that there is one argument, display syntex and exit if not

  auto decoration = std::make_shared<DecorationStep>();
  auto display    = std::make_shared<DisplayStep   >();
  decoration->setNext(display);

  ParserPcapng parser([decoration](const Record &record)
                      { decoration->process(record); });

  if (!parser.parseFile(argv[1]))
  {
    std::cerr << "Error opening the pcap file" << std::endl;
    return 1;
  }

  return 0;
}
