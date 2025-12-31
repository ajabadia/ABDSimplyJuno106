#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"

class JunoVCASection : public juce::Component
{
public:
    JunoVCASection(juce::AudioProcessorValueTreeState& apvts, MidiLearnHandler& mlh)
    {
        JunoUI::setupVerticalSlider(levelSlider);
        addAndMakeVisible(levelSlider);
        levelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vcaLevel", levelSlider);
        JunoUI::setupLabel(levelLabel, "LEVEL", *this);

        addAndMakeVisible(gateButton);
        gateButton.setButtonText("GATE");
        gateAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "vcaMode", gateButton);

        JunoUI::setupMidiLearn(levelSlider, mlh, "vcaLevel", midiLearnListeners);
        JunoUI::setupMidiLearn(gateButton, mlh, "vcaMode", midiLearnListeners);

        levelSlider.getProperties().set("numberSide", "Right");
    }

    void paint(juce::Graphics& g) override
    {
        JunoUI::drawSectionBackground(g, getLocalBounds(), "VCA");
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 30);
        int halfW = area.getWidth() / 2;

        // Switch
        gateButton.setBounds(area.getX(), area.getY() + 20, 50, 30);
        
        // Slider
        // Slider
        int labelH = 20;
        levelLabel.setBounds(area.getX() + halfW, area.getY(), 50, labelH);
        levelSlider.setBounds(area.getX() + halfW, area.getY() + labelH, 50, area.getHeight() - labelH - 5);
    }

private:
    juce::Slider levelSlider;
    juce::ToggleButton gateButton;
    juce::Label levelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateAtt;
    
    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
