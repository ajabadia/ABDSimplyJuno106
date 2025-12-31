#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"

class JunoVCFSection : public juce::Component
{
public:
    JunoVCFSection(juce::AudioProcessorValueTreeState& apvts, MidiLearnHandler& mlh)
    {
        JunoUI::setupVerticalSlider(cutoffSlider);
        JunoUI::setupVerticalSlider(resSlider);
        JunoUI::setupVerticalSlider(envSlider);
        JunoUI::setupVerticalSlider(lfoSlider);
        JunoUI::setupVerticalSlider(keySlider);

        addAndMakeVisible(cutoffSlider);
        addAndMakeVisible(resSlider);
        addAndMakeVisible(envSlider);
        addAndMakeVisible(lfoSlider);
        addAndMakeVisible(keySlider);

        // Polarity Switch (Authentic)
        // Positioned between RES and ENV.
        addAndMakeVisible(invSwitch);
        invSwitch.setSliderStyle(juce::Slider::LinearVertical);
        invSwitch.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        invSwitch.setRange(0.0, 1.0, 1.0);
        invSwitch.getProperties().set("isSwitch", true); 
        
        // Corrected Parameter IDs based on SynthParams.h / PluginProcessor.cpp
        cutoffAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vcfFreq", cutoffSlider);
        resAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "resonance", resSlider);
        envAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "envAmount", envSlider); // Fixed ID
        lfoAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lfoToVCF", lfoSlider); // Fixed ID
        keyAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "kybdTracking", keySlider);
        invAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vcfPolarity", invSwitch); // Switch is Slider

        JunoUI::setupLabel(cutoffLabel, "FREQ", *this);
        JunoUI::setupLabel(resLabel, "RES", *this);
        JunoUI::setupLabel(envLabel, "ENV", *this); // Added missing label
        JunoUI::setupLabel(lfoLabel, "LFO", *this);
        JunoUI::setupLabel(keyLabel, "KYBD", *this);
        
        // Switch Graphics (Polarity Icon?)
        // Authentic: Polarity Graphic between RES and ENV.
        // I will draw it or use labels? The image shows graphic.
        // I'll add a label "POL" for now or just trust the switch.

        // MIDI Learn
        auto setup = [&](juce::Component& c, const juce::String& p) { JunoUI::setupMidiLearn(c, mlh, p, midiLearnListeners); };
        setup(cutoffSlider, "vcfFreq");
        setup(resSlider, "resonance");
        setup(invSwitch, "vcfPolarity");
        setup(envSlider, "envAmount");
        setup(lfoSlider, "lfoToVCF");
        setup(keySlider, "kybdTracking");

        // Authentic Number Siding
        cutoffSlider.getProperties().set("numberSide", "Left");
        resSlider.getProperties().set("numberSide", "Right");
        envSlider.getProperties().set("numberSide", "Left");
        lfoSlider.getProperties().set("numberSide", "Right");
        keySlider.getProperties().set("numberSide", "Left");
    }

    void paint(juce::Graphics& g) override
    {
        JunoUI::drawSectionBackground(g, getLocalBounds(), "VCF");
        
        // Draw Authentic Polarity Graphic (Normal / Inverted Envelope) near Switch
        // Switch is in Column 3.
        // Normal (Up): Envelope graphic
        // Inverted (Down): Inverted Enevelope graphic
        
        // I need coordinates. I'll defer to resized if I store coords, or just hardcode relative.
        // Let's assume layout is strict. If I move things, I must update this.
        // Better: Find switch bounds.
        
        auto swBounds = invSwitch.getBounds();
        if (swBounds.isEmpty()) return;
        
        g.setColour(JunoUI::kTextGrey);
        juce::Path p;
        
        // Normal (Top) - Above Switch
        float x = (float)swBounds.getCentreX() - 5.0f;
        float y = (float)swBounds.getY() - 15.0f;
        
        // Small icon of envelope (Normal)
        p.startNewSubPath(x, y + 10);
        p.lineTo(x + 5, y);
        p.lineTo(x + 10, y + 10);
        
        // Inverted (Bottom) - Below Switch
        float y2 = (float)swBounds.getBottom() + 5.0f;
        p.startNewSubPath(x, y2);
        p.lineTo(x + 5, y2 + 10);
        p.lineTo(x + 10, y2);
        
        g.strokePath(p, juce::PathStrokeType(1.5f));
        
        // Connection line from ENV slider to Switch? (As seen in image)
        // Authentic image shows line from "ENV" label to Switch?
        // Actually line connects Freq/Res and Env/LFO/Kybd groups?
        // I'll skip complex lines for now to keep it clean.
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 30);
        // We have 6 elements: Freq, Res, Switch, Env, LFO, Kybd
        // Switch takes less width.
        // Let's allocate 5 main columns plus a thin one?
        // Or 6 equal columns.
        int colW = area.getWidth() / 6;
        int startX = area.getX();
        
        auto place = [&](juce::Component& c, juce::Label* l) {
            
            if (l) {
                int labelH = 20;
                l->setBounds(startX, area.getY(), 50, labelH);
                c.setBounds(startX + (colW - 50)/2, area.getY() + labelH, 50, area.getHeight() - labelH - 5);
            } else {
                 c.setBounds(startX + (colW - 50)/2, area.getY(), 50, area.getHeight() - 25);
            }
            
            startX += colW;
        };

        // 1. Freq
        place(cutoffSlider, &cutoffLabel);
        
        // 2. Res
        place(resSlider, &resLabel);

        // 3. Polarity Switch (Shorter, centered vertically, no label below)
        // Switch height 40? No, authentic is small slider switch.
        // Length 40-50 px.
        int swH = 50;
        int swW = 30;
        int swY = area.getY() + (area.getHeight() - swH) / 2;
        invSwitch.setBounds(startX + (colW - swW)/2, swY, swW, swH);
        startX += colW;

        // 4. Env
        place(envSlider, &envLabel);

        // 5. LFO
        place(lfoSlider, &lfoLabel);

        // Col 6: Kybd
        place(keySlider, &keyLabel);
    }

private:
    juce::Slider cutoffSlider, resSlider, envSlider, lfoSlider, keySlider;
    juce::Slider invSwitch; // Was invButton
    juce::Label cutoffLabel, resLabel, envLabel, lfoLabel, keyLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAtt, resAtt, envAtt, lfoAtt, keyAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> invAtt;

    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
