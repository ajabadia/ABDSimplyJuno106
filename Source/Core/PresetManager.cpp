#include "PresetManager.h"
#include "FactoryPresets.h"
#include "JunoTapeDecoder.h"

PresetManager::PresetManager() {
    // Initialize with first bank
    addBank("Factory Bank");
    loadFactoryPresets();
    loadUserPresets();
}

void PresetManager::addBank(const juce::String& name) {
    Bank b;
    b.name = name;
    banks.push_back(b);
}

void PresetManager::selectBank(int index) {
    currentBankIndex = juce::jlimit(0, getNumBanks() - 1, index);
}

juce::Result PresetManager::loadTape(const juce::File& wavFile) {
    auto result = JunoTapeDecoder::decodeWavFile(wavFile);
    if (!result.success) return juce::Result::fail(result.errorMessage);
    
    // Create new bank for this tape
    addBank(wavFile.getFileNameWithoutExtension());
    int newBankIdx = getNumBanks() - 1;
    Bank& bank = banks[newBankIdx];
    
    // Decode patches
    for (int p = 0; p < 64; ++p) {
        if ((p + 1) * 18 > (int)result.data.size()) break;
        
        const unsigned char* patchBytes = &result.data[p * 18];
        juce::String patchName = juce::String(p + 1).paddedLeft('0', 2);
        bank.patches.push_back(createPresetFromJunoBytes(patchName, patchBytes));
    }
    
    selectBank(newBankIdx);
    
    // If we have warning message (like Partial Bank), we could append it, but Result usually means Success or Fail.
    // We can assume if Success, it is OK.
    return juce::Result::ok();
}

void PresetManager::createFactoryPreset(Bank& bank, const juce::String& name,
                                       float sawLevel, float pulseLevel, float pwm, float subOsc,
                                       float cutoff, float resonance, float envAmount,
                                       float attack, float decay, float sustain, float release,
                                       float lfoRate, float lfoDepth, int lfoDest,
                                       bool chorusI, bool chorusII, bool gateMode) {
    juce::ValueTree state("Parameters");
    state.setProperty("sawOn", sawLevel > 0.0f, nullptr);
    state.setProperty("pulseOn", pulseLevel > 0.0f, nullptr);
    state.setProperty("pwm", pwm, nullptr);
    state.setProperty("subOsc", subOsc, nullptr);
    state.setProperty("vcfFreq", cutoff, nullptr);
    state.setProperty("resonance", resonance, nullptr);
    state.setProperty("envAmount", envAmount, nullptr);
    state.setProperty("attack", attack, nullptr);
    state.setProperty("decay", decay, nullptr);
    state.setProperty("sustain", sustain, nullptr);
    state.setProperty("release", release, nullptr);
    state.setProperty("lfoRate", lfoRate, nullptr);
    state.setProperty("lfoToDCO", lfoDepth, nullptr);
    state.setProperty("chorus1", chorusI, nullptr);
    state.setProperty("chorus2", chorusII, nullptr);
    state.setProperty("vcaMode", gateMode ? 1 : 0, nullptr);
    state.setProperty("vcaLevel", 0.8f, nullptr);
    
    bank.patches.push_back(Preset(name, "Factory", state));
}

PresetManager::Preset PresetManager::createPresetFromJunoBytes(const juce::String& name, const unsigned char* bytes) {
    juce::ValueTree state("Parameters");
    auto toNorm = [](unsigned char b) { return static_cast<float>(b) / 127.0f; };
    
    state.setProperty("lfoRate", toNorm(bytes[0]), nullptr);
    state.setProperty("lfoDelay", toNorm(bytes[1]), nullptr);
    state.setProperty("lfoToDCO", toNorm(bytes[2]), nullptr);
    state.setProperty("pwm", toNorm(bytes[3]), nullptr);
    state.setProperty("noise", toNorm(bytes[4]), nullptr);
    state.setProperty("vcfFreq", toNorm(bytes[5]), nullptr);
    state.setProperty("resonance", toNorm(bytes[6]), nullptr);
    state.setProperty("envAmount", toNorm(bytes[7]), nullptr);
    state.setProperty("lfoToVCF", toNorm(bytes[8]), nullptr);
    state.setProperty("kybdTracking", toNorm(bytes[9]), nullptr);
    state.setProperty("vcaLevel", toNorm(bytes[10]), nullptr);
    state.setProperty("attack", toNorm(bytes[11]), nullptr);
    state.setProperty("decay", toNorm(bytes[12]), nullptr);
    state.setProperty("sustain", toNorm(bytes[13]), nullptr);
    state.setProperty("release", toNorm(bytes[14]), nullptr);
    state.setProperty("subOsc", toNorm(bytes[15]), nullptr);
    
    unsigned char sw1 = bytes[16];
    int range = 1; 
    if (sw1 & (1 << 0)) range = 0; // 16'
    else if (sw1 & (1 << 1)) range = 1; // 8'
    else if (sw1 & (1 << 2)) range = 2; // 4'
    state.setProperty("dcoRange", range, nullptr);
    state.setProperty("pulseOn", (sw1 & (1 << 3)) != 0, nullptr);
    state.setProperty("sawOn", (sw1 & (1 << 4)) != 0, nullptr);
    
    bool chorusOn = (sw1 & (1 << 5)) == 0;
    bool chorusModeII = (sw1 & (1 << 6)) != 0; // 0=I, 1=II
    state.setProperty("chorus1", chorusOn && !chorusModeII, nullptr);
    state.setProperty("chorus2", chorusOn && chorusModeII, nullptr);

    unsigned char sw2 = bytes[17];
    state.setProperty("pwmMode", (sw2 & (1 << 0)) != 0, nullptr);
    state.setProperty("vcfPolarity", (sw2 & (1 << 1)) != 0, nullptr);
    state.setProperty("vcaMode", (sw2 & (1 << 2)) != 0, nullptr);
    
    int hpfRaw = (sw2 >> 3) & 0x03;
    state.setProperty("hpfFreq", hpfRaw, nullptr); // Map 1:1
    
    return Preset(name, "Factory", state);
}

void PresetManager::loadFactoryPresets() {
    Bank& bank = banks[0];
    bank.patches.clear();
    
    // Add factory sounds from header
    for (const auto& data : junoFactoryPresets) {
        bank.patches.push_back(createPresetFromJunoBytes(data.name, data.bytes));
    }
}

void PresetManager::loadUserPresets() {
    // For now, user presets are just another bank
    addBank("User Bank");
    Bank& bank = banks.back();
    
    auto userDir = getUserPresetsDirectory();
    if (!userDir.exists()) return;
    auto files = userDir.findChildFiles(juce::File::findFiles, false, "*.json");
    for (const auto& file : files) {
        auto json = juce::JSON::parse(file);
        if (json.isObject()) {
            auto obj = json.getDynamicObject();
            if (obj != nullptr) {
                juce::String name = obj->getProperty("name").toString();
                auto stateVar = obj->getProperty("state");
                if (stateVar.isObject()) {
                    juce::ValueTree state = juce::ValueTree::fromXml(stateVar.toString());
                    if (state.isValid()) bank.patches.push_back(Preset(name, "User", state));
                }
            }
        }
    }
}

void PresetManager::saveUserPreset(const juce::String& name, const juce::ValueTree& state) {
    auto userDir = getUserPresetsDirectory();
    userDir.createDirectory();
    auto file = userDir.getChildFile(name + ".json");
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("state", state.toXmlString());
    juce::var json(obj.get());
    file.replaceWithText(juce::JSON::toString(json));
}

void PresetManager::deleteUserPreset(const juce::String& name) {
    auto userDir = getUserPresetsDirectory();
    auto file = userDir.getChildFile(name + ".json");
    file.deleteFile();
}

juce::StringArray PresetManager::getPresetNames() const {
    juce::StringArray names;
    if (currentBankIndex < getNumBanks()) {
        for (const auto& p : banks[currentBankIndex].patches) names.add(p.name);
    }
    return names;
}

const PresetManager::Preset* PresetManager::getPreset(int index) const {
    if (currentBankIndex < getNumBanks() && index >= 0 && index < (int)banks[currentBankIndex].patches.size()) {
        return &banks[currentBankIndex].patches[index];
    }
    return nullptr;
}

void PresetManager::setCurrentPreset(int index) {
    currentPresetIndex = index;
}

juce::File PresetManager::getUserPresetsDirectory() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("SimpleJuno106v2").getChildFile("UserPresets");
}
