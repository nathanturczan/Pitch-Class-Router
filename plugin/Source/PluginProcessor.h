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

    // Base CC (default 20, so C=CC20, C#=CC21, ..., B=CC31)
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
    int inputChannel = 7;
    int outputChannel = 8;

    // Base CC number for pitch class 0 (C)
    int baseCC = 20;

    // Pitch class state (12 elements, one per pitch class)
    std::array<bool, 12> pitchClassState;

    // Mutex for thread safety
    juce::CriticalSection midiLock;

    // Pending MIDI messages to send in processBlock
    juce::MidiBuffer pendingMidiMessages;

    void sendPitchClassCC(int pitchClass, bool isOn);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchClassRouterProcessor)
};
