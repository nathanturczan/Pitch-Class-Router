#pragma once

#include "PluginProcessor.h"

class PitchClassRouterEditor : public juce::AudioProcessorEditor,
                                public juce::Timer
{
public:
    explicit PitchClassRouterEditor(PitchClassRouterProcessor&);
    ~PitchClassRouterEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    PitchClassRouterProcessor& processor;

    // MIDI Input settings
    juce::Label midiInputLabel;
    juce::ComboBox midiInputCombo;
    juce::Label inputChannelLabel;
    juce::ComboBox inputChannelCombo;

    // MIDI Output settings
    juce::Label midiOutputLabel;
    juce::ComboBox midiOutputCombo;
    juce::Label outputChannelLabel;
    juce::ComboBox outputChannelCombo;

    // Base CC setting
    juce::Label baseCCLabel;
    juce::ComboBox baseCCCombo;

    // Mapping buttons (12 pitch classes)
    juce::Label mappingLabel;
    juce::OwnedArray<juce::TextButton> mappingButtons;

    // Status display
    juce::Label statusLabel;

    // Pitch class state indicators
    juce::OwnedArray<juce::Label> stateIndicators;

    void refreshMidiDevices();
    void updateStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchClassRouterEditor)
};
