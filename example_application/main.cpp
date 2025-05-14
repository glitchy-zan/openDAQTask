/**
 * Server application
 */

#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int argc, const char* argv[])
{
    // root object
    const auto instance = Instance();

    // device stuff
    auto referenceDevice = instance.addDevice("daqref://device0");
    daq::ChannelPtr channel = referenceDevice.getChannels()[0];
    // freq and noise from user guide
    channel.setPropertyValue("Frequency", 5);
    channel.setPropertyValue("NoiseAmplitude", 0.75);

    // renderer
    auto renderer = instance.addFunctionBlock("RefFBModuleRenderer");

    // our modules
    auto exampleModuleIIR = instance.addFunctionBlock("ExampleIIRFilter");
    auto exampleModuleIIR2 = instance.addFunctionBlock("ExampleIIRFilter");
    auto exampleModuleIIR3 = instance.addFunctionBlock("ExampleIIRFilter");
    auto exampleModuleIIR4 = instance.addFunctionBlock("ExampleIIRFilter");
    
    exampleModuleIIR.setPropertyValue("CutoffFrequency", 99);
    exampleModuleIIR2.setPropertyValue("CutoffFrequency", 55);
    exampleModuleIIR3.setPropertyValue("CutoffFrequency", 11);
    exampleModuleIIR4.setPropertyValue("CutoffFrequency", 0.5);

    // connect right ports
    exampleModuleIIR.getInputPorts()[0].connect(referenceDevice.getSignalsRecursive()[0]);
    exampleModuleIIR2.getInputPorts()[0].connect(referenceDevice.getSignalsRecursive()[0]);
    exampleModuleIIR3.getInputPorts()[0].connect(referenceDevice.getSignalsRecursive()[0]);
    exampleModuleIIR4.getInputPorts()[0].connect(referenceDevice.getSignalsRecursive()[0]);

    renderer.getInputPorts()[0].connect(referenceDevice.getSignalsRecursive()[0]);
    renderer.getInputPorts()[1].connect(exampleModuleIIR.getSignals()[0]);
    renderer.getInputPorts()[2].connect(exampleModuleIIR2.getSignals()[0]);
    renderer.getInputPorts()[3].connect(exampleModuleIIR3.getSignals()[0]);
    renderer.getInputPorts()[4].connect(exampleModuleIIR4.getSignals()[0]);

    // exit
    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
