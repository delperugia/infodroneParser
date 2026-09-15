#pragma once

#include <memory>
#include "record.hpp"

class PipelineStep
{
protected:
  std::shared_ptr<PipelineStep> nextStep;

public:
  virtual ~PipelineStep() = default;

  void setNext(std::shared_ptr<PipelineStep> next) { nextStep = next; }

  virtual void process(const Record &record);
};

class DecorationStep : public PipelineStep
{
public:
  void process(const Record &record) override;
};

class DisplayStep : public PipelineStep
{
public:
  void process(const Record &record) override;
};
