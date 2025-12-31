#pragma once

#include <JuceHeader.h>
#include "../Synth/Voice.h"
#include "SynthParams.h"
#include <array>

/**
 * JunoVoiceManager
 * 
 * Handles the allocation and lifecycle of 6 fixed voices, mimicking the
 * Roland Juno-106 architecture (DCOs 1-6).
 * 
 * - Fixed 6-voice polyphony
 * - Round-Robin allocation
 * - Voice stealing based on oldest timestamp
 * - Centralized parameter updates
 */
class JunoVoiceManager {
public:
    JunoVoiceManager();
    
    void prepare(double sampleRate, int maxBlockSize);
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    void noteOn(int midiChannel, int midiNote, float velocity);
    void noteOff(int midiChannel, int midiNote, float velocity);
    void outputActiveVoiceInfo(); // Debug helper
    
    // Updates all voices with the current parameter state
    // Should be called once per block, not per sample
    void updateParams(const SynthParams& params);
    
    void setPolyMode(int mode); // 1-Poly1, 2-Poly2, 3-Unison
    int getLastTriggeredVoiceIndex() const { return lastAllocatedVoiceIndex; }
    void setAllNotesOff();

private:
    static constexpr int MAX_VOICES = 6;
    std::array<Voice, MAX_VOICES> voices;
    
    // Timestamps for voice stealing (incremented on noteOn)
    std::array<uint64_t, MAX_VOICES> voiceTimestamps;
    uint64_t currentTimestamp = 0;
    
    int lastAllocatedVoiceIndex = -1; // For Round-Robin
    int polyMode = 1; // 1=Poly1, 2=Poly2, 3=Unison
    
    // Finds the best voice to play a new note
    int findFreeVoiceIndex();
    
    // Finds the oldest voice to steal
    int findVoiceToSteal();
};
