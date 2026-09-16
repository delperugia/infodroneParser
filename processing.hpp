#pragma once

#include "record.hpp"
#include <memory>

class PipelineStep
{
protected:
    std::shared_ptr< PipelineStep > nextStep;

public:
    virtual ~PipelineStep() = default;

    void setNext( std::shared_ptr< PipelineStep > next ) { nextStep = next; }

    virtual void process( Record & record );
};

class DecorationStep : public PipelineStep
{
public:
    void process( Record & record ) override;
};

class DisplayStep : public PipelineStep
{
public:
    void process( Record & record ) override;
};
