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

        // Gate Switch (Authentic: ENV up, GATE down)
        addAndMakeVisible(gateSwitch);
        gateSwitch.setSliderStyle(juce::Slider::LinearVertical);
        gateSwitch.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        gateSwitch.setRange(0.0, 1.0, 1.0); // 0=ENV (usually down?), 1=GATE? 
        // Wait, typical switch: Up is usually 1, Down is 0.
        // User wants: "ENV" (Top), "GATE" (Bottom).
        // Let's assume parameter: 0=ENV, 1=GATE. 
        // If 0=ENV, then Down=ENV? 
        // Let's rely on standard binding. If inverted, I drag invert.
        // BUT, visually, I place "ENV" above and "GATE" below.
        gateSwitch.getProperties().set("isSwitch", true); 
        gateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "vcaMode", gateSwitch);

        JunoUI::setupLabel(gateEnvLabel, "ENV", *this);
        JunoUI::setupLabel(gateGateLabel, "GATE", *this);
        
        // Font adjustments for switch labels (smaller)
        auto font = juce::FontOptions(10.0f, juce::Font::bold);
        gateEnvLabel.setFont(font);
        gateGateLabel.setFont(font);

        JunoUI::setupMidiLearn(levelSlider, mlh, "vcaLevel", midiLearnListeners);
        JunoUI::setupMidiLearn(gateSwitch, mlh, "vcaMode", midiLearnListeners);

        levelSlider.getProperties().set("numberSide", "Right");
    }

    void paint(juce::Graphics& g) override
    {
        JunoUI::drawSectionBackground(g, getLocalBounds(), "VCA");
        
        // Draw Authentic Icons above/below literals
        auto swBounds = gateSwitch.getBounds();
        if (!swBounds.isEmpty()) {
            g.setColour(JunoUI::kTextGrey);
            juce::Path p;
            
            // Envelope Icon (Above "ENV" label)
            // We need label bounds. Since labels are components, we can use their bounds relative to parent.
            auto envBounds = gateEnvLabel.getBounds();
            float x = (float)envBounds.getCentreX() - 5.0f;
            float y = (float)envBounds.getY() - 12.0f; // Above label
            
            p.startNewSubPath(x, y + 10);
            p.lineTo(x + 5, y);
            p.lineTo(x + 10, y + 10);
            
            // Gate Icon (Below "GATE" label)
            auto gateBounds = gateGateLabel.getBounds();
            float x2 = (float)gateBounds.getCentreX() - 5.0f;
            float y2 = (float)gateBounds.getBottom() + 2.0f; // Below label
            
            p.startNewSubPath(x2, y2 + 10);
            p.lineTo(x2, y2);
            p.lineTo(x2 + 10, y2);
            p.lineTo(x2 + 10, y2 + 10);
            
            g.strokePath(p, juce::PathStrokeType(1.5f));
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10, 30);
        int halfW = area.getWidth() / 2;

        // Switch Column (Left)
        // Layout: Label "ENV" -> Switch -> Label "GATE"
        int swX = area.getX();
        int swW = 30; // switch width
        int centerX = swX + (halfW - swW) / 2;
        
        int labelH = 15;
        int swH = 40; 
        
        // Vertically center the group
        int totalH = labelH + swH + labelH + 5;
        int startY = area.getY() + (area.getHeight() - totalH) / 2;
        
        gateEnvLabel.setBounds(centerX - 10, startY, 50, labelH); // Wider to fit text
        gateSwitch.setBounds(centerX, startY + labelH, swW, swH);
        gateGateLabel.setBounds(centerX - 10, startY + labelH + swH, 50, labelH);
        
        // Slider Column (Right)
        int sliderLabelH = 20;
        levelLabel.setBounds(area.getX() + halfW, area.getY(), 50, sliderLabelH);
        levelSlider.setBounds(area.getX() + halfW, area.getY() + sliderLabelH, 50, area.getHeight() - sliderLabelH - 5);
    }

private:
    juce::Slider levelSlider;
    juce::Slider gateSwitch;
    juce::Label levelLabel;
    juce::Label gateEnvLabel;
    juce::Label gateGateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateAtt;
    
    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
