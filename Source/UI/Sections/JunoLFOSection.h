#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"

class JunoLFOSection : public juce::Component
{
public:
    JunoLFOSection(juce::AudioProcessorValueTreeState& apvts, MidiLearnHandler& mlh)
    {
        JunoUI::setupVerticalSlider(rateSlider);
        JunoUI::setupVerticalSlider(delaySlider);

        addAndMakeVisible(rateSlider);
        addAndMakeVisible(delaySlider);

        JunoUI::setupLabel(rateLabel, "RATE", *this);
        JunoUI::setupLabel(delayLabel, "DELAY", *this);

        rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lfoRate", rateSlider);
        delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lfoDelay", delaySlider);

        JunoUI::setupMidiLearn(rateSlider, mlh, "lfoRate", midiLearnListeners);
        JunoUI::setupMidiLearn(delaySlider, mlh, "lfoDelay", midiLearnListeners);

        // Authentic Number Siding
        rateSlider.getProperties().set("numberSide", "Left");
        delaySlider.getProperties().set("numberSide", "Right");
    }

    void paint(juce::Graphics& g) override
    {
        JunoUI::drawSectionBackground(g, getLocalBounds(), "LFO");
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10, 30); // Margin top for title
        // Simple 2-column layout
        int sliderW = 50;
        int gap = (bounds.getWidth() - (sliderW * 2)) / 3;

        // Labels placed ABOVE sliders
        int labelH = 20;
        int sliderY = bounds.getY() + labelH;
        int sliderH = bounds.getHeight() - labelH - 10;

        rateLabel.setBounds(bounds.getX() + gap, bounds.getY(), sliderW, labelH);
        rateSlider.setBounds(bounds.getX() + gap, sliderY, sliderW, sliderH);
        
        delayLabel.setBounds(rateSlider.getRight() + gap, bounds.getY(), sliderW, labelH);
        delaySlider.setBounds(rateSlider.getRight() + gap, sliderY, sliderW, sliderH);
    }

private:
    juce::Slider rateSlider, delaySlider;
    juce::Label rateLabel, delayLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayAttachment;
    
    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
