#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::StringArray& PitchClassRouterProcessor::getPitchClassNames()
{
    static const juce::StringArray names = {
        "C", "C#/Db", "D", "D#/Eb", "E", "F",
        "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B"
    };
    return names;
}

PitchClassRouterProcessor::PitchClassRouterProcessor()
    : AudioProcessor(BusesProperties())
{
    pitchClassState.fill(false);
}

PitchClassRouterProcessor::~PitchClassRouterProcessor()
{
    if (midiInput)
        midiInput->stop();
}

void PitchClassRouterProcessor::prepareToPlay(double, int)
{
}

void PitchClassRouterProcessor::releaseResources()
{
}

bool PitchClassRouterProcessor::isBusesLayoutSupported(const BusesLayout&) const
{
    return true;
}

void PitchClassRouterProcessor::processBlock(juce::AudioBuffer<float>&,
                                              juce::MidiBuffer& midiMessages)
{
    // Add any pending MIDI messages from external input
    {
        juce::ScopedLock lock(midiLock);
        midiMessages.addEvents(pendingMidiMessages, 0, -1, 0);
        pendingMidiMessages.clear();
    }
}

juce::AudioProcessorEditor* PitchClassRouterProcessor::createEditor()
{
    return new PitchClassRouterEditor(*this);
}

void PitchClassRouterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state("PitchClassRouterState");
    state.setProperty("midiInput", currentMidiInputName, nullptr);
    state.setProperty("midiOutput", currentMidiOutputName, nullptr);
    state.setProperty("inputChannel", inputChannel, nullptr);
    state.setProperty("outputChannel", outputChannel, nullptr);
    state.setProperty("baseCC", baseCC, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PitchClassRouterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
    {
        juce::ValueTree state = juce::ValueTree::fromXml(*xml);
        if (state.isValid())
        {
            juce::String inputName = state.getProperty("midiInput", "");
            juce::String outputName = state.getProperty("midiOutput", "");
            inputChannel = state.getProperty("inputChannel", 7);
            outputChannel = state.getProperty("outputChannel", 8);
            baseCC = state.getProperty("baseCC", 20);

            if (inputName.isNotEmpty())
                setMidiInput(inputName);
            if (outputName.isNotEmpty())
                setMidiOutput(outputName);
        }
    }
}

void PitchClassRouterProcessor::handleIncomingMidiMessage(juce::MidiInput*,
                                                           const juce::MidiMessage& message)
{
    // Only process Note On/Off messages on the input channel
    if (!message.isNoteOnOrOff())
        return;

    int channel = message.getChannel(); // 1-16
    if (channel != inputChannel)
        return;

    int noteNumber = message.getNoteNumber();
    int pitchClass = noteNumber % 12;
    bool isOn = message.isNoteOn() && message.getVelocity() > 0;

    // Only send if state changed
    if (pitchClassState[pitchClass] != isOn)
    {
        pitchClassState[pitchClass] = isOn;
        sendPitchClassCC(pitchClass, isOn);
    }
}

void PitchClassRouterProcessor::sendPitchClassCC(int pitchClass, bool isOn)
{
    int ccNumber = baseCC + pitchClass;
    int value = isOn ? 127 : 0;

    auto ccMessage = juce::MidiMessage::controllerEvent(outputChannel, ccNumber, value);

    // Send to external MIDI output if configured
    if (midiOutput)
        midiOutput->sendMessageNow(ccMessage);

    // Also queue for plugin output (so it goes through DAW for MIDI mapping)
    {
        juce::ScopedLock lock(midiLock);
        pendingMidiMessages.addEvent(ccMessage, 0);
    }
}

juce::StringArray PitchClassRouterProcessor::getAvailableMidiInputs() const
{
    juce::StringArray inputs;
    for (auto& device : juce::MidiInput::getAvailableDevices())
        inputs.add(device.name);
    return inputs;
}

juce::StringArray PitchClassRouterProcessor::getAvailableMidiOutputs() const
{
    juce::StringArray outputs;
    for (auto& device : juce::MidiOutput::getAvailableDevices())
        outputs.add(device.name);
    return outputs;
}

void PitchClassRouterProcessor::setMidiInput(const juce::String& deviceName)
{
    if (midiInput)
    {
        midiInput->stop();
        midiInput.reset();
    }

    currentMidiInputName = "";

    for (auto& device : juce::MidiInput::getAvailableDevices())
    {
        if (device.name == deviceName)
        {
            midiInput = juce::MidiInput::openDevice(device.identifier, this);
            if (midiInput)
            {
                midiInput->start();
                currentMidiInputName = deviceName;
            }
            break;
        }
    }
}

void PitchClassRouterProcessor::setMidiOutput(const juce::String& deviceName)
{
    midiOutput.reset();
    currentMidiOutputName = "";

    for (auto& device : juce::MidiOutput::getAvailableDevices())
    {
        if (device.name == deviceName)
        {
            midiOutput = juce::MidiOutput::openDevice(device.identifier);
            if (midiOutput)
                currentMidiOutputName = deviceName;
            break;
        }
    }
}

void PitchClassRouterProcessor::setInputChannel(int channel)
{
    inputChannel = juce::jlimit(1, 16, channel);
}

void PitchClassRouterProcessor::setOutputChannel(int channel)
{
    outputChannel = juce::jlimit(1, 16, channel);
}

void PitchClassRouterProcessor::setBaseCC(int cc)
{
    baseCC = juce::jlimit(0, 119, cc); // Max CC119 so CC+11 doesn't exceed 127
}

void PitchClassRouterProcessor::sendMappingCC(int pitchClass)
{
    if (pitchClass < 0 || pitchClass > 11)
        return;

    // Send CC with value 127 for mapping purposes
    int ccNumber = baseCC + pitchClass;
    auto ccMessage = juce::MidiMessage::controllerEvent(outputChannel, ccNumber, 127);

    if (midiOutput)
        midiOutput->sendMessageNow(ccMessage);

    {
        juce::ScopedLock lock(midiLock);
        pendingMidiMessages.addEvent(ccMessage, 0);
    }
}

bool PitchClassRouterProcessor::getPitchClassState(int pitchClass) const
{
    if (pitchClass < 0 || pitchClass > 11)
        return false;
    return pitchClassState[pitchClass];
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchClassRouterProcessor();
}
