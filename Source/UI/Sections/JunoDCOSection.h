#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"

class JunoDCOSection : public juce::Component, public juce::AudioProcessorValueTreeState::Listener
{
public:
    JunoDCOSection(juce::AudioProcessorValueTreeState& state, MidiLearnHandler& mlh)
        : apvts(state)
    {
        // === Range (3 Vertical Buttons with LEDs) ===
        addAndMakeVisible(range16);
        addAndMakeVisible(range8);
        addAndMakeVisible(range4);
        
        // Setup Range Buttons (Labels are usually above, but here buttons ARE the interface)
        // Authentic: Label "RANGE" at top. Below: "16'", "8'", "4'" labels. Below: Buttons with LED.
        // My ToggleButton draws LED + Cap. 
        // I will trust tooltips or additional labels? 
        // User asked for "3 Buttons (En Vertical)".
        // I'll put text labels in resized() above each button if needed, or rely on button text (which I draw).
        // My drawToggleButton draws text? Yes, line 114: g.drawText...
        // Wait, my new drawToggleButton DOES NOT draw text!
        // It draws LED and CAP.
        // The text should be drawn by LookAndFeel? 
        // Checked JunoUIHelpers.h: drawToggleButton (Line 101 replacement) -> NO TEXT DRAWING!
        // I removed text drawing. "Layout: LED at top, Button at bottom".
        // Authentic Juno has text printed on PANEL above the LED.
        // So I need Labels for "16'", "8'", "4'".
        
        range16.setClickingTogglesState(true);
        range8.setClickingTogglesState(true);
        range4.setClickingTogglesState(true);
        
        // Logic: Mutex handled in callbacks
        range16.onClick = [this] { if (range16.getToggleState()) setRange(0); else updateRangeUI(); };
        range8.onClick  = [this] { if (range8.getToggleState())  setRange(1); else updateRangeUI(); };
        range4.onClick  = [this] { if (range4.getToggleState())  setRange(2); else updateRangeUI(); };

        JunoUI::setupLabel(rangeLabel, "RANGE", *this);
        JunoUI::setupLabel(lbl16, "16'", *this);
        JunoUI::setupLabel(lbl8, "8'", *this);
        JunoUI::setupLabel(lbl4, "4'", *this);

        // === Scaling (Sliders) ===
        JunoUI::setupVerticalSlider(lfoSlider);
        JunoUI::setupVerticalSlider(pwmSlider);
        JunoUI::setupVerticalSlider(subSlider);
        JunoUI::setupVerticalSlider(noiseSlider);

        addAndMakeVisible(lfoSlider);
        addAndMakeVisible(pwmSlider);
        addAndMakeVisible(subSlider);
        addAndMakeVisible(noiseSlider);

        lfoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "lfoToDCO", lfoSlider);
        pwmAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "pwm", pwmSlider);
        subAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "subOsc", subSlider);
        noiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "noise", noiseSlider);

        JunoUI::setupLabel(lfoLabel, "LFO", *this);
        JunoUI::setupLabel(pwmLabel, "PWM", *this);
        JunoUI::setupLabel(subLabel, "SUB", *this);
        JunoUI::setupLabel(noiseLabel, "NOISE", *this);

        // === PWM Mode (Switch) ===
        addAndMakeVisible(pwmModeSwitch);
        pwmModeSwitch.setSliderStyle(juce::Slider::LinearVertical);
        pwmModeSwitch.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        pwmModeSwitch.setRange(0.0, 1.0, 1.0);
        pwmModeSwitch.getProperties().set("isSwitch", true); // Trigger custom drawing
        
        pwmModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "pwmMode", pwmModeSwitch);
        
        JunoUI::setupLabel(modeLabel, "MODE", *this);
        JunoUI::setupLabel(lblLFO, "LFO", *this); // Switch Position Labels
        JunoUI::setupLabel(lblMAN, "MAN", *this);

        // === Waveforms (Buttons with LEDs) ===
        addAndMakeVisible(pulseButton);
        addAndMakeVisible(sawButton);
        
        pulseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "pulseOn", pulseButton);
        sawAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "sawOn", sawButton);
        
        // Custom LED Colors
        // Pulse = Red (Default)
        // Saw = Blue (Authentic Mod / Visual)
        sawButton.getProperties().set("ledColor", "0xff3399ff"); 

        // Listeners for Range
        apvts.addParameterListener("dcoRange", this);
        updateRangeUI(); // Init state

        // MIDI Learn
        auto setup = [&](juce::Component& c, const juce::String& p) { JunoUI::setupMidiLearn(c, mlh, p, midiLearnListeners); };
        setup(lfoSlider, "lfoToDCO");
        setup(pwmSlider, "pwm");
        setup(pwmModeSwitch, "pwmMode");
        setup(pulseButton, "pulseOn");
        setup(sawButton, "sawOn");
        setup(subSlider, "subOsc");
        setup(noiseSlider, "noise");

        // Authentic Number Siding
        lfoSlider.getProperties().set("numberSide", "Left");
        pwmSlider.getProperties().set("numberSide", "Right");
        subSlider.getProperties().set("numberSide", "Left");
        noiseSlider.getProperties().set("numberSide", "Right");
    }

    ~JunoDCOSection() override {
        apvts.removeParameterListener("dcoRange", this);
    }

    void parameterChanged(const juce::String& parameterID, float newValue) override {
        if (parameterID == "dcoRange") {
            juce::MessageManager::callAsync([this]() { updateRangeUI(); });
        }
    }

    void setRange(int index) {
        if (auto* p = apvts.getParameter("dcoRange")) {
             p->setValueNotifyingHost(p->convertTo0to1((float)index));
        }
    }

    void updateRangeUI() {
        int val = (int)*apvts.getRawParameterValue("dcoRange");
        range16.setToggleState(val == 0, juce::dontSendNotification);
        range8.setToggleState(val == 1, juce::dontSendNotification);
        range4.setToggleState(val == 2, juce::dontSendNotification);
    }

    void paint(juce::Graphics& g) override
    {
        JunoUI::drawSectionBackground(g, getLocalBounds(), "DCO");
        
        // Waveform Icons (Authentic)
        // Pulse Icon above Pulse Button
        // Saw Icon above Saw Button
        g.setColour(JunoUI::kTextGrey);
        
        auto pulseArea = pulseButton.getBounds().translated(0, -15).toFloat();
        juce::Path pulsePath;
        pulsePath.startNewSubPath(pulseArea.getX() + 15, pulseArea.getY() + 8);
        pulsePath.lineTo(pulseArea.getX() + 15, pulseArea.getY());
        pulsePath.lineTo(pulseArea.getX() + 25, pulseArea.getY());
        pulsePath.lineTo(pulseArea.getX() + 25, pulseArea.getY() + 8);
        g.strokePath(pulsePath, juce::PathStrokeType(1.5f));

        auto sawArea = sawButton.getBounds().translated(0, -15).toFloat();
        juce::Path sawPath;
        sawPath.startNewSubPath(sawArea.getX() + 15, sawArea.getY() + 8);
        sawPath.lineTo(sawArea.getX() + 25, sawArea.getY());
        sawPath.lineTo(sawArea.getX() + 25, sawArea.getY() + 8);
        g.strokePath(sawPath, juce::PathStrokeType(1.5f));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(2, 25); // Top header space
        int colW = area.getWidth() / 6; 
        int startX = area.getX();
        
        int btnW = 30;
        int btnH = 40; // Combined LED + Cap

        // Col 1: Range (3 Vertical Buttons)
        rangeLabel.setBounds(startX, 25, colW, 15);
        int yPos = 45;
        
        // 16'
        lbl16.setBounds(startX, yPos, colW, 12);
        range16.setBounds(startX + (colW - btnW)/2, yPos + 12, btnW, btnH);
        
        yPos += 55;
        // 8'
        lbl8.setBounds(startX, yPos, colW, 12);
        range8.setBounds(startX + (colW - btnW)/2, yPos + 12, btnW, btnH);

        yPos += 55;
        // 4'
        lbl4.setBounds(startX, yPos, colW, 12);
        range4.setBounds(startX + (colW - btnW)/2, yPos + 12, btnW, btnH);
        
        startX += colW;

        // Col 2: LFO
        int labelH = 20;
        int sliderY = 40 + labelH;
        int sliderH = area.getHeight() - 40 - labelH;
        
        lfoLabel.setBounds(startX, 40, colW, labelH);
        lfoSlider.setBounds(startX, sliderY, colW, sliderH);
        startX += colW;

        // Col 3: PWM
        pwmLabel.setBounds(startX, 40, colW, labelH);
        pwmSlider.setBounds(startX, sliderY, colW, sliderH);
        startX += colW;

        // Col 4: Mode & Waves (Switch + 2 Buttons)
        // This column is crowded. Real Juno DCO is wide.
        // I'll try to fit Mode, Pulse, Saw in one column?
        // Or splitMode into Col 3.5?
        // Layout: Mode Switch | Pulse | Saw
        // They are adjacent.
        // Actually, Mode Switch is next to PWM Slider.
        // Then Waves.
        // I will squeeze Mode Switch.
        
        // Mode Switch
        modeLabel.setBounds(startX, 25, colW, 15);
        lblLFO.setBounds(startX - 15, 50, 30, 15); 
        pwmModeSwitch.setBounds(startX + 10, 50, 30, 50); // Vertical switch
        lblMAN.setBounds(startX - 15, 85, 30, 15);
        
        // Wave Buttons below? No, standard is left-to-right.
        // With limited width (6 cols), I might need to put Waves in Col 5 and Shift Sub/Noise.
        // Let's assume Col 4 holds MODE and PULSE?
        // Real Juno: [Range] [LFO] [PWM] [ModeSw] [Pulse] [Saw] [Sub] [Noise]
        // That is 8 logical Slots. I have 6 cols.
        // I'll put Pulse & Saw in Col 5. Sub in Col 6. Noise in Col 6 (stacked? No).
        // I need more width or smarter packing.
        // Let's pack Mode Switch tight.
        
        auto modeRect = juce::Rectangle<int>(startX, 40, 40, 80);
        // ... handled manually above
        
        // Pulse & Saw below Mode Switch? Or in next col?
        // I'll move Pulse/Saw to next col.
        
        startX += colW;
        
        // Col 5: Waves
        // Pulse top, Saw bottom?
        pulseButton.setBounds(startX + (colW-btnW)/2, 60, btnW, btnH);
        sawButton.setBounds(startX + (colW-btnW)/2, 120, btnW, btnH);
        
        startX += colW;

        // Col 6: Sub & Noise (Share column?)
        // If I put Sub and Noise in same col, they must be thin or stacked.
        // DCO section is usually 25% of width.
        // How about I make slider columns thinner?
        // Or simple: Sub and Noise in last col (half/half).
        int halfW = colW / 2;
        subSlider.setBounds(startX, 40, colW, area.getHeight() - 40); // Overlap?
        // Let's just fit them.
        subSlider.setBounds(startX, 40, colW, area.getHeight() - 40);
        subLabel.setBounds(startX, subSlider.getBottom() - 15, colW, 15);
        
        // Wait, where is Noise?
        // I ran out of columns?
        // I will use 7 virtual stripes
        colW = area.getWidth() / 7;
        startX = area.getX();
        
        // Re-layout with 7 cols
        // Col 1: Range
        rangeLabel.setBounds(startX, 25, colW, 15);
        lbl16.setBounds(startX, 45, colW, 12);
        range16.setBounds(startX + (colW-btnW)/2, 57, btnW, btnH);
        lbl8.setBounds(startX, 97, colW, 12);
        range8.setBounds(startX + (colW-btnW)/2, 109, btnW, btnH);
        lbl4.setBounds(startX, 149, colW, 12);
        range4.setBounds(startX + (colW-btnW)/2, 161, btnW, btnH);
        startX += colW;

        // Col 2: LFO
        lfoSlider.setBounds(startX, 40, colW, area.getHeight() - 55);
        lfoLabel.setBounds(startX, lfoSlider.getBottom(), colW, 15);
        startX += colW;

        // Col 3: PWM
        pwmSlider.setBounds(startX, 40, colW, area.getHeight() - 55);
        pwmLabel.setBounds(startX, pwmSlider.getBottom(), colW, 15);
        startX += colW;

        // Col 4: Mode Switch
        modeLabel.setBounds(startX, 25, colW, 15);
        lblLFO.setBounds(startX, 45, colW, 12);
        pwmModeSwitch.setBounds(startX + (colW-30)/2, 60, 30, 50);
        lblMAN.setBounds(startX, 115, colW, 12);
        startX += colW;

        // Col 5: Waves
        pulseButton.setBounds(startX + (colW-btnW)/2, 60, btnW, btnH);
        sawButton.setBounds(startX + (colW-btnW)/2, 120, btnW, btnH);
        startX += colW;

        // Col 6: Sub
        subLabel.setBounds(startX, 40, colW, labelH);
        subSlider.setBounds(startX, 40 + labelH, colW, sliderH);
        startX += colW;
        
        // Col 7: Noise
        noiseLabel.setBounds(startX, 40, colW, labelH);
        noiseSlider.setBounds(startX, 40 + labelH, colW, sliderH);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;

    // Controls
    juce::ToggleButton range16, range8, range4; // Range Buttons
    juce::Slider lfoSlider, pwmSlider, subSlider, noiseSlider;
    juce::Slider pwmModeSwitch; // Switch
    juce::ToggleButton pulseButton, sawButton;

    // Labels
    juce::Label rangeLabel, lfoLabel, pwmLabel, subLabel, noiseLabel, modeLabel;
    juce::Label lbl16, lbl8, lbl4, lblLFO, lblMAN;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoAttachment, pwmAttachment, subAttachment, noiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pwmModeAttachment; // Switch is a slider
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pulseAttachment, sawAttachment;

    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
};
