#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"

class JunoHPFSection : public juce::Component
{
public:
    JunoHPFSection(juce::AudioProcessorValueTreeState& apvts, MidiLearnHandler& mlh)
    {
        addAndMakeVisible(hpfSlider);
        hpfSlider.setSliderStyle(juce::Slider::LinearVertical);
        hpfSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        hpfSlider.setRange(0, 3, 1);
        
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "hpfFreq", hpfSlider);
        JunoUI::setupLabel(label, "FREQ", *this);

        JunoUI::setupMidiLearn(hpfSlider, mlh, "hpfFreq", midiLearnListeners);
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        JunoUI::drawSectionBackground(g, area, "HPF");

        // Authentic 0, 1, 2, 3 labels for the slider
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        
        float sliderTop = 60.0f;
        float sliderBottom = (float)getHeight() - 40.0f;
        float step = (sliderBottom - sliderTop) / 3.0f;

        for (int i = 0; i <= 3; ++i)
        {
            float y = sliderBottom - (i * step);
            // Draw on left and right of slider
            g.drawText(juce::String(i), 5, (int)y - 10, 15, 20, juce::Justification::centredRight);
            g.drawText(juce::String(i), getWidth() - 20, (int)y - 10, 15, 20, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(5, 30);
        label.setBounds(area.getX(), area.getY(), area.getWidth(), 20);
        hpfSlider.setBounds(area.getX() + 15, area.getY() + 25, area.getWidth() - 30, area.getHeight() - 30);
    }

private:
    juce::Slider hpfSlider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
