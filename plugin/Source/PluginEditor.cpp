#include "PluginEditor.h"

PitchClassRouterEditor::PitchClassRouterEditor(PitchClassRouterProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(500, 400);

    // MIDI Input
    midiInputLabel.setText("MIDI Input:", juce::dontSendNotification);
    midiInputLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(midiInputLabel);

    midiInputCombo.onChange = [this] {
        int idx = midiInputCombo.getSelectedItemIndex();
        if (idx >= 0)
        {
            auto inputs = processor.getAvailableMidiInputs();
            if (idx < inputs.size())
                processor.setMidiInput(inputs[idx]);
        }
    };
    addAndMakeVisible(midiInputCombo);

    inputChannelLabel.setText("Ch:", juce::dontSendNotification);
    inputChannelLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(inputChannelLabel);

    for (int i = 1; i <= 16; ++i)
        inputChannelCombo.addItem(juce::String(i), i);
    inputChannelCombo.setSelectedId(processor.getInputChannel(), juce::dontSendNotification);
    inputChannelCombo.onChange = [this] {
        processor.setInputChannel(inputChannelCombo.getSelectedId());
    };
    addAndMakeVisible(inputChannelCombo);

    // MIDI Output
    midiOutputLabel.setText("MIDI Output:", juce::dontSendNotification);
    midiOutputLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(midiOutputLabel);

    midiOutputCombo.onChange = [this] {
        int idx = midiOutputCombo.getSelectedItemIndex();
        if (idx >= 0)
        {
            auto outputs = processor.getAvailableMidiOutputs();
            if (idx < outputs.size())
                processor.setMidiOutput(outputs[idx]);
        }
    };
    addAndMakeVisible(midiOutputCombo);

    outputChannelLabel.setText("Ch:", juce::dontSendNotification);
    outputChannelLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(outputChannelLabel);

    for (int i = 1; i <= 16; ++i)
        outputChannelCombo.addItem(juce::String(i), i);
    outputChannelCombo.setSelectedId(processor.getOutputChannel(), juce::dontSendNotification);
    outputChannelCombo.onChange = [this] {
        processor.setOutputChannel(outputChannelCombo.getSelectedId());
    };
    addAndMakeVisible(outputChannelCombo);

    // Base CC
    baseCCLabel.setText("Base CC:", juce::dontSendNotification);
    baseCCLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(baseCCLabel);

    for (int i = 0; i <= 116; ++i)
        baseCCCombo.addItem("CC " + juce::String(i), i + 1);
    baseCCCombo.setSelectedId(processor.getBaseCC() + 1, juce::dontSendNotification);
    baseCCCombo.onChange = [this] {
        processor.setBaseCC(baseCCCombo.getSelectedId() - 1);
    };
    addAndMakeVisible(baseCCCombo);

    // Mapping section
    mappingLabel.setText("Mapping Helper (click to send CC):", juce::dontSendNotification);
    mappingLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(mappingLabel);

    auto& pcNames = PitchClassRouterProcessor::getPitchClassNames();
    for (int i = 0; i < 12; ++i)
    {
        auto* btn = new juce::TextButton(pcNames[i]);
        btn->onClick = [this, i] {
            processor.sendMappingCC(i);
        };
        mappingButtons.add(btn);
        addAndMakeVisible(btn);

        auto* indicator = new juce::Label();
        indicator->setJustificationType(juce::Justification::centred);
        stateIndicators.add(indicator);
        addAndMakeVisible(indicator);
    }

    // Status
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(statusLabel);

    // Refresh device lists
    refreshMidiDevices();

    // Start timer for UI updates
    startTimerHz(10);
}

PitchClassRouterEditor::~PitchClassRouterEditor()
{
    stopTimer();
}

void PitchClassRouterEditor::refreshMidiDevices()
{
    midiInputCombo.clear();
    auto inputs = processor.getAvailableMidiInputs();
    int selectedInput = -1;
    for (int i = 0; i < inputs.size(); ++i)
    {
        midiInputCombo.addItem(inputs[i], i + 1);
        if (inputs[i] == processor.getCurrentMidiInput())
            selectedInput = i + 1;
    }
    if (selectedInput > 0)
        midiInputCombo.setSelectedId(selectedInput, juce::dontSendNotification);

    midiOutputCombo.clear();
    auto outputs = processor.getAvailableMidiOutputs();
    int selectedOutput = -1;
    for (int i = 0; i < outputs.size(); ++i)
    {
        midiOutputCombo.addItem(outputs[i], i + 1);
        if (outputs[i] == processor.getCurrentMidiOutput())
            selectedOutput = i + 1;
    }
    if (selectedOutput > 0)
        midiOutputCombo.setSelectedId(selectedOutput, juce::dontSendNotification);
}

void PitchClassRouterEditor::timerCallback()
{
    updateStatus();

    // Update pitch class state indicators
    for (int i = 0; i < 12; ++i)
    {
        bool isOn = processor.getPitchClassState(i);
        stateIndicators[i]->setColour(juce::Label::backgroundColourId,
                                       isOn ? juce::Colours::green : juce::Colours::darkgrey);
    }
}

void PitchClassRouterEditor::updateStatus()
{
    juce::String status;
    auto inputName = processor.getCurrentMidiInput();
    auto outputName = processor.getCurrentMidiOutput();

    if (inputName.isEmpty() && outputName.isEmpty())
        status = "No MIDI devices selected";
    else
    {
        if (inputName.isNotEmpty())
            status += "IN: " + inputName + " Ch" + juce::String(processor.getInputChannel());
        if (outputName.isNotEmpty())
        {
            if (status.isNotEmpty()) status += " | ";
            status += "OUT: " + outputName + " Ch" + juce::String(processor.getOutputChannel());
        }
    }

    statusLabel.setText(status, juce::dontSendNotification);
}

void PitchClassRouterEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("Pitch Class Router", 0, 10, getWidth(), 25, juce::Justification::centred);

    // Divider line
    g.setColour(juce::Colours::grey);
    g.drawLine(20, 180, getWidth() - 20, 180, 1.0f);
}

void PitchClassRouterEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(35); // Title space

    // MIDI Input row
    auto inputRow = area.removeFromTop(30);
    midiInputLabel.setBounds(inputRow.removeFromLeft(90));
    inputRow.removeFromLeft(5);
    midiInputCombo.setBounds(inputRow.removeFromLeft(200));
    inputRow.removeFromLeft(10);
    inputChannelLabel.setBounds(inputRow.removeFromLeft(30));
    inputChannelCombo.setBounds(inputRow.removeFromLeft(60));

    area.removeFromTop(10);

    // MIDI Output row
    auto outputRow = area.removeFromTop(30);
    midiOutputLabel.setBounds(outputRow.removeFromLeft(90));
    outputRow.removeFromLeft(5);
    midiOutputCombo.setBounds(outputRow.removeFromLeft(200));
    outputRow.removeFromLeft(10);
    outputChannelLabel.setBounds(outputRow.removeFromLeft(30));
    outputChannelCombo.setBounds(outputRow.removeFromLeft(60));

    area.removeFromTop(10);

    // Base CC row
    auto ccRow = area.removeFromTop(30);
    baseCCLabel.setBounds(ccRow.removeFromLeft(90));
    ccRow.removeFromLeft(5);
    baseCCCombo.setBounds(ccRow.removeFromLeft(100));

    area.removeFromTop(25);

    // Mapping section
    mappingLabel.setBounds(area.removeFromTop(25));
    area.removeFromTop(10);

    // Mapping buttons - 2 rows of 6
    int buttonWidth = 70;
    int buttonHeight = 35;
    int indicatorHeight = 8;
    int spacing = 5;

    for (int row = 0; row < 2; ++row)
    {
        auto buttonRow = area.removeFromTop(buttonHeight + indicatorHeight + spacing);
        int startX = (getWidth() - (6 * buttonWidth + 5 * spacing)) / 2;

        for (int col = 0; col < 6; ++col)
        {
            int idx = row * 6 + col;
            int x = startX + col * (buttonWidth + spacing);

            mappingButtons[idx]->setBounds(x, buttonRow.getY(), buttonWidth, buttonHeight);
            stateIndicators[idx]->setBounds(x, buttonRow.getY() + buttonHeight + 2,
                                            buttonWidth, indicatorHeight);
        }

        area.removeFromTop(5);
    }

    // Status at bottom
    statusLabel.setBounds(area.removeFromBottom(25));
}
