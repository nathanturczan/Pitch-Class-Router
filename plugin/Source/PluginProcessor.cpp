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
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    for (auto& state : pitchClassState)
        state.store(false);
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

bool PitchClassRouterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Accept stereo or mono configurations
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input and output must match
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void PitchClassRouterProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    // Pass audio through unchanged (we're only processing MIDI)
    juce::ScopedNoDenormals noDenormals;

    // Add any pending MIDI messages from external input (no lock on the audio thread)
    {
        const auto scope = pendingFifo.read(pendingFifo.getNumReady());
        scope.forEach([&](int index)
        {
            const auto& cc = pendingEvents[(size_t) index];
            midiMessages.addEvent(juce::MidiMessage::controllerEvent(cc.channel, cc.controller, cc.value), 0);
        });
    }

    // Audio passthrough - nothing to do, buffer already contains input
    (void)buffer;
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
    state.setProperty("inputChannel", inputChannel.load(), nullptr);
    state.setProperty("outputChannel", outputChannel.load(), nullptr);
    state.setProperty("baseCC", baseCC.load(), nullptr);

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
            setInputChannel(state.getProperty("inputChannel", 7));
            outputChannel = (int) state.getProperty("outputChannel", 8);
            setBaseCC(state.getProperty("baseCC", 20));

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

    juce::ScopedLock lock(stateLock);

    int channel = message.getChannel(); // 1-16
    if (channel != inputChannel)
        return;

    int noteNumber = message.getNoteNumber();
    int pitchClass = noteNumber % 12;
    heldNotes[(size_t) noteNumber] = message.isNoteOn() && message.getVelocity() > 0;

    // The pitch class stays on while any note of that pitch class is still held
    bool isOn = false;
    for (int note = pitchClass; note < 128; note += 12)
        isOn = isOn || heldNotes[(size_t) note];

    // Only send if state changed
    if (pitchClassState[(size_t) pitchClass].exchange(isOn) != isOn)
    {
        sendPitchClassCC(pitchClass, isOn);
    }
}

void PitchClassRouterProcessor::resetPitchClassState()
{
    juce::ScopedLock lock(stateLock);

    heldNotes.fill(false);

    // Turn off whatever was on, so the target plugin does not keep stale toggles
    for (int pitchClass = 0; pitchClass < 12; ++pitchClass)
        if (pitchClassState[(size_t) pitchClass].exchange(false))
            sendPitchClassCC(pitchClass, false);
}

void PitchClassRouterProcessor::sendPitchClassCC(int pitchClass, bool isOn)
{
    int ccNumber = baseCC + pitchClass;
    int value = isOn ? 127 : 0;
    int channel = outputChannel;

    auto ccMessage = juce::MidiMessage::controllerEvent(channel, ccNumber, value);

    // Send to external MIDI output if configured
    {
        juce::ScopedLock lock(outputLock);
        if (midiOutput)
            midiOutput->sendMessageNow(ccMessage);
    }

    // Also queue for plugin output (so it goes through DAW for MIDI mapping)
    queueForHost(channel, ccNumber, value);
}

void PitchClassRouterProcessor::queueForHost(int channel, int ccNumber, int value)
{
    juce::ScopedLock lock(midiLock);

    // If the host is not calling processBlock and the FIFO is full, the event is dropped
    const auto scope = pendingFifo.write(1);
    if (scope.blockSize1 > 0)
        pendingEvents[(size_t) scope.startIndex1] = { channel, ccNumber, value };
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

    // Notes held on the previous device will never get their note-offs
    resetPitchClassState();

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
    // Open the new device first, then swap it in under the lock so the
    // MIDI-input thread never sends to a device that is being destroyed
    std::unique_ptr<juce::MidiOutput> newOutput;

    for (auto& device : juce::MidiOutput::getAvailableDevices())
    {
        if (device.name == deviceName)
        {
            newOutput = juce::MidiOutput::openDevice(device.identifier);
            break;
        }
    }

    currentMidiOutputName = newOutput ? deviceName : juce::String();

    {
        juce::ScopedLock lock(outputLock);
        std::swap(midiOutput, newOutput);
    }
    // The previous device (now in newOutput) is closed here, outside the lock
}

void PitchClassRouterProcessor::setInputChannel(int channel)
{
    channel = juce::jlimit(1, 16, channel);

    juce::ScopedLock lock(stateLock);

    if (channel == inputChannel)
        return;

    // Notes held on the previous channel will never get their note-offs
    resetPitchClassState();
    inputChannel = channel;
}

void PitchClassRouterProcessor::setOutputChannel(int channel)
{
    outputChannel = juce::jlimit(1, 16, channel);
}

void PitchClassRouterProcessor::setBaseCC(int cc)
{
    baseCC = juce::jlimit(0, 108, cc); // Max CC108 so CC+11 stays below the channel-mode messages (CC 120-127)
}

void PitchClassRouterProcessor::sendMappingCC(int pitchClass)
{
    if (pitchClass < 0 || pitchClass > 11)
        return;

    // Send CC with value 127 for mapping purposes
    int ccNumber = baseCC + pitchClass;
    int channel = outputChannel;
    auto ccMessage = juce::MidiMessage::controllerEvent(channel, ccNumber, 127);

    {
        juce::ScopedLock lock(outputLock);
        if (midiOutput)
            midiOutput->sendMessageNow(ccMessage);
    }

    queueForHost(channel, ccNumber, 127);
}

bool PitchClassRouterProcessor::getPitchClassState(int pitchClass) const
{
    if (pitchClass < 0 || pitchClass > 11)
        return false;
    return pitchClassState[(size_t) pitchClass].load();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchClassRouterProcessor();
}
