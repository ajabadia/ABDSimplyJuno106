#pragma once

#include <JuceHeader.h>

#include "../Synth/Voice.h"
#include "JunoVoiceManager.h"
#include "JunoSysEx.h"
#include "MidiLearnHandler.h"

class PresetManager;

/**
 * SimpleJuno106AudioProcessor
 */
class SimpleJuno106AudioProcessor : public juce::AudioProcessor,
                                     public juce::MidiKeyboardState::Listener {
public:
    SimpleJuno106AudioProcessor();
    ~SimpleJuno106AudioProcessor() override;

    // AudioProcessor
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    // MIDI / SysEx Support
    bool midiOutEnabled = false;
    int midiChannel = 1; // 1-16
    juce::MidiBuffer midiOutBuffer;
    MidiLearnHandler midiLearnHandler;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Accessors
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    class PresetManager* getPresetManager();
    const JunoVoiceManager& getVoiceManager() const { return voiceManager; }
    
    juce::MidiKeyboardState keyboardState;

    // Preset Management
    void loadPreset(int index);
    
    // Internal Synth Logic
    void updateParamsFromAPVTS();
    
    // SysEx Handling
    void handleSysEx(const uint8_t* data, int size);
    void sendPatchDump();
    juce::MidiMessage generatePatchDumpMessage();

    // Test Mode (Authentic Hardware Behavior)
    bool isTestMode = false;
    // int lastTriggeredVoiceIdx = -1; // Removed: Handled internally or debug data from manager
    void triggerTestProgram(int bankIndex);
    void enterTestMode(bool enter);

    // Midi Listener
    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Core Engine
    JunoVoiceManager voiceManager;
    SynthParams currentParams;
    SynthParams lastParams; // Used to detect changes for SysEx send

    std::unique_ptr<class PresetManager> presetManager;
    
    // MIDI State
    bool sustainPedalActive = false;
    std::vector<int> pendingNoteOffs;

    juce::dsp::Chorus<float> chorus; 
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> dcBlocker;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleJuno106AudioProcessor)
};
