#pragma once
#include <JuceHeader.h>
#include <vector>

/**
 * JunoTapeDecoder
 * Decodes Roland Juno-106 FSK tape audio (1300Hz/2100Hz) into binary patch data.
 */
class JunoTapeDecoder {
public:
    struct DecodeResult {
        bool success = false;
        std::vector<uint8_t> data;
        juce::String errorMessage;
    };

    /**
     * Decodes a WAV file containing Juno-106 tape data.
     * Expects a full bank (1152 bytes) or partial bank (multiple of 18 bytes).
     */
    static DecodeResult decodeWavFile(const juce::File& file);

private:
    // FSK Constants
    static constexpr float kFreq0 = 1300.0f;
    static constexpr float kFreq1 = 2100.0f;
    static constexpr float kThreshold = 0.5f; // Frequency discrimination threshold

    // Bit-stream Reconstruction
};
