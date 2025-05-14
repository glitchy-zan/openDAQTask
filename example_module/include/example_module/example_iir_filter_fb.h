#pragma once
#include "common.h"
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>
#include <opendaq/stream_reader_ptr.h>

BEGIN_NAMESPACE_EXAMPLE_MODULE

class ExampleFBImplIIR final : public FunctionBlock
{
public:
    explicit ExampleFBImplIIR(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~ExampleFBImplIIR() override = default;

    static FunctionBlockTypePtr CreateType();

private:
    InputPortPtr inputPort;

    DataDescriptorPtr inputDataDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;

    DataDescriptorPtr outputDataDescriptor;
    DataDescriptorPtr outputDomainDataDescriptor;

    SampleType inputSampleType;

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    StreamReaderPtr reader;
    SizeT sampleRate;
    std::vector<float> inputData;
    std::vector<uint64_t> inputDomainData;

    Float cutoffFrequency = 100.00;
    Float outputHighValue;
    Float outputLowValue;
    Bool useCustomOutputRange;
    std::string outputUnit;
    std::string outputName;
    bool configValid = false;

    // Filter coefficients
    double b0 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;

    // Filter state variables
    struct FilterState
    {
        double x_prev1 = 0.0;
        double x_prev2 = 0.0;
        double y_prev1 = 0.0;
        double y_prev2 = 0.0;
    } filterState;

    void createInputPorts();
    void createSignals();

    void calculate();
    void processData(SizeT readAmount, SizeT packetOffset);
    void processEventPacket(const EventPacketPtr& packet);

    void processSignalDescriptorChanged(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor);
    void configure();

    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();
};

END_NAMESPACE_EXAMPLE_MODULE
