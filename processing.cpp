#include <iostream>

#include "processing.hpp"

void PipelineStep::process(const Record &record)
{
  if (nextStep)
    nextStep->process(record);
}

void DecorationStep::process(const Record &record)
{
  // if (record.data.find("1") != std::string::npos)
  {
    PipelineStep::process(record);
  }
}

void DisplayStep::process(const Record &record)
{
  std::cout << "[Display] " << record.ssid.value() << " \n";
  PipelineStep::process(record);
}
