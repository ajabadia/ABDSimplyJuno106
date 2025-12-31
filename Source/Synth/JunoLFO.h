// Source/Synth/JunoLFO.h
#pragma once

#include <JuceHeader.h>

/**
 * JunoLFO - Authentic Juno-106 Low Frequency Oscillator
 * 
 * CHARACTERISTICS:
 * - Sine wave only (authentic Juno-106)
 * - Rate: 0.1 Hz to 30 Hz
 * - Delay: 0 to 3 seconds (fade-in time)
 * - Destinations: PWM, Filter, DCO (pitch), VCA (amplitude)
 * 
 * IMPLEMENTATION:
 * - Uses juce::dsp::Oscillator (JUCE standard component)
 * - Modular architecture (separate from Voice)
 */
class JunoLFO {
public:
    JunoLFO();
    
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    
    // Parameters
    void setRate(float hz);          // 0.1 - 30 Hz
    void setDepth(float amount);     // 0 - 1
    void setDelay(float seconds);    // 0 - 5 seconds
    
    float getRate() const { return rate; }
    float getDepth() const { return depth; }
    float getDelay() const { return delay; }
    
    // Processing
    void noteOn();
    void noteOff();
    float getNextSample();
    float getCurrentValue() const { return currentValue; }
    
private:
    juce::dsp::Oscillator<float> oscillator;
    double sampleRate = 44100.0;
    
    float rate = 5.0f;               // Hz
    float depth = 1.0f;              // 0-1
    float delay = 0.0f;              // seconds
    
    // Delay envelope
    float delayTimer = 0.0f;
    float delayEnvelope = 0.0f;
    bool noteActive = false;
    
    float currentValue = 0.0f;
};
