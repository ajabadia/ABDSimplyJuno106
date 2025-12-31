#pragma once

#include <JuceHeader.h>
#include "../Core/SynthParams.h"
#include "JunoDCO.h"
#include "JunoLFO.h"
#include "JunoADSR.h"

/**
 * Voice - Single voice for SimpleJuno106
 * 
 * Uses JUCE built-in components + Juno-106 authentic modules:
 * - JunoDCO for oscillator (RANGE, PWM modes, Noise, authentic)
 * - juce::dsp::LadderFilter for VCF
 * - JunoVCO for oscillator (with drift)
 * - JunoLFO for modulation (with delay)
 */
class Voice {
public:
    Voice();
    
    // Analog Variance (Mojo)
    struct Variance {
        float filterCutoffScale = 1.0f; // +/- 5%
        float filterResScale = 1.0f;    // +/- 5%
        float envTimeScale = 1.0f;      // +/- 2%
        float pwOffset = 0.0f;          // +/- 0.02
    };
    
    void setVariance(const Variance& v) { variance = v; }
    void prepare(double sampleRate, int maxBlockSize);
    void noteOn(int midiNote, float velocity);
    void noteOff();
    
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    void updateParams(const SynthParams& params);
    
    bool isActive() const { return adsr.isActive(); }
    int getCurrentNote() const { return currentNote; }
    
private:
    // State
    double sampleRate = 44100.0;
    int currentNote = -1;
    float velocity = 0.0f;
    
    // Portamento
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    
    // Juno modules
    JunoDCO dco;
    JunoLFO lfo;
    
    // Juno ADSR (linear ramps, authentic)
    JunoADSR adsr;
    
    // JUCE Ladder Filter
    juce::dsp::LadderFilter<float> filter;
    juce::dsp::IIR::Filter<float> hpFilter;  // Juno-106 HPF
    
    // Cached params
    SynthParams params;
    
    // Helper methods
    void updateHPF();
    
    Variance variance;
};
