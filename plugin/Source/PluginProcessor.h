#pragma once

#include <JuceHeader.h>

class PitchClassRouterProcessor : public juce::AudioProcessor,
                                   public juce::MidiInputCallback
{
public:
    PitchClassRouterProcessor();
    ~PitchClassRouterProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // MidiInputCallback override
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

    // MIDI device management
    juce::StringArray getAvailableMidiInputs() const;
    juce::StringArray getAvailableMidiOutputs() const;

    void setMidiInput(const juce::String& deviceName);
    void setMidiOutput(const juce::String& deviceName);
    juce::String getCurrentMidiInput() const { return currentMidiInputName; }
    juce::String getCurrentMidiOutput() const { return currentMidiOutputName; }

    // Channel settings (1-16)
    void setInputChannel(int channel);
    void setOutputChannel(int channel);
    int getInputChannel() const { return inputChannel; }
    int getOutputChannel() const { return outputChannel; }

    // Base CC (default 20, so C=CC20, C#=CC21, ..., B=CC31), limited to 0-108
    void setBaseCC(int cc);
    int getBaseCC() const { return baseCC; }

    // Mapping helper - send CC for a specific pitch class
    void sendMappingCC(int pitchClass);

    // Pitch class state (for UI display)
    bool getPitchClassState(int pitchClass) const;

    // Pitch class names
    static const juce::StringArray& getPitchClassNames();

private:
    // MIDI devices
    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::String currentMidiInputName;
    juce::String currentMidiOutputName;

    // Channel settings (1-16)
    // Atomic: written by the UI, read by the MIDI-input thread
    std::atomic<int> inputChannel { 7 };
    std::atomic<int> outputChannel { 8 };

    // Base CC number for pitch class 0 (C)
    std::atomic<int> baseCC { 20 };

    // Pitch class state (12 elements, one per pitch class)
    std::array<std::atomic<bool>, 12> pitchClassState;

    // Notes currently held on the input channel; a pitch class is on while any of its notes is held
    std::array<bool, 128> heldNotes {};

    // Guards heldNotes and the input channel against changes while a note is being handled
    juce::CriticalSection stateLock;

    // Guards midiOutput (swapped by the UI while the MIDI-input thread sends to it)
    juce::CriticalSection outputLock;

    // Serialises the threads that queue CCs (MIDI input, UI); never taken in processBlock
    juce::CriticalSection midiLock;

    // Pending CCs to send in processBlock: fixed-size FIFO read lock-free by the audio thread
    struct PendingCC { int channel, controller, value; };
    static constexpr int pendingFifoSize = 1024;
    juce::AbstractFifo pendingFifo { pendingFifoSize };
    std::array<PendingCC, pendingFifoSize> pendingEvents {};

    void queueForHost(int channel, int ccNumber, int value);
    void sendPitchClassCC(int pitchClass, bool isOn);
    void resetPitchClassState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchClassRouterProcessor)
};
