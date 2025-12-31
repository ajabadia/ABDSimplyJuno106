#include "Voice.h"
#include <cmath>

Voice::Voice() {
    // Initialize filter with Juno-106 characteristics
    // IR3109 is a 4-pole (24dB/oct) lowpass
    filter.setMode(juce::dsp::LadderFilterMode::LPF24);
    filter.setResonance(0.0f);
    filter.setCutoffFrequencyHz(5500.0f);  // Authentic Juno default
    filter.setDrive(1.2f);  // Slight drive for analog warmth
}

void Voice::prepare(double sr, int maxBlockSize) {
    sampleRate = sr;
    
    // Setup DCO (Juno-106)
    dco.prepare(sr, maxBlockSize);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;
    
    // Setup ADSR (Juno-106)
    adsr.setSampleRate(sr);
    
    filter.prepare(spec);
    filter.reset();
    
    // Setup HPF (Juno-106)
    hpFilter.prepare(spec);
    hpFilter.reset();
    updateHPF();
    
    // Setup LFO (Juno-106)
    lfo.prepare(sr, maxBlockSize);
}

void Voice::noteOn(int midiNote, float vel) {
    currentNote = midiNote;
    velocity = vel;
    
    // Calculate target frequency
    targetFrequency = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
    // Portamento: if enabled, glide from current to target
    if (params.portamentoOn && adsr.isActive()) {
        // Keep current frequency, will glide to target
        // currentFrequency is already set from previous note, will glide to targetFrequency
    } else {
        // Jump immediately to target
        currentFrequency = targetFrequency;
    }
    
    // Set DCO frequency
    dco.setFrequency(currentFrequency);
    
    // Trigger ADSR
    adsr.noteOn();
    
    // Trigger LFO (with delay)
    lfo.noteOn();
}

void Voice::noteOff() {
    adsr.noteOff();
}

void Voice::updateParams(const SynthParams& p) {
    params = p;
    
    // Update DCO (Complete Juno-106)
    dco.setRange(static_cast<JunoDCO::Range>(p.dcoRange));
    dco.setSawLevel(p.sawOn ? 1.0f : 0.0f); 
    dco.setPulseLevel(p.pulseOn ? 1.0f : 0.0f); 
    dco.setSubLevel(p.subOscLevel);
    dco.setNoiseLevel(p.noiseLevel);
    dco.setPWM(juce::jlimit(0.0f, 0.95f, p.pwmAmount + variance.pwOffset));
    dco.setPWMMode(static_cast<JunoDCO::PWMMode>(p.pwmMode));
    dco.setLFODepth(p.lfoToDCO);
    dco.setDrift(p.drift);
    
    // Update ADSR (Spec: 1.5ms to 3s/12s) - Normalized input to Log scaling
    float attackTime = 0.0015f * std::pow(3.0f / 0.0015f, p.attack);
    float decayTime = 0.0015f * std::pow(12.0f / 0.0015f, p.decay);
    float releaseTime = 0.0015f * std::pow(12.0f / 0.0015f, p.release);

    adsr.setAttack(attackTime * variance.envTimeScale);
    adsr.setDecay(decayTime * variance.envTimeScale);
    adsr.setSustain(p.sustain);
    adsr.setRelease(releaseTime * variance.envTimeScale);
    adsr.setGateMode(p.vcaMode == 1);
    
    // IR3109 resonance: self oscillation push (Normalize 0-1 to internal range)
    filter.setResonance(juce::jlimit(0.0f, 10.0f, p.resonance * 1.05f * variance.filterResScale)); 
    
    // Update HPF
    updateHPF();
    
    // Update LFO (Spec: 0.1Hz to 30Hz, 3s Delay)
    float lfoRateHz = 0.1f * std::pow(30.0f / 0.1f, p.lfoRate);
    lfo.setRate(lfoRateHz);
    lfo.setDepth(1.0f); 
    lfo.setDelay(p.lfoDelay * 3.0f);
}

void Voice::updateHPF() {
    float cutoffFreq = 10.0f;
    
    // Authentic Juno-106 HPF Cutoff Frequencies
    switch (params.hpfFreq) {
        case 0: cutoffFreq = 10.0f; break;    // Position 0: Bypass/Boost (DC Block level)
        case 1: cutoffFreq = 225.0f; break;   // Position 1
        case 2: cutoffFreq = 360.0f; break;   // Position 2
        case 3: cutoffFreq = 720.0f; break;   // Position 3
    }
    
    hpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffFreq);
}

void Voice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
    if (!adsr.isActive()) return;
    
    // Portamento: smooth glide to target frequency
    if (params.portamentoOn && std::abs(currentFrequency - targetFrequency) > 0.1f) {
        float glideTime = params.portamentoTime * 5.0f;
        float glideRate = 1.0f / (glideTime * static_cast<float>(sampleRate));
        
        if (currentFrequency < targetFrequency) {
            currentFrequency += (targetFrequency - currentFrequency) * glideRate;
        } else {
            currentFrequency -= (currentFrequency - targetFrequency) * glideRate;
        }
    } else {
        currentFrequency = targetFrequency;
    }
    
    // Apply bender and master tune to DCO
    float bendedFrequency = currentFrequency;
    bendedFrequency *= std::pow(2.0f, params.tune / 1200.0f);

    if (params.benderValue != 0.0f && params.benderToDCO > 0.0f) {
        float bendSemitones = params.benderValue * params.benderToDCO * 12.0f;
        bendedFrequency *= std::pow(2.0f, bendSemitones / 12.0f);
    }
    
    dco.setFrequency(bendedFrequency);
    
    // Base VCF cutoff (10Hz to 24kHz authentic range)
    float baseCutoff = 10.0f * std::pow(24000.0f / 10.0f, params.vcfFreq);
    if (params.kybdTracking > 0.0f) {
        float semitones = static_cast<float>(currentNote) - 60.0f;
        baseCutoff *= std::pow(2.0f, (semitones * params.kybdTracking) / 12.0f);
    }

    // Temporary buffers
    juce::AudioBuffer<float> voiceBuffer(1, numSamples);
    voiceBuffer.clear();
    
    // Generate oscillator and modulation
    for (int i = 0; i < numSamples; ++i) {
        // 1. Advance LFO (Per-sample is mandatory for timing)
        float lfoValue = lfo.getNextSample();
        
        // 2. Update VCF Cutoff (Frequent update for snap)
        if ((i % 8) == 0) {
            float envVal = adsr.getCurrentValue();
            
            float envModOctaves = envVal * params.envAmount * 14.0f;
            if (params.vcfPolarity == 1) envModOctaves = -envModOctaves;
            
            float lfoModOctaves = lfoValue * params.vcfLFOAmount * 3.5f;
            float benderModOctaves = params.benderValue * params.benderToVCF * 3.5f;
            
            float modulatedCutoff = baseCutoff * std::pow(2.0f, envModOctaves + lfoModOctaves + benderModOctaves);
            modulatedCutoff = juce::jlimit(5.0f, static_cast<float>(sampleRate * 0.45), modulatedCutoff * variance.filterCutoffScale);
            filter.setCutoffFrequencyHz(modulatedCutoff);
        }
        
        // 3. Generate sample from JunoDCO
        float sample = dco.getNextSample(lfoValue);
        
        // 4. Apply HPF
        sample = hpFilter.processSample(sample);
        
        voiceBuffer.setSample(0, i, sample);
    }
    
    // Process filter on entire block
    juce::dsp::AudioBlock<float> block(voiceBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filter.process(context);
    
    // Apply VCA envelope and mix to output
    for (int i = 0; i < numSamples; ++i) {
        float sample = voiceBuffer.getSample(0, i);
        float envelope = adsr.getNextSample();
        
        if (params.vcaMode == 1) { // GATE (handled by adsr internal mode)
            sample *= envelope * velocity; 
        } else { // ENV
            sample *= envelope * velocity;
        }
        
        sample *= params.vcaLevel;
        
        buffer.addSample(0, startSample + i, sample);
        if (buffer.getNumChannels() > 1) {
            buffer.addSample(1, startSample + i, sample);
        }
    }
}
