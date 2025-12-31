#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"

class JunoENVSection : public juce::Component
{
public:
    JunoENVSection(juce::AudioProcessorValueTreeState& apvts, MidiLearnHandler& mlh)
    {
        auto setup = [&](juce::Slider& s, juce::Label& l, const char* id, const char* name) {
            JunoUI::setupVerticalSlider(s);
            addAndMakeVisible(s);
            JunoUI::setupLabel(l, name, *this);
            atts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, id, s));
            JunoUI::setupMidiLearn(s, mlh, id, midiLearnListeners);
        };

        setup(aSlider, aLabel, "attack", "A");
        setup(dSlider, dLabel, "decay", "D");
        setup(sSlider, sLabel, "sustain", "S");
        setup(rSlider, rLabel, "release", "R");

        // Authentic Number Siding
        aSlider.getProperties().set("numberSide", "Left");
        dSlider.getProperties().set("numberSide", "Right");
        sSlider.getProperties().set("numberSide", "Left");
        rSlider.getProperties().set("numberSide", "Right");
    }

    void paint(juce::Graphics& g) override
    {
        JunoUI::drawSectionBackground(g, getLocalBounds(), "ENV");
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 30);
        int colW = area.getWidth() / 4;
        int startX = area.getX();

        auto place = [&](juce::Slider& s, juce::Label& l) {
            int labelH = 20;
            l.setBounds(startX, area.getY(), 50, labelH);
            s.setBounds(startX, area.getY() + labelH, 50, area.getHeight() - labelH - 5);
            startX += colW;
        };

        place(aSlider, aLabel);
        place(dSlider, dLabel);
        place(sSlider, sLabel);
        place(rSlider, rLabel);
    }

private:
    juce::Slider aSlider, dSlider, sSlider, rSlider;
    juce::Label aLabel, dLabel, sLabel, rLabel;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;
    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
