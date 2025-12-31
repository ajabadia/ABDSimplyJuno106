#pragma once
#include <JuceHeader.h>
#include <vector>

/**
 * JunoSysEx - Helper for Roland Juno-106 SysEx protocol
 * Supports:
 * - 0x32 (Individual Parameter Change): F0 41 32 <channel> <paramId> <value> F7
 * - 0x30 (Patch Dump): F0 41 30 <channel> <patchNum> <16 params> <sw1> <sw2> F7
 * - 0x31 (Manual Mode): F0 41 31 <channel> <patchNum?> F7
 */
namespace JunoSysEx
{
    static constexpr uint8_t kRolandID = 0x41;
    static constexpr uint8_t kMsgPatchDump = 0x30;
    static constexpr uint8_t kMsgManualMode = 0x31;
    static constexpr uint8_t kMsgParamChange = 0x32;

    // Parameter Identifiers for 0x32
    enum ParamID {
        LFO_RATE = 0x00,
        LFO_DELAY = 0x01,
        DCO_LFO = 0x02,
        DCO_PWM = 0x03,
        DCO_NOISE = 0x04,
        VCF_FREQ = 0x05,
        VCF_RES = 0x06,
        VCF_ENV = 0x07,
        VCF_LFO = 0x08,
        VCF_KYBD = 0x09,
        VCA_LEVEL = 0x0A,
        ENV_A = 0x0B,
        ENV_D = 0x0C,
        ENV_S = 0x0D,
        ENV_R = 0x0E,
        DCO_SUB = 0x0F,
        SWITCHES_1 = 0x10,
        SWITCHES_2 = 0x11
    };

    /** Creates a 7-byte Roland SysEx message for individual parameter changes (0x32) */
    inline juce::MidiMessage createParamChange(int channel, int paramId, int value)
    {
        uint8_t data[7];
        data[0] = 0xF0;
        data[1] = kRolandID;
        data[2] = kMsgParamChange;
        data[3] = static_cast<uint8_t>(channel & 0x0F);
        data[4] = static_cast<uint8_t>(paramId & 0x7F);
        data[5] = static_cast<uint8_t>(value & 0x7F);
        data[6] = 0xF7;
        return juce::MidiMessage(data, 7);
    }

    /** Creates a Patch Dump message (0x30). Returns full MidiMessage with F0/F7. 
     *  Internal data payload: RolandID, MsgType, Ch, PatchNum, 16 Params, SW1, SW2.
     */
    inline juce::MidiMessage createPatchDump(int channel, int patchNum, const uint8_t* params16, uint8_t sw1, uint8_t sw2)
    {
        std::vector<uint8_t> data;
        data.push_back(kRolandID);
        data.push_back(kMsgPatchDump);
        data.push_back(static_cast<uint8_t>(channel & 0x0F));
        data.push_back(static_cast<uint8_t>(patchNum & 0x7F));
        
        for (int i = 0; i < 16; ++i) data.push_back(params16[i] & 0x7F);
        data.push_back(sw1 & 0x7F);
        data.push_back(sw2 & 0x7F);
        
        return juce::MidiMessage::createSysExMessage(data.data(), (int)data.size());
    }

    /** Parses a MidiMessage to see if it's a valid Juno-106 SysEx */
    inline bool parseMessage(const juce::MidiMessage& msg, int& type, int& channel, int& p1, int& p2, std::vector<uint8_t>& dumpData)
    {
        if (!msg.isSysEx()) return false;
        
        // getSysExData returns pointer to Byte AFTER F0.
        // getRawDataSize returns Total Bytes including F0 and F7.
        const uint8_t* data = msg.getSysExData();
        int size = msg.getRawDataSize() - 2; // Size of content between F0 and F7

        if (size < 3) return false;
        if (data[0] != kRolandID) return false;
        
        type = data[1];
        channel = data[2] & 0x0F;

        if (type == kMsgParamChange && size == 5)
        {
            p1 = data[3] & 0x7F; // Param ID
            p2 = data[4] & 0x7F; // Value
            return true;
        }
        // We allow >= 22 to support dumps that might include extra padding or unofficial checksums.
        // The standard dump is exactly 22 bytes (header+payload), but robust parsers often ignore trailing bytes.
        else if (type == kMsgPatchDump && size >= 22) // 41 30 CH Patch [16 params] SW1 SW2
        {
            p1 = data[3] & 0x7F; // Patch Num
            dumpData.clear();
            // We need 18 bytes (16 params + 2 switches)
            // Valid range indexes: 4 to 21
            // Validation: Ensure valid 7-bit data
            for (int i = 4; i < 22; ++i) {
                if (data[i] > 0x7F) return false; // Invalid SysEx data
                dumpData.push_back(data[i]);
            }
            return true;
        }
        else if (type == kMsgManualMode)
        {
            return true;
        }
        
        return false;
    }
}
