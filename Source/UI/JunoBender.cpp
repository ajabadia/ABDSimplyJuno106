#include "JunoBender.h"

JunoBender::JunoBender() {
    // Setup bender lever (horizontal)
    setupSlider(benderLever, juce::Slider::LinearHorizontal);
    benderLever.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setupLabel(benderLabel, "BENDER");
    
    // Setup destination sliders (vertical)
    setupSlider(dcoSlider, juce::Slider::LinearVertical);
    setupLabel(dcoLabel, "DCO");
    
    setupSlider(vcfSlider, juce::Slider::LinearVertical);
    setupLabel(vcfLabel, "VCF");
    
    setupSlider(lfoSlider, juce::Slider::LinearVertical);
    setupLabel(lfoLabel, "LFO");
}

void JunoBender::attachToParameters(juce::AudioProcessorValueTreeState& apvts) {
    benderAttachment = std::make_unique<SliderAttachment>(apvts, "bender", benderLever);
    dcoAttachment = std::make_unique<SliderAttachment>(apvts, "benderToDCO", dcoSlider);
    vcfAttachment = std::make_unique<SliderAttachment>(apvts, "benderToVCF", vcfSlider);
    lfoAttachment = std::make_unique<SliderAttachment>(apvts, "benderToLFO", lfoSlider);
}

void JunoBender::setupSlider(juce::Slider& slider, juce::Slider::SliderStyle style) {
    addAndMakeVisible(slider);
    slider.setSliderStyle(style);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
}

void JunoBender::setupLabel(juce::Label& label, const juce::String& text) {
    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colours::lightblue);
}

void JunoBender::paint(juce::Graphics& g) {
    // Background
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    
    // Title
    g.setColour(juce::Colours::lightblue);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("BENDER", 0, 5, getWidth(), 20, juce::Justification::centred);
    
    // Divider line
    g.setColour(juce::Colour(0xff4a4a4a));
    g.drawLine(10.0f, 25.0f, (float)getWidth() - 10.0f, 25.0f, 1.0f);
}

void JunoBender::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromTop(30); // Title space
    
    int padding = 10;
    bounds.reduce(padding, padding);
    
    // Layout: [DCO] [VCF] [LFO] sliders on left, bender lever on right
    int sliderWidth = 30;
    int sliderSpacing = 10;
    int totalSliderWidth = (sliderWidth * 3) + (sliderSpacing * 2);
    
    // Destination sliders (vertical, left side)
    auto sliderArea = bounds.removeFromLeft(totalSliderWidth);
    
    auto dcoArea = sliderArea.removeFromLeft(sliderWidth);
    dcoSlider.setBounds(dcoArea.removeFromTop(bounds.getHeight() - 25));
    dcoLabel.setBounds(dcoArea);
    
    sliderArea.removeFromLeft(sliderSpacing);
    
    auto vcfArea = sliderArea.removeFromLeft(sliderWidth);
    vcfSlider.setBounds(vcfArea.removeFromTop(bounds.getHeight() - 25));
    vcfLabel.setBounds(vcfArea);
    
    sliderArea.removeFromLeft(sliderSpacing);
    
    auto lfoArea = sliderArea.removeFromLeft(sliderWidth);
    lfoSlider.setBounds(lfoArea.removeFromTop(bounds.getHeight() - 25));
    lfoLabel.setBounds(lfoArea);
    
    // Bender lever (horizontal, right side)
    bounds.removeFromLeft(15); // Spacing
    
    auto leverArea = bounds;
    benderLever.setBounds(leverArea.removeFromBottom(40).reduced(0, 5));
    benderLabel.setBounds(leverArea.removeFromBottom(20));
}
