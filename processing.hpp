#pragma once

#include "record.hpp"
#include <memory>

//-------------------------------------------------------------------
// A step in the processing chain of responsibility
class PipelineStep
{
protected:
    std::shared_ptr< PipelineStep > nextStep;
    //
public:
    virtual ~PipelineStep() = default;
    //
    void setNext( std::shared_ptr< PipelineStep > next ) { nextStep = next; }
    //
    virtual void process( Record & record ) = 0;
};

//-------------------------------------------------------------------
// A decoration step, possibly altering the record to add information
class DecorationStep : public PipelineStep
{
public:
    void process( Record & record ) override;
};

//-------------------------------------------------------------------
// A console display step
class DisplayStep : public PipelineStep
{
public:
    void process( Record & record ) override;
};
