#pragma once

#include <JuceHeader.h>

/**
 * PresetManager - Manages factory and user presets
 * 
 * Features:
 * - Factory presets (hardcoded)
 * - User presets (saved to AppData as JSON)
 * - Load/Save functionality
 * - Preset list management
 */
class PresetManager {
public:
    struct Preset {
        juce::String name;
        juce::String category; // "Factory", "User", or Tape name
        juce::ValueTree state;
        
        Preset() = default;
        Preset(const juce::String& n, const juce::String& cat, const juce::ValueTree& s)
            : name(n), category(cat), state(s) {}
    };

    struct Bank {
        juce::String name;
        std::vector<Preset> patches; // Juno-106 banks are 64 patches (8x8)
        
        Bank() { patches.reserve(64); }
    };
    
    PresetManager();
    ~PresetManager() = default;
    
    // Bank management
    void addBank(const juce::String& name);
    void selectBank(int index);
    int getActiveBankIndex() const { return currentBankIndex; }
    int getNumBanks() const { return static_cast<int>(banks.size()); }
    const Bank& getBank(int index) const { return banks[juce::jlimit(0, getNumBanks() - 1, index)]; }
    
    // Tape Loading
    juce::Result loadTape(const juce::File& wavFile);

    // Preset management
    void loadFactoryPresets();
    void loadUserPresets();
    void saveUserPreset(const juce::String& name, const juce::ValueTree& state);
    void deleteUserPreset(const juce::String& name);
    
    // Preset access (for current bank)
    juce::StringArray getPresetNames() const;
    const Preset* getPreset(int index) const;
    
    // Current preset
    void setCurrentPreset(int index);
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    
    // File paths
    juce::File getUserPresetsDirectory() const;
    
private:
    std::vector<Bank> banks;
    int currentBankIndex = 0;
    int currentPresetIndex = 0;
    
    void createFactoryPreset(Bank& bank, const juce::String& name, 
                            float sawLevel, float pulseLevel, float pwm, float subOsc,
                            float cutoff, float resonance, float envAmount,
                            float attack, float decay, float sustain, float release,
                            float lfoRate, float lfoDepth, int lfoDest,
                            bool chorusI, bool chorusII, bool gateMode);

    Preset createPresetFromJunoBytes(const juce::String& name, const unsigned char* bytes);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
