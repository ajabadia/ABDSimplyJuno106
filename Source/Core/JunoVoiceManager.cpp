#include "JunoVoiceManager.h"

JunoVoiceManager::JunoVoiceManager() {
    voiceTimestamps.fill(0);
    initVariances();
}

void JunoVoiceManager::initVariances() {
    juce::Random rng;
    rng.setSeed(1984); // 1984: Release year of Juno-106 (Deterministic "Mojo")

    for (int i = 0; i < MAX_VOICES; ++i) {
        Voice::Variance v;
        // Subtle analog deviations (Component tolerances)
        v.filterCutoffScale = 1.0f + (rng.nextFloat() * 0.03f - 0.015f); // +/- 1.5%
        v.filterResScale = 1.0f + (rng.nextFloat() * 0.10f - 0.05f);    // +/- 5% (Resonance varies more)
        v.envTimeScale = 1.0f + (rng.nextFloat() * 0.04f - 0.02f);      // +/- 2%
        v.pwOffset = rng.nextFloat() * 0.04f - 0.02f;                   // +/- 2% 
        
        voices[i].setVariance(v);
    }
}

void JunoVoiceManager::prepare(double sampleRate, int maxBlockSize) {
    for (auto& voice : voices) {
        voice.prepare(sampleRate, maxBlockSize);
    }
}

void JunoVoiceManager::updateParams(const SynthParams& params) {
    for (auto& voice : voices) {
        voice.updateParams(params);
    }
}

void JunoVoiceManager::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    // Clear buffer is handled by the processor usually, but we accumulate
    // voices. The processor passes a buffer that might contain other things, 
    // but typically it's cleared before audio generation.
    // Assuming buffer is ready for accumulation or cleared.
    // To be safe and typical for a manager, we usually assume addition.
    // But since this is the primary source, PluginProcessor should clear it.
    
    // We render each voice into a temp buffer and add, or add directly if Voice supports it.
    // Voice::renderNextBlock typically adds to the buffer.
    
    for (auto& voice : voices) {
        if (voice.isActive()) {
            voice.renderNextBlock(buffer, startSample, numSamples);
        }
    }
}

void JunoVoiceManager::setPolyMode(int mode) {
    if (polyMode != mode) {
        polyMode = mode;
        setAllNotesOff(); // Prevent stuck notes when switching modes
    }
}

void JunoVoiceManager::noteOn(int midiChannel, int midiNote, float velocity) {
    currentTimestamp++;
    
    // === UNISON MODE (3) ===
    // Triggers ALL voices for the same note.
    if (polyMode == 3) {
        for (int i = 0; i < MAX_VOICES; ++i) {
            voices[i].noteOn(midiNote, velocity);
            voiceTimestamps[i] = currentTimestamp;
        }
        lastAllocatedVoiceIndex = 0; // Reset
        return;
    }
    
    // === POLY MODES (1 & 2) ===
    
    // 1. Check for retrigger (Same note already playing)
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices[i].isActive() && voices[i].getCurrentNote() == midiNote) {
            voices[i].noteOn(midiNote, velocity);
            voiceTimestamps[i] = currentTimestamp;
            lastAllocatedVoiceIndex = i; // Update last allocated even on retrigger
            return;
        }
    }
    
    // 2. Find free voice strategy
    int voiceIndex = findFreeVoiceIndex();
    
    // 3. Stealing (if no free voice)
    if (voiceIndex == -1) {
        voiceIndex = findVoiceToSteal();
    }
    
    // 4. Allocate
    if (voiceIndex != -1) {
        voices[voiceIndex].noteOn(midiNote, velocity);
        voiceTimestamps[voiceIndex] = currentTimestamp;
        lastAllocatedVoiceIndex = voiceIndex;
    }
}

void JunoVoiceManager::noteOff(int midiChannel, int midiNote, float velocity) {
    // If Unison, kill all matching notes
    if (polyMode == 3) {
        for (int i = 0; i < MAX_VOICES; ++i) {
             if (voices[i].getCurrentNote() == midiNote) {
                 voices[i].noteOff();
             }
        }
        return;
    }

    // Poly Mode: Kill specific voice
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices[i].isActive() && voices[i].getCurrentNote() == midiNote) {
            voices[i].noteOff();
            return;
        }
    }
}

int JunoVoiceManager::findFreeVoiceIndex() {
    // Poly 1: Cyclic (Round Robin) - Authentic natural release
    if (polyMode == 1) {
        for (int i = 0; i < MAX_VOICES; ++i) {
            int idx = (lastAllocatedVoiceIndex + 1 + i) % MAX_VOICES;
            if (!voices[idx].isActive()) return idx;
        }
    }
    // Poly 2: Lowest Free (Linear) - Tends to cut tails, cleaner
    else if (polyMode == 2) {
         for (int i = 0; i < MAX_VOICES; ++i) {
            if (!voices[i].isActive()) return i;
        }
    }
    
    return -1;
}

int JunoVoiceManager::findVoiceToSteal() {
    int oldestIndex = -1;
    uint64_t minTimestamp = UINT64_MAX;
    
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voiceTimestamps[i] < minTimestamp) {
            minTimestamp = voiceTimestamps[i];
            oldestIndex = i;
        }
    }
    
    return oldestIndex;
}

void JunoVoiceManager::outputActiveVoiceInfo() {
    juce::String state;
    for (int i = 0; i < MAX_VOICES; ++i) {
        state += "[" + juce::String(i) + ":" + (voices[i].isActive() ? juce::String(voices[i].getCurrentNote()) : ".") + "] ";
    }
    DBG("Voices: " << state);
}

void JunoVoiceManager::setAllNotesOff() {
    for (auto& voice : voices) {
        voice.noteOff();
    }
}
