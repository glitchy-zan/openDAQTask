#define _USE_MATH_DEFINES
#include <example_module/dispatch.h>
#include </Users/akasi/Desktop/OpenDAQ/repos/example_module/include/example_module/example_iir_filter_fb.h>
#include <opendaq/event_packet_params.h>
#include <cmath>

BEGIN_NAMESPACE_EXAMPLE_MODULE
ExampleFBImplIIR::ExampleFBImplIIR(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initComponentStatus();
    createInputPorts();
    createSignals();
    initProperties();
}

// initialization stuff
void ExampleFBImplIIR::initProperties()
{
    // cut off freq
    const auto cutoffProp = FloatProperty("CutoffFrequency", 100.0);
    objPtr.addProperty(cutoffProp);
    objPtr.getOnPropertyValueWrite("CutoffFrequency") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto useCustomOutputRangeProp = BoolProperty("UseCustomOutputRange", False);
    objPtr.addProperty(useCustomOutputRangeProp);
    objPtr.getOnPropertyValueWrite("UseCustomOutputRange") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customHighValueProp = FloatProperty("OutputHighValue", 10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("OutputHighValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto customLowValueProp = FloatProperty("OutputLowValue", -10.0, EvalValue("$UseCustomOutputRange"));
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("OutputLowValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto outputNameProp = StringProperty("OutputName", "");
    objPtr.addProperty(outputNameProp);
    objPtr.getOnPropertyValueWrite("OutputName") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    const auto outputUnitProp = StringProperty("OutputUnit", "");
    objPtr.addProperty(outputUnitProp);
    objPtr.getOnPropertyValueWrite("OutputUnit") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    readProperties();
}

void ExampleFBImplIIR::propertyChanged(bool configure)
{
    readProperties();
    if (configure)
        this->configure();
}

void ExampleFBImplIIR::readProperties()
{
    // cut off freq
    cutoffFrequency = objPtr.getPropertyValue("CutoffFrequency");
    useCustomOutputRange = objPtr.getPropertyValue("UseCustomOutputRange");
    outputHighValue = objPtr.getPropertyValue("OutputHighValue");
    outputLowValue = objPtr.getPropertyValue("OutputLowValue");
    outputUnit = static_cast<std::string>(objPtr.getPropertyValue("OutputUnit"));
    outputName = static_cast<std::string>(objPtr.getPropertyValue("OutputName"));
}

FunctionBlockTypePtr ExampleFBImplIIR::CreateType()
{
    return FunctionBlockType("ExampleIIRFilter", "IIR", "IIR Filter");
}

void ExampleFBImplIIR::processSignalDescriptorChanged(const DataDescriptorPtr& dataDescriptor, const DataDescriptorPtr& domainDescriptor)
{
    if (dataDescriptor.assigned())
        this->inputDataDescriptor = dataDescriptor;
    if (domainDescriptor.assigned())
        this->inputDomainDataDescriptor = domainDescriptor;

    configure();
}

// validates signal
void ExampleFBImplIIR::configure()
{
    try
    {
        if (!inputDomainDataDescriptor.assigned() || inputDomainDataDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("No domain input");
        }

        if (!inputDataDescriptor.assigned() || inputDataDescriptor == NullDataDescriptor())
        {
            throw std::runtime_error("No value input");
        }

        if (inputDataDescriptor.getDimensions().getCount() > 0)
        {
            throw std::runtime_error("Arrays not supported");
        }

        inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Float64 && inputSampleType != SampleType::Float32 && inputSampleType != SampleType::Int8 &&
            inputSampleType != SampleType::Int16 && inputSampleType != SampleType::Int32 && inputSampleType != SampleType::Int64 &&
            inputSampleType != SampleType::UInt8 && inputSampleType != SampleType::UInt16 && inputSampleType != SampleType::UInt32 &&
            inputSampleType != SampleType::UInt64)
        {
            throw std::runtime_error("Invalid sample type");
        }

        // Accept only synchronous (linear implicit) domain signals
        if (inputDomainDataDescriptor.getSampleType() != SampleType::Int64 &&
            inputDomainDataDescriptor.getSampleType() != SampleType::UInt64)
        {
            throw std::runtime_error("Incompatible domain data sample type");
        }

        const auto domainUnit = inputDomainDataDescriptor.getUnit();
        if (domainUnit.getSymbol() != "s" && domainUnit.getSymbol() != "seconds")
        {
            throw std::runtime_error("Domain unit expected in seconds");
        }

        const auto domainRule = inputDomainDataDescriptor.getRule();
        if (inputDomainDataDescriptor.getRule().getType() != DataRuleType::Linear)
        {
            throw std::runtime_error("Domain rule must be linear");
        }

        // filter stuff
        /************************************************************************************************/
        // Get sample rate from existing code
        sampleRate = reader::getSampleRate(inputDomainDataDescriptor);

        // Validate sample rate first
        if (sampleRate <= 0)
            throw std::runtime_error("Invalid sample rate");

        // Validate cutoff frequency
        double fc = cutoffFrequency;
        if (fc <= 0.0 || fc >= sampleRate / 2.0)
            throw std::runtime_error("Cutoff frequency must be between 0 and Nyquist frequency");

        // Calculate Butterworth coefficients (2nd order)
        const double theta_c = 2.0 * M_PI * fc / sampleRate;
        const double C = 1.0 / tan(theta_c / 2.0);
        const double C_squared = C * C;
        const double sqrt2_C = std::sqrt(2.0) * C;
        const double a0 = C_squared + sqrt2_C + 1.0;

        b0 = 1.0 / a0;
        b1 = 2.0 * b0;
        b2 = b0;
        a1 = 2.0 * (1.0 - C_squared) / a0;
        a2 = (C_squared - sqrt2_C + 1.0) / a0;

        // Reset filter state between configurations
        filterState = FilterState();

        /************************************************************************************************/

        RangePtr outputRange;
        if (useCustomOutputRange)
        {
            outputRange = Range(outputLowValue, outputHighValue);
        }
        else
        {
            // Now uses raw input range instead of scaled values
            const auto inputHigh = static_cast<Float>(inputDataDescriptor.getValueRange().getHighValue());
            const auto inputLow = static_cast<Float>(inputDataDescriptor.getValueRange().getLowValue());
            outputRange = Range(inputLow, inputHigh);
        }

        auto name = outputName.empty() ? inputPort.getSignal().getName().toStdString() + "/Scaled" : outputName;
        auto unit = outputUnit.empty() ? inputDataDescriptor.getUnit() : Unit(outputUnit);

        outputDataDescriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).setValueRange(outputRange).setUnit(unit).build();
        outputDomainDataDescriptor = inputDomainDataDescriptor;

        outputSignal.setDescriptor(outputDataDescriptor);
        outputSignal.setName(name);
        outputDomainSignal.setDescriptor(inputDomainDataDescriptor);

        // Allocate 1s buffer
        inputData.resize(sampleRate);
        inputDomainData.resize(sampleRate);

        setComponentStatus(ComponentStatus::Ok);
    }
    catch (const std::exception& e)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, fmt::format("Configuration error: {}", e.what()));
        outputSignal.setDescriptor(nullptr);
        configValid = false;
    }
    configValid = true;
}

// preproc
void ExampleFBImplIIR::calculate()
{
    auto lock = this->getAcquisitionLock();

    while (!reader.getEmpty())
    {
        SizeT readAmount = std::min(reader.getAvailableCount(), sampleRate);
        const auto status = reader.readWithDomain(inputData.data(), inputDomainData.data(), &readAmount);

        if (configValid)
        {
            processData(readAmount, status.getOffset());
        }

        if (status.getReadStatus() == ReadStatus::Event)
        {
            const auto eventPacket = status.getEventPacket();
            if (eventPacket.assigned())
                processEventPacket(eventPacket);
            return;
        }
    }
}

// proc
void ExampleFBImplIIR::processData(SizeT readAmount, SizeT packetOffset)
{
    if (readAmount == 0)
        return;

    const auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, readAmount, packetOffset);
    const auto outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, readAmount);
    auto outputData = static_cast<Float*>(outputPacket.getRawData());

    // filter implementation
    for (size_t i = 0; i < readAmount; i++)
    {
        double inputSample = static_cast<double>(inputData[i]);

        // Apply IIR filter
        double outputSample =
            b0 * inputSample + b1 * filterState.x_prev1 + b2 * filterState.x_prev2 - a1 * filterState.y_prev1 - a2 * filterState.y_prev2;

        // Update filter state
        filterState.x_prev2 = filterState.x_prev1;
        filterState.x_prev1 = inputSample;
        filterState.y_prev2 = filterState.y_prev1;
        filterState.y_prev1 = outputSample;

        *outputData++ = outputSample;
    }

    outputSignal.sendPacket(outputPacket);
    outputDomainSignal.sendPacket(outputDomainPacket);
}

void ExampleFBImplIIR::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        DataDescriptorPtr dataDesc = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr domainDesc = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(dataDesc, domainDesc);
    }
}

// initialization stuff
void ExampleFBImplIIR::createInputPorts()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::Scheduler);
    reader = StreamReaderFromPort(inputPort, SampleType::Float32, SampleType::UInt64);
    reader.setOnDataAvailable([this] { calculate(); });
}

// initialization stuff
void ExampleFBImplIIR::createSignals()
{
    outputSignal = createAndAddSignal("Scaled");
    outputDomainSignal = createAndAddSignal("ScaledTime", nullptr, false);
    outputSignal.setDomainSignal(outputDomainSignal);
}

END_NAMESPACE_EXAMPLE_MODULE
