// Source/Synth/JunoADSR.cpp
#include "JunoADSR.h"
#include <algorithm>

JunoADSR::JunoADSR() {
    reset();
}

void JunoADSR::setSampleRate(double sr) {
    sampleRate = sr;
    calculateRates();
}

void JunoADSR::reset() {
    stage = Stage::Idle;
    currentValue = 0.0f;
}

void JunoADSR::setAttack(float seconds) {
    attackTime = std::max(0.001f, seconds);
    calculateRates();
}

void JunoADSR::setDecay(float seconds) {
    decayTime = std::max(0.001f, seconds);
    calculateRates();
}

void JunoADSR::setSustain(float level) {
    sustainLevel = juce::jlimit(0.0f, 1.0f, level);
    calculateRates();
}

void JunoADSR::setRelease(float seconds) {
    releaseTime = std::max(0.001f, seconds);
    calculateRates();
}

void JunoADSR::setGateMode(bool enabled) {
    gateMode = enabled;
}

void JunoADSR::calculateRates() {
    if (sampleRate <= 0) return;
    
    // Attack: Exponential approach
    // We want to reach target in approx 'attackTime' seconds. 
    // Time Constant (Tau) ~ Time / 5?
    // Using a simple coefficient: coeff = 1 - exp(-1 / (time * sr))
    // Or approximate for small coeffs: coeff = 1 / (time * sr) * Factor
    
    // Factor 3.0 means we reach 95% in 'time'.
    
    float attackSamples = attackTime * static_cast<float>(sampleRate);
    attackRate = 1.0f - std::exp(-3.0f / attackSamples);
    if (attackSamples < 1.0f) attackRate = 1.0f;
    
    // Decay
    float decaySamples = decayTime * static_cast<float>(sampleRate);
    decayRate = 1.0f - std::exp(-3.0f / decaySamples);
    if (decaySamples < 1.0f) decayRate = 1.0f;
    
    // Release
    float releaseSamples = releaseTime * static_cast<float>(sampleRate);
    releaseRate = 1.0f - std::exp(-3.0f / releaseSamples);
    if (releaseSamples < 1.0f) releaseRate = 1.0f;
}

void JunoADSR::noteOn() {
    stage = Stage::Attack;
    // Don't reset currentValue - allows legato/retriggering
}

void JunoADSR::noteOff() {
    if (stage != Stage::Idle) {
        stage = Stage::Release;
    }
}

float JunoADSR::getNextSample() {
    // If Gate Mode is active, bypass ADSR phases and act as a simple Gate
    if (gateMode) {
        if (stage == Stage::Release || stage == Stage::Idle) {
            currentValue = 0.0f;
            stage = Stage::Idle;
        } else {
            currentValue = 1.0f;
        }
        return currentValue;
    }

    // Authentic Analog ADSR (approximated with Exponential Curves)
    // Target levels
    float target = 0.0f;
    float rate = 0.0f;

    switch (stage) {
        case Stage::Idle:
            currentValue = 0.0f;
            return 0.0f;
            
        case Stage::Attack:
            target = 1.01f; // Overshoot for fast exponential attack
            rate = attackRate;
            currentValue += (target - currentValue) * rate;
            if (currentValue >= 1.0f) {
                currentValue = 1.0f;
                stage = Stage::Decay;
            }
            break;
            
        case Stage::Decay:
            target = sustainLevel;
            rate = decayRate;
            currentValue += (target - currentValue) * rate;
            // Epsilon check to enter sustain (if close enough)
            if (std::abs(currentValue - target) < 0.001f) {
                currentValue = target;
                stage = Stage::Sustain;
            }
            break;
            
        case Stage::Sustain:
            currentValue = sustainLevel;
            break;
            
        case Stage::Release:
            target = 0.0f;
            rate = releaseRate;
            currentValue += (target - currentValue) * rate;
            if (currentValue < 0.001f) {
                currentValue = 0.0f;
                stage = Stage::Idle;
            }
            break;
    }
    
    return currentValue;
}
