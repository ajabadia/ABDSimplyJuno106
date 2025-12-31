// Source/Synth/JunoADSR.h
#pragma once

#include <JuceHeader.h>

/**
 * JunoADSR - Authentic Juno-106 ADSR Envelope
 * 
 * CHARACTERISTICS:
 * - Linear ramps (not exponential) - authentic Juno-106
 * - Fast, responsive envelopes
 * - GATE mode support (ENV button)
 * - 5-Stage State Machine: Idle, Attack, Decay, Sustain, Release
 * 
 * IMPLEMENTATION:
 * - Uses only JUCE standard components
 * - Per-sample processing for stability
 * - Simple linear math (no pow/exp)
 */
class JunoADSR {
public:
    enum class Stage {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };
    
    JunoADSR();
    
    // Setup
    void setSampleRate(double sampleRate);
    void reset();
    
    // Parameters (seconds)
    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);      // 0-1
    void setRelease(float seconds);
    void setGateMode(bool enabled);    // ENV button
    
    // Lifecycle
    void noteOn();
    void noteOff();
    
    // Processing
    float getNextSample();
    bool isActive() const { return stage != Stage::Idle; }
    
    Stage getCurrentStage() const { return stage; }
    float getCurrentValue() const { return currentValue; }
    
private:
    double sampleRate = 44100.0;
    
    // Parameters
    float attackTime = 0.01f;      // seconds
    float decayTime = 0.3f;        // seconds
    float sustainLevel = 0.7f;     // 0-1
    float releaseTime = 0.5f;      // seconds
    bool gateMode = false;         // ENV button state
    
    // Linear rates (per-sample increment/decrement)
    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float releaseRate = 0.0f;
    
    // State
    Stage stage = Stage::Idle;
    float currentValue = 0.0f;
    
    // Helper
    void calculateRates();
};
