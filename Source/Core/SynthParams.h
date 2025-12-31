#pragma once

#include <JuceHeader.h>

/**
 * SynthParams - Parameter definitions for SimpleJuno106
 * 
 * Simple, flat structure - no complex modular architecture
 */
struct SynthParams {
    // DCO (Complete Juno-106 authentic)
    int dcoRange = 1;           // 0=16', 1=8', 2=4' (octave selector)
    bool sawOn = true;          // On/Off (Authentic: Switched, not mixed)
    bool pulseOn = true;        // On/Off (Authentic: Switched)
    float pwmAmount = 0.5f;     // 0-1 (pulse width or PWM depth)
    int pwmMode = 0;            // 0=MAN, 1=LFO
    float subOscLevel = 0.0f;   // 0-1
    float noiseLevel = 0.0f;    // 0-1 (NEW!)
    float lfoToDCO = 0.0f;      // 0-1 (LFO modulation to pitch) (NEW!)
    
    // VCF
    float vcfFreq = 0.8f;       // 0-1 (mapped to Hz in Voice)
    float resonance = 0.0f;     // 0-1
    float envAmount = 0.5f;     // 0-1 (envelope modulation depth)
    
    // ADSR
    float attack = 0.01f;       // seconds
    float decay = 0.3f;         // seconds
    float sustain = 0.7f;       // 0-1
    float release = 0.5f;       // seconds
    
    // LFO (Juno-106 authentic)
    float lfoRate = 2.0f;       // 0.1 to 30 Hz
    // lfoDepth removed - Authenticity fix: Global depth doesn't exist.
    // Depths are controlled per-section (DCO, VCF, PWM).
    float lfoDelay = 0.0f;      // 0-5 seconds (fade-in time)
    // lfoDestination removed - authentic Juno-106 has dedicated sliders per section
    
    // Chorus (can combine both)
    bool chorus1 = false;       // Chorus I
    bool chorus2 = false;       // Chorus II
    
    // VCA
    int vcaMode = 0;           // 0=ENV, 1=GATE
    
    // Chorus/Ensemble (Authentic Juno-106)
    int chorusMode = 0;         // 0=Off, 1=I, 2=II, 3=I+II
    
    // POLY Modes (Authentic Juno-106)
    int polyMode = 0;           // 0=POLY1, 1=POLY2, 2=UNISON
    
    // Master
    float vcaLevel = 0.8f;      // 0-1
    
    // Bender
    float benderValue = 0.0f;   // -1 to +1
    float benderToDCO = 1.0f;   // 0-1 (amount to pitch)
    float benderToVCF = 0.0f;   // 0-1 (amount to filter)
    float benderToLFO = 0.0f;   // 0-1 (amount to LFO rate)
    
    // Analog Character
    float drift = 0.0f;            // 0 to 1, analog drift amount
    float tune = 0.0f;             // ±50 cents (Master Tune)
    
    // VCF Modulation (Juno-106 authentic controls)
    // vcfEnvAmount removed (consolidated with envAmount)

    float vcfLFOAmount = 0.0f;     // LFO: 0 to 1 (LFO modulation depth)
    float lfoToVCF = 0.0f;         // LFO: 0 to 1 (authentic LFO to VCF slider)
    float kybdTracking = 0.0f;     // KYBD: 0 to 1 (keyboard tracking amount)
    int vcfPolarity = 0;           // 0=Normal, 1=Inverted
    
    // HPF (High-Pass Filter - Juno-106 authentic)
    int hpfFreq = 0;               // 0=Off, 1=80Hz, 2=180Hz, 3=330Hz
    
    // Portamento
    bool portamentoOn = false;  // ON/OFF
    float portamentoTime = 0.0f; // 0-1 (0-5 seconds glide time)
};
