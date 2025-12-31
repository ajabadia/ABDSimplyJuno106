#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"

// Note: Combining Chorus and "Extras" (Portamento, Mode, etc.) might be cleaner 
// separated, but usually they are the "Lower Control Board" or "Left Cheek" + "Right Chorus".
// Let's separate Chorus to match the top strip flow if desired, or just "LowerPanel".

class JunoChorusSection : public juce::Component
{
public:
    JunoChorusSection(juce::AudioProcessorValueTreeState& apvts, MidiLearnHandler& mlh)
    {
        addAndMakeVisible(b1);
        b1.setButtonText(""); // No text in button, use label above
        att1 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "chorus1", b1);
        JunoUI::setupLabel(l1, "I", *this);

        addAndMakeVisible(b2);
        b2.setButtonText(""); // No text in button
        att2 = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "chorus2", b2);
        JunoUI::setupLabel(l2, "II", *this);

        JunoUI::setupMidiLearn(b1, mlh, "chorus1", midiLearnListeners);
        JunoUI::setupMidiLearn(b2, mlh, "chorus2", midiLearnListeners);
    }

    void paint(juce::Graphics& g) override
    {
        JunoUI::drawSectionBackground(g, getLocalBounds(), "CHORUS");
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(10, 30);
        int btnW = 40;
        int btnH = 50;
        int gap = 10;
        
        int totalW = (btnW * 2) + gap;
        int startX = (getWidth() - totalW) / 2;
        int btnY = 60; // Lower buttons
        int labelY = 40;
        
        l1.setBounds(startX, labelY, btnW, 20);
        b1.setBounds(startX, btnY, btnW, btnH);
        
        l2.setBounds(startX + btnW + gap, labelY, btnW, 20);
        b2.setBounds(startX + btnW + gap, btnY, btnW, btnH);
    }

private:
    juce::ToggleButton b1, b2;
    juce::Label l1, l2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att1, att2;
    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
