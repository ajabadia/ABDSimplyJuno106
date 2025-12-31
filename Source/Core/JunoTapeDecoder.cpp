#include "JunoTapeDecoder.h"
#include <cmath>

JunoTapeDecoder::DecodeResult JunoTapeDecoder::decodeWavFile(const juce::File& file) {
    DecodeResult result;
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr) {
        result.errorMessage = "Could not read WAV file: " + file.getFileName();
        return result;
    }
    
    juce::AudioBuffer<float> buffer(1, (int)reader->lengthInSamples);
    reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, false);
    
    const float* samples = buffer.getReadPointer(0);
    double sr = reader->sampleRate;
    const int numSamples = buffer.getNumSamples();
    
    // 1. Detection with adaptive hysteresis
    // 1. Detection with adaptive threshold based on Peak
    // This handles quiet or normalized recordings better than fixed 0.01
    float maxPeak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float absS = std::abs(samples[i]);
        if (absS > maxPeak) maxPeak = absS;
    }
    
    // Fallback if silent
    if (maxPeak < 0.001f) {
        result.errorMessage = "Signal is silence.";
        return result;
    }

    std::vector<int> crossings;
    bool isPositive = samples[0] > 0.0f;
    // Hysteresis threshold: 10% of peak (robust against noise floor)
    float threshold = maxPeak * 0.1f; 
    
    for (int i = 1; i < numSamples; ++i) {
        if (isPositive && samples[i] < -threshold) {
            crossings.push_back(i);
            isPositive = false;
        }
        else if (!isPositive && samples[i] > threshold) {
            crossings.push_back(i);
            isPositive = true;
        }
    }
    
    if (crossings.size() < 50) {
        result.errorMessage = "Signal too weak. Transitions: " + juce::String((int)crossings.size());
        return result;
    }
    
    // 2. Sample-accurate frequency state (FSK 1.3kHz vs 2.1kHz)
    double midHalfPeriod = sr / 3400.0;
    std::vector<bool> state(numSamples, true);
    for (size_t i = 1; i < crossings.size(); ++i) {
        int halfPeriod = crossings[i] - crossings[i-1];
        bool mark = (halfPeriod < midHalfPeriod); 
        for (int s = crossings[i-1]; s < crossings[i]; ++s) state[s] = mark;
    }
    
    // 3. Byte Scanning
    std::vector<uint8_t> decodedBytes;
    double samplesPerBit = sr / 1200.0;
    
    int s = 0;
    while (s < numSamples - (int)(samplesPerBit * 11)) {
        if (state[s] == true && state[s+1] == false) {
            double bitPos = (double)s + 1.0 + (samplesPerBit * 0.5);
            uint8_t byte = 0;
            bool framingSuccess = true;
            
            if (state[(int)bitPos] == false) {
                for (int b = 0; b < 8; ++b) {
                    bitPos += samplesPerBit;
                    if (state[(int)bitPos]) byte |= (1 << b);
                }
                
                bitPos += samplesPerBit;
                if (state[(int)bitPos] == false) framingSuccess = false;
                
                if (framingSuccess) {
                    decodedBytes.push_back(byte);
                    s = (int)(bitPos + (samplesPerBit * 0.2)); 
                    continue;
                }
            }
        }
        s++;
    }
    
    result.data = decodedBytes;
    
    // Validate Full Bank or Partial
    int numPatches = (int)(decodedBytes.size() / 18);
    int remainder = (int)(decodedBytes.size() % 18);
    
    if (numPatches > 0) {
        result.success = true;
        result.data.resize(numPatches * 18); // Truncate to valid boundary
        
        if (numPatches == 64 && remainder == 0) {
             // Perfect bank
             // errorMessage empty means "Perfect"
        }
        else {
             result.errorMessage = "Loaded " + juce::String(numPatches) + " patches (Partial Bank).";
             if (remainder > 0) {
                 result.errorMessage += " Discarded " + juce::String(remainder) + " trailing bytes.";
             }
        }
    }
    else {
        result.success = false;
        result.errorMessage = "No valid patches found. Extracted " + juce::String((int)decodedBytes.size()) + " bytes (Need 18 per patch).";
    }
    
    return result;
}

// reconstructBytes removed (logic integrated in decodeWavFile)
