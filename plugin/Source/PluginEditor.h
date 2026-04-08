#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"

class PitchClassRouterEditor : public juce::AudioProcessorEditor,
                                public juce::Timer
{
public:
    explicit PitchClassRouterEditor(PitchClassRouterProcessor&);
    ~PitchClassRouterEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Font helper - Inter font with tracking (matches Tonalign)
    static juce::Font getInterFont(float size, int style = juce::Font::plain) {
        static juce::Typeface::Ptr interRegular = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize);
        static juce::Typeface::Ptr interBold = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);

        juce::Typeface::Ptr typeface = (style & juce::Font::bold) ? interBold : interRegular;
        juce::Font font(typeface);
        font.setHeight(size);
        font.setExtraKerningFactor(-0.005f);  // letter-spacing: -0.005em
        return font;
    }

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

    // Branding labels
    juce::Label mBrandLabel;      // "Scale Navigator"
    juce::Label mProductLabel;    // "Pitch Class Router"

    // Pitch class state indicators
    juce::OwnedArray<juce::Label> stateIndicators;

    void refreshMidiDevices();
    void updateStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchClassRouterEditor)
};
