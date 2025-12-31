#pragma once

#include <JuceHeader.h>

/**
 * JunoBender - Modular Bender Component
 * 
 * Authentic Juno-106 bender panel with:
 * - Horizontal bender lever (-1 to +1)
 * - 3 destination sliders (DCO, VCF, LFO)
 * 
 * This is a self-contained UI component that can be reused
 */
class JunoBender : public juce::Component {
public:
    JunoBender();
    ~JunoBender() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Attach to APVTS parameters
    void attachToParameters(juce::AudioProcessorValueTreeState& apvts);
    
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    // Bender lever (horizontal)
    juce::Slider benderLever;
    std::unique_ptr<SliderAttachment> benderAttachment;
    juce::Label benderLabel;
    
    // Destination sliders (vertical)
    juce::Slider dcoSlider;
    juce::Slider vcfSlider;
    juce::Slider lfoSlider;
    
    std::unique_ptr<SliderAttachment> dcoAttachment;
    std::unique_ptr<SliderAttachment> vcfAttachment;
    std::unique_ptr<SliderAttachment> lfoAttachment;
    
    juce::Label dcoLabel;
    juce::Label vcfLabel;
    juce::Label lfoLabel;
    
    void setupSlider(juce::Slider& slider, juce::Slider::SliderStyle style);
    void setupLabel(juce::Label& label, const juce::String& text);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JunoBender)
};
