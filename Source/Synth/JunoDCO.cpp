// Source/Synth/JunoDCO.cpp
#include "JunoDCO.h"
#include <cmath>

JunoDCO::JunoDCO() {
    // Sawtooth using JUCE Oscillator
    sawOsc.initialise([](float x) {
        return juce::jmap(x, 
                         -juce::MathConstants<float>::pi, 
                          juce::MathConstants<float>::pi, 
                         -1.0f, 1.0f);
    }, 128);
    
    updateRangeMultiplier();
    reset();
}

void JunoDCO::prepare(double sr, int maxBlockSize) {
    sampleRate = sr;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;
    
    sawOsc.prepare(spec);
    reset();
}

void JunoDCO::reset() {
    sawOsc.reset();
    pulsePhase = 0.0;
    driftMigrator = 0.0f;
    driftTarget = 0.0f;
    driftCounter = 0;
    currentPWM = pwmValue;
    subFlipFlop = false;
}

void JunoDCO::setFrequency(float hz) {
    baseFrequency = hz;
}

void JunoDCO::setRange(Range r) {
    range = r;
    updateRangeMultiplier();
}

void JunoDCO::updateRangeMultiplier() {
    switch (range) {
        case Range::Range16: rangeMultiplier = 0.5f; break;  // -1 octave
        case Range::Range8:  rangeMultiplier = 1.0f; break;  // Normal
        case Range::Range4:  rangeMultiplier = 2.0f; break;  // +1 octave
    }
}

void JunoDCO::setPulseLevel(float level) {
    pulseLevel = juce::jlimit(0.0f, 1.0f, level);
}

void JunoDCO::setSawLevel(float level) {
    sawLevel = juce::jlimit(0.0f, 1.0f, level);
}

void JunoDCO::setSubLevel(float level) {
    subLevel = juce::jlimit(0.0f, 1.0f, level);
}

void JunoDCO::setNoiseLevel(float level) {
    noiseLevel = juce::jlimit(0.0f, 1.0f, level);
}

void JunoDCO::setPWM(float value) {
    pwmValue = juce::jlimit(0.0f, 1.0f, value);
}

void JunoDCO::setPWMMode(PWMMode mode) {
    pwmMode = mode;
}

void JunoDCO::setLFODepth(float depth) {
    lfoDepth = juce::jlimit(0.0f, 1.0f, depth);
}

void JunoDCO::setDrift(float amount) {
    driftAmount = juce::jlimit(0.0f, 1.0f, amount);
}

float JunoDCO::getNextSample(float lfoValue) {
    // === ANALOG DRIFT (Random Walk) ===
    // Update target occasionally to create "wandering" pitch
    if (++driftCounter > 1000) {
        driftCounter = 0;
        // Random walk target between -1.0 and 1.0
        driftTarget = (noiseGen.nextFloat() * 2.0f - 1.0f); 
    }
    
    // Smoothly migrate towards target (Simulate thermal capacitance/instability)
    driftMigrator += (driftTarget - driftMigrator) * 0.005f;
    
    // Scale by user drift amount (Max 15 cents equivalent)
    float driftSemitones = driftMigrator * driftAmount * 0.15f; 
    
    // === FREQUENCY with RANGE, LFO, and DRIFT ===
    float freq = baseFrequency * rangeMultiplier;
    
    // Apply LFO to pitch (vibrato)
    float lfoSemitones = lfoValue * lfoDepth * 0.5f; // LFO max range ~ half semitone (checked/reduced)
    freq *= std::pow(2.0f, (lfoSemitones + driftSemitones) / 12.0f);
    
    // Nyquist check
    if (freq >= sampleRate * 0.49f) {
        freq = static_cast<float>(sampleRate * 0.49);
    }
    
    sawOsc.setFrequency(freq);
    
    // === UPDATE PHASE (Must happen even if Pulse is OFF for Sub Osc) ===
    double dt = freq / sampleRate;
    pulsePhase += dt;
    if (pulsePhase >= 1.0) {
        pulsePhase -= 1.0;
        subFlipFlop = !subFlipFlop; // Authentic: Sub is derived from DCO clock
    }
    
    float output = 0.0f;
    
    // === 1. SAWTOOTH (JUCE) ===
    if (sawLevel > 0.0f) {
        float saw = sawOsc.processSample(0.0f);
        output += saw * sawLevel;
    }
    
    // === 2. PULSE with PWM ===
    if (pulseLevel > 0.0f) {
        // Calculate target PWM based on mode
        float targetPWM = 0.5f;
        
        if (pwmMode == PWMMode::Manual) {
            targetPWM = juce::jlimit(0.05f, 0.95f, pwmValue);
        } else {
            // LFO varies around 50% +/- depth
            float pwmDepth = pwmValue;
            targetPWM = juce::jlimit(0.05f, 0.95f, 0.5f + lfoValue * pwmDepth * 0.45f);
        }
        
        // Slew PWM
        currentPWM += (targetPWM - currentPWM) * 0.01f;
        
        // Generate pulse
        float pulse = (pulsePhase < currentPWM) ? 1.0f : -1.0f;
        output += pulse * pulseLevel;
    }
    
    // === 3. SUB-OSCILLATOR ===
    if (subLevel > 0.0f) {
        float sub = subFlipFlop ? 1.0f : -1.0f;
        output += sub * subLevel;
    }
    
    // === 4. NOISE ===
    if (noiseLevel > 0.0f) {
        float noise = noiseGen.nextFloat() * 2.0f - 1.0f;
        output += noise * noiseLevel;
    }
    
    return output * 0.5f; 
}
