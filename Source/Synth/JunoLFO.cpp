// Source/Synth/JunoLFO.cpp
#include "JunoLFO.h"
#include <cmath>

JunoLFO::JunoLFO() {
    // Initialize sine wave (authentic Juno-106)
    oscillator.initialise([](float x) { 
        return std::sin(x); 
    }, 128);
}

void JunoLFO::prepare(double sr, int maxBlockSize) {
    sampleRate = sr;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;
    
    oscillator.prepare(spec);
    oscillator.setFrequency(rate);
    reset();
}

void JunoLFO::reset() {
    oscillator.reset();
    delayTimer = 0.0f;
    delayEnvelope = 0.0f;
    currentValue = 0.0f;
    noteActive = false;
}

void JunoLFO::setRate(float hz) {
    rate = juce::jlimit(0.1f, 30.0f, hz);
    oscillator.setFrequency(rate);
}

void JunoLFO::setDepth(float amount) {
    depth = juce::jlimit(0.0f, 1.0f, amount);
}

void JunoLFO::setDelay(float seconds) {
    delay = juce::jlimit(0.0f, 3.0f, seconds);
}

void JunoLFO::noteOn() {
    noteActive = true;
    delayTimer = 0.0f;
    delayEnvelope = 0.0f;
}

void JunoLFO::noteOff() {
    noteActive = false;
}

float JunoLFO::getNextSample() {
    // Update delay envelope (fade-in)
    if (noteActive && delayEnvelope < 1.0f) {
        if (delay > 0.0f) {
            delayTimer += 1.0f / static_cast<float>(sampleRate);
            delayEnvelope = juce::jlimit(0.0f, 1.0f, delayTimer / delay);
        } else {
            delayEnvelope = 1.0f;
        }
    }
    
    // Get LFO sample (sine wave)
    float lfoSample = oscillator.processSample(0.0f);
    
    // Apply depth and delay envelope
    currentValue = lfoSample * depth * delayEnvelope;
    
    return currentValue;
}
