#include "PluginProcessor.h"
#include <JuceHeader.h>
#include "PluginEditor.h"
#include "PresetManager.h"

//==============================================================================
SimpleJuno106AudioProcessor::SimpleJuno106AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#else
    : AudioProcessor(JucePlugin_PreferredChannelConfigurations),
#endif
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    presetManager = std::make_unique<PresetManager>();
    // voiceTimestamp init removed

    // Default MIDI CC Mapping
    midiLearnHandler.bind(16, "lfoRate");
    midiLearnHandler.bind(17, "lfoDelay");
    midiLearnHandler.bind(18, "lfoToDCO");
    midiLearnHandler.bind(19, "pwm");
    midiLearnHandler.bind(20, "subOsc");
    midiLearnHandler.bind(21, "noise");
    midiLearnHandler.bind(22, "hpfFreq");
    midiLearnHandler.bind(23, "vcfFreq");
    midiLearnHandler.bind(24, "resonance");
    midiLearnHandler.bind(25, "envAmount");
    midiLearnHandler.bind(26, "lfoToVCF");
    midiLearnHandler.bind(27, "kybdTracking");
    midiLearnHandler.bind(28, "attack");
    midiLearnHandler.bind(29, "decay");
    midiLearnHandler.bind(30, "sustain");
    midiLearnHandler.bind(31, "release");
    midiLearnHandler.bind(32, "vcaLevel");
    
    // Connect on-screen keyboard to the engine
    keyboardState.addListener(this);
}

SimpleJuno106AudioProcessor::~SimpleJuno106AudioProcessor()
{
}

//==============================================================================
const juce::String SimpleJuno106AudioProcessor::getName() const { return JucePlugin_Name; }

bool SimpleJuno106AudioProcessor::acceptsMidi() const { return true; }
bool SimpleJuno106AudioProcessor::producesMidi() const { return true; }
bool SimpleJuno106AudioProcessor::isMidiEffect() const { return false; }
double SimpleJuno106AudioProcessor::getTailLengthSeconds() const { return 0.0; }
int SimpleJuno106AudioProcessor::getNumPrograms() { return 1; }
int SimpleJuno106AudioProcessor::getCurrentProgram() { return 0; }
void SimpleJuno106AudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused(index); }
const juce::String SimpleJuno106AudioProcessor::getProgramName (int index) { juce::ignoreUnused(index); return {}; }
void SimpleJuno106AudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void SimpleJuno106AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;
    
    chorus.prepare(spec);
    chorus.reset();
    
    dcBlocker.prepare(spec);
    *dcBlocker.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
}

void SimpleJuno106AudioProcessor::releaseResources() {}

bool SimpleJuno106AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void SimpleJuno106AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 1. Pre-process keyboard state (updates listeners, i.e., us)
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    // 2. Handle SysEx and CCs (Directly from midiBuffer to avoid overhead in callbacks)
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isSysEx())
        {
            int type, ch, p1, p2;
            std::vector<uint8_t> dumpData;
            if (JunoSysEx::parseMessage(message, type, ch, p1, p2, dumpData))
            {
                if (type == JunoSysEx::kMsgManualMode) { /* Handle manual */ }
                else handleSysEx(message.getSysExData(), (int)message.getRawDataSize() - 2);
            }
        }
        else if (message.isController())
        {
            auto cn = message.getControllerNumber();
            auto cv = message.getControllerValue();
            
            if (cn == 1) // Modulation
            {
                if (auto* p = apvts.getParameter("benderToLFO")) p->setValueNotifyingHost(cv / 127.0f);
            }
            else if (cn == 64) // Sustain
            {
                sustainPedalActive = (cv >= 64);
                if (!sustainPedalActive)
                {
                    for (int note : pendingNoteOffs) handleNoteOff(nullptr, 0, note, 0.0f);
                    pendingNoteOffs.clear();
                }
            }
            else
            {
                midiLearnHandler.handleIncomingCC(cn, cv, apvts);
            }
        }
        else if (message.isPitchWheel())
        {
            auto val = (float)message.getPitchWheelValue();
            // Map 0-16383 to -1 to 1
            float norm = (val / 8192.0f) - 1.0f;
            if (auto* p = apvts.getParameter("bender")) p->setValueNotifyingHost(p->convertTo0to1(norm));
        }
    }

    // 3. Update internal parameters
    updateParamsFromAPVTS();
    voiceManager.updateParams(currentParams);

    // 4. Render voices
    buffer.clear();
    voiceManager.renderNextBlock(buffer, 0, buffer.getNumSamples());

    // 5. Apply Effects
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    bool c1 = apvts.getRawParameterValue("chorus1")->load() > 0.5f;
    bool c2 = apvts.getRawParameterValue("chorus2")->load() > 0.5f;
    
    if (c1 || c2) {
         if (c1 && !c2) {
            chorus.setRate(0.5f); chorus.setDepth(0.12f); chorus.setMix(0.5f); chorus.setCentreDelay(6.0f);
         } else if (!c1 && c2) {
            chorus.setRate(0.8f); chorus.setDepth(0.25f); chorus.setMix(0.5f); chorus.setCentreDelay(8.0f);
         } else {
            chorus.setRate(1.0f); chorus.setDepth(0.15f); chorus.setMix(0.5f); chorus.setCentreDelay(7.0f);
         }
         chorus.process(context);
    }
    dcBlocker.process(context);

    // 6. MIDI Out
    if (midiOutEnabled)
    {
        midiMessages.addEvents(midiOutBuffer, 0, buffer.getNumSamples(), 0);
        midiOutBuffer.clear();
    }
}

//==============================================================================
void SimpleJuno106AudioProcessor::enterTestMode(bool enter)
{
    isTestMode = enter;
    if (isTestMode)
    {
        // Authentic Test Mode display pattern (approximation)
        // Usually it shows segments. We'll set lastTriggeredVoiceIdx to a special value
        // lastTriggeredVoiceIdx = 0; // Show CH 1 on entry (Variable removed in refactor)
    }
}

void SimpleJuno106AudioProcessor::triggerTestProgram(int bankIndex)
{
    if (!isTestMode) return;

    auto setP = [&](juce::String id, float val) {
        if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(val / 10.0f);
    };
    auto setBool = [&](juce::String id, bool val) {
        if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(val ? 1.0f : 0.0f);
    };
    auto setInt = [&](juce::String id, int val) {
        if (auto* p = apvts.getParameter(id)) {
             float norm = p->convertTo0to1(static_cast<float>(val));
             p->setValueNotifyingHost(norm);
        }
    };
    auto setParam = [&](juce::String id, float normVal) {
        if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(normVal);
    };

    // Table settings from service manual
    auto applyCommon = [&]() {
        setP("lfoRate", 5); setP("lfoDelay", 0); setP("lfoToDCO", 8); 
        setInt("dcoRange", 1); // 8'
        setBool("sawOn", false); setBool("pulseOn", false);
        setP("subOsc", 0); setP("noise", 0); setP("lfoToVCF", 0); setP("pwm", 0);
        setParam("pwmMode", 1.0f); // MAN
        setParam("hpfFreq", 1.0f); // HPF 1
        setP("vcfFreq", 10); setP("resonance", 0); setP("envAmount", 0);
        setParam("vcfPolarity", 0.0f); 
        setP("kybdTracking", 10);
        setParam("vcaMode", 0.0f); // ENV
        setP("vcaLevel", 5);
        setP("attack", 0); setP("decay", 0); setP("sustain", 10); setP("release", 0);
        setBool("chorus1", false); setBool("chorus2", false);
    };

    applyCommon();

    switch (bankIndex)
    {
        case 0: break; // 1: VCA OFFSET
        case 1: setP("subOsc", 10); break; // 2: SUB OSC
        case 2: setP("vcfFreq", 6.3f); setP("resonance", 10); break; // 3: VCA/VCF GAIN
        case 3: setBool("sawOn", true); break; // 4: RAMP WAVE
        case 4: setBool("pulseOn", true); setP("pwm", 5); break; // 5: PWM 50%
        case 5: setP("noise", 10); break; // 6: NOISE LEVEL
        case 6: break; // 7: VCF HI/LO
        case 7: // 8: RE-TRIGGER
            setBool("pulseOn", true);
            setP("decay", 1.3f); setP("sustain", 0); setP("release", 1.3f);
            break;
    }
}

void SimpleJuno106AudioProcessor::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    voiceManager.noteOn(midiChannel, midiNoteNumber, velocity);
}

void SimpleJuno106AudioProcessor::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    if (sustainPedalActive)
    {
        if (std::find(pendingNoteOffs.begin(), pendingNoteOffs.end(), midiNoteNumber) == pendingNoteOffs.end())
            pendingNoteOffs.push_back(midiNoteNumber);
        return;
    }

    voiceManager.noteOff(midiChannel, midiNoteNumber, velocity);
}

void SimpleJuno106AudioProcessor::updateParamsFromAPVTS() {
    auto getVal = [this](juce::String id) { return apvts.getRawParameterValue(id)->load(); };
    auto getBool = [this](juce::String id) { return apvts.getRawParameterValue(id)->load() > 0.5f; };
    auto getInt = [this](juce::String id) { return static_cast<int>(apvts.getRawParameterValue(id)->load()); };

    currentParams.dcoRange = getInt("dcoRange");
    currentParams.sawOn = getBool("sawOn");
    currentParams.pulseOn = getBool("pulseOn");
    currentParams.pwmAmount = getVal("pwm");
    currentParams.pwmMode = getInt("pwmMode");
    currentParams.subOscLevel = getVal("subOsc");
    currentParams.noiseLevel = getVal("noise");
    currentParams.lfoToDCO = getVal("lfoToDCO");
    currentParams.hpfFreq = getInt("hpfFreq");
    currentParams.vcfFreq = getVal("vcfFreq");
    currentParams.resonance = getVal("resonance");
    currentParams.envAmount = getVal("envAmount");
    currentParams.lfoToVCF = getVal("lfoToVCF");
    currentParams.kybdTracking = getVal("kybdTracking");
    currentParams.vcfPolarity = getInt("vcfPolarity");
    currentParams.vcaMode = getInt("vcaMode");
    currentParams.vcaLevel = getVal("vcaLevel");
    currentParams.attack = getVal("attack");
    currentParams.decay = getVal("decay");
    currentParams.sustain = getVal("sustain");
    currentParams.release = getVal("release");
    currentParams.lfoRate = getVal("lfoRate");
    currentParams.lfoDelay = getVal("lfoDelay");
    currentParams.chorus1 = getBool("chorus1");
    currentParams.chorus2 = getBool("chorus2");
    currentParams.polyMode = getInt("polyMode");
    voiceManager.setPolyMode(currentParams.polyMode);

    currentParams.portamentoTime = getVal("portamentoTime");
    currentParams.portamentoOn = getBool("portamentoOn");
    currentParams.benderValue = getVal("bender");
    currentParams.benderToDCO = getVal("benderToDCO");
    currentParams.benderToVCF = getVal("benderToVCF");
    
    // Add Mod Wheel (benderToLFO) to LFO depths (Authentic feel: Modulation lever adds LFO)
    float modWheel = getVal("benderToLFO");
    currentParams.lfoToDCO = juce::jlimit(0.0f, 1.0f, getVal("lfoToDCO") + modWheel);
    currentParams.vcfLFOAmount = juce::jlimit(0.0f, 1.0f, getVal("lfoToVCF") + modWheel);

    currentParams.tune = getVal("tune");

    midiOutEnabled = getBool("midiOut");

    // Detect changes and send SysEx (individual params 0x32)
    if (midiOutEnabled)
    {
        auto sendIfChanged = [&](float oldVal, float newVal, JunoSysEx::ParamID pId) {
            if (std::abs(oldVal - newVal) > 0.001f)
                midiOutBuffer.addEvent(JunoSysEx::createParamChange(midiChannel - 1, pId, static_cast<int>(newVal * 127.0f)), 0);
        };

        sendIfChanged(lastParams.lfoRate, currentParams.lfoRate, JunoSysEx::LFO_RATE);
        sendIfChanged(lastParams.lfoDelay, currentParams.lfoDelay, JunoSysEx::LFO_DELAY);
        sendIfChanged(lastParams.lfoToDCO, currentParams.lfoToDCO, JunoSysEx::DCO_LFO);
        sendIfChanged(lastParams.pwmAmount, currentParams.pwmAmount, JunoSysEx::DCO_PWM);
        sendIfChanged(lastParams.noiseLevel, currentParams.noiseLevel, JunoSysEx::DCO_NOISE);
        sendIfChanged(lastParams.vcfFreq, currentParams.vcfFreq, JunoSysEx::VCF_FREQ);
        sendIfChanged(lastParams.resonance, currentParams.resonance, JunoSysEx::VCF_RES);
        sendIfChanged(lastParams.envAmount, currentParams.envAmount, JunoSysEx::VCF_ENV);
        sendIfChanged(lastParams.lfoToVCF, currentParams.lfoToVCF, JunoSysEx::VCF_LFO);
        sendIfChanged(lastParams.kybdTracking, currentParams.kybdTracking, JunoSysEx::VCF_KYBD);
        sendIfChanged(lastParams.vcaLevel, currentParams.vcaLevel, JunoSysEx::VCA_LEVEL);
        sendIfChanged(lastParams.attack, currentParams.attack, JunoSysEx::ENV_A);
        sendIfChanged(lastParams.decay, currentParams.decay, JunoSysEx::ENV_D);
        sendIfChanged(lastParams.sustain, currentParams.sustain, JunoSysEx::ENV_S);
        sendIfChanged(lastParams.release, currentParams.release, JunoSysEx::ENV_R);
        sendIfChanged(lastParams.subOscLevel, currentParams.subOscLevel, JunoSysEx::DCO_SUB);

        // SW1 (Byte 17)
        uint8_t sw1 = 0;
        if (currentParams.dcoRange == 0) sw1 |= (1 << 0);
        if (currentParams.dcoRange == 1) sw1 |= (1 << 1);
        if (currentParams.dcoRange == 2) sw1 |= (1 << 2);
        if (currentParams.pulseOn) sw1 |= (1 << 3);
        if (currentParams.sawOn) sw1 |= (1 << 4);
        if (!(currentParams.chorus1 || currentParams.chorus2)) sw1 |= (1 << 5); 
        if (currentParams.chorus1) sw1 |= (1 << 6); 

        midiOutBuffer.addEvent(JunoSysEx::createParamChange(midiChannel - 1, JunoSysEx::SWITCHES_1, sw1), 0);

        // SW2 (Byte 18)
        uint8_t sw2 = 0;
        if (currentParams.pwmMode == 1) sw2 |= (1 << 0);
        if (currentParams.vcfPolarity == 1) sw2 |= (1 << 1);
        if (currentParams.vcaMode == 1) sw2 |= (1 << 2);
        
        int hpfVal = currentParams.hpfFreq;
        sw2 |= (static_cast<uint8_t>(hpfVal & 0x03) << 3);

        midiOutBuffer.addEvent(JunoSysEx::createParamChange(midiChannel - 1, JunoSysEx::SWITCHES_2, sw2), 0);
    }
    lastParams = currentParams;
}

void SimpleJuno106AudioProcessor::loadPreset(int index) {
    if (!presetManager) return;
    const auto* preset = presetManager->getPreset(index);
    if (preset != nullptr) {
        for (int i = 0; i < (int)preset->state.getNumProperties(); ++i) {
            auto propName = preset->state.getPropertyName(i);
            auto value = preset->state.getProperty(propName);
            if (auto* param = apvts.getParameter(propName.toString())) {
                if (value.isBool() || (value.isDouble() && ((double)value == 0.0 || (double)value == 1.0))) {
                    param->setValueNotifyingHost(static_cast<bool>(value) ? 1.0f : 0.0f);
                }
                else {
                    float normValue = param->convertTo0to1(static_cast<float>(value));
                    param->setValueNotifyingHost(normValue);
                }
            }
        }
        presetManager->setCurrentPreset(index);
    }
}

void SimpleJuno106AudioProcessor::handleSysEx(const uint8_t* data, int size) 
{ 
    juce::MidiMessage msg = juce::MidiMessage::createSysExMessage(data, size);
    int type, ch, p1, p2;
    std::vector<uint8_t> dumpData;
    
    if (JunoSysEx::parseMessage(msg, type, ch, p1, p2, dumpData))
    {
        auto setParam = [&](juce::String id, float normVal) {
            if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(normVal);
        };

        if (type == JunoSysEx::kMsgParamChange)
        {
            float fVal = p2 / 127.0f;
            switch (p1)
            {
                case JunoSysEx::LFO_RATE: setParam("lfoRate", fVal); break;
                case JunoSysEx::LFO_DELAY: setParam("lfoDelay", fVal); break;
                case JunoSysEx::DCO_LFO: setParam("lfoToDCO", fVal); break;
                case JunoSysEx::DCO_PWM: setParam("pwm", fVal); break;
                case JunoSysEx::DCO_NOISE: setParam("noise", fVal); break;
                case JunoSysEx::VCF_FREQ: setParam("vcfFreq", fVal); break;
                case JunoSysEx::VCF_RES: setParam("resonance", fVal); break;
                case JunoSysEx::VCF_ENV: setParam("envAmount", fVal); break;
                case JunoSysEx::VCF_LFO: setParam("lfoToVCF", fVal); break;
                case JunoSysEx::VCF_KYBD: setParam("kybdTracking", fVal); break;
                case JunoSysEx::VCA_LEVEL: setParam("vcaLevel", fVal); break;
                case JunoSysEx::ENV_A: setParam("attack", fVal); break;
                case JunoSysEx::ENV_D: setParam("decay", fVal); break;
                case JunoSysEx::ENV_S: setParam("sustain", fVal); break;
                case JunoSysEx::ENV_R: setParam("release", fVal); break;
                case JunoSysEx::DCO_SUB: setParam("subOsc", fVal); break;
                
                case JunoSysEx::SWITCHES_1:
                {
                    setParam("dcoRange", (p2 & 1) ? 0.0f : ((p2 & 2) ? 1.0f : 2.0f));
                    setParam("pulseOn", (p2 & (1 << 3)) != 0);
                    setParam("sawOn", (p2 & (1 << 4)) != 0);
                    bool chorusOn = (p2 & (1 << 5)) == 0;
                    bool chorusMode1 = (p2 & (1 << 6)) != 0;
                    setParam("chorus1", chorusOn && chorusMode1);
                    setParam("chorus2", chorusOn && !chorusMode1);
                    break;
                }
                case JunoSysEx::SWITCHES_2:
                {
                    setParam("pwmMode", (p2 & (1 << 0)) != 0);
                    setParam("vcaMode", (p2 & (1 << 1)) != 0);
                    setParam("vcfPolarity", (p2 & (1 << 2)) != 0);
                    int hpf = (p2 >> 3) & 0x03;
                    setParam("hpfFreq", (float)hpf);
                    break;
                }
            }
        }
        else if (type == JunoSysEx::kMsgPatchDump && dumpData.size() >= 18)
        {
            setParam("lfoRate", dumpData[0] / 127.0f);
            setParam("lfoDelay", dumpData[1] / 127.0f);
            setParam("lfoToDCO", dumpData[2] / 127.0f);
            setParam("pwm", dumpData[3] / 127.0f);
            setParam("noise", dumpData[4] / 127.0f);
            setParam("vcfFreq", dumpData[5] / 127.0f);
            setParam("resonance", dumpData[6] / 127.0f);
            setParam("envAmount", dumpData[7] / 127.0f);
            setParam("lfoToVCF", dumpData[8] / 127.0f);
            setParam("kybdTracking", dumpData[9] / 127.0f);
            setParam("vcaLevel", dumpData[10] / 127.0f);
            setParam("attack", dumpData[11] / 127.0f);
            setParam("decay", dumpData[12] / 127.0f);
            setParam("sustain", dumpData[13] / 127.0f);
            setParam("release", dumpData[14] / 127.0f);
            setParam("subOsc", dumpData[15] / 127.0f);
            
            uint8_t sw1 = dumpData[16];
            setParam("dcoRange", (sw1 & 1) ? 0.0f : ((sw1 & 2) ? 1.0f : 2.0f));
            setParam("pulseOn", (sw1 & (1 << 3)) != 0);
            setParam("sawOn", (sw1 & (1 << 4)) != 0);
            bool chorusOn = (sw1 & (1 << 5)) == 0;
            bool chorusMode1 = (sw1 & (1 << 6)) != 0;
            setParam("chorus1", chorusOn && chorusMode1);
            setParam("chorus2", chorusOn && !chorusMode1);

            uint8_t sw2 = dumpData[17];
            setParam("pwmMode", (sw2 & (1 << 0)) != 0);
            setParam("vcaMode", (sw2 & (1 << 1)) != 0);
            setParam("vcfPolarity", (sw2 & (1 << 2)) != 0);
            int hpf = (sw2 >> 3) & 0x03;
            setParam("hpfFreq", (float)hpf);
        }
    }
}

void SimpleJuno106AudioProcessor::sendPatchDump()
{
    if (midiOutEnabled) midiOutBuffer.addEvent(generatePatchDumpMessage(), 0);
}

juce::MidiMessage SimpleJuno106AudioProcessor::generatePatchDumpMessage()
{
    uint8_t params[16];
    params[0] = static_cast<uint8_t>(currentParams.lfoRate * 127.0f);
    params[1] = static_cast<uint8_t>(currentParams.lfoDelay * 127.0f);
    params[2] = static_cast<uint8_t>(currentParams.lfoToDCO * 127.0f);
    params[3] = static_cast<uint8_t>(currentParams.pwmAmount * 127.0f);
    params[4] = static_cast<uint8_t>(currentParams.noiseLevel * 127.0f);
    params[5] = static_cast<uint8_t>(currentParams.vcfFreq * 127.0f);
    params[6] = static_cast<uint8_t>(currentParams.resonance * 127.0f);
    params[7] = static_cast<uint8_t>(currentParams.envAmount * 127.0f);
    params[8] = static_cast<uint8_t>(currentParams.lfoToVCF * 127.0f);
    params[9] = static_cast<uint8_t>(currentParams.kybdTracking * 127.0f);
    params[10] = static_cast<uint8_t>(currentParams.vcaLevel * 127.0f);
    params[11] = static_cast<uint8_t>(currentParams.attack * 127.0f);
    params[12] = static_cast<uint8_t>(currentParams.decay * 127.0f);
    params[13] = static_cast<uint8_t>(currentParams.sustain * 127.0f);
    params[14] = static_cast<uint8_t>(currentParams.release * 127.0f);
    params[15] = static_cast<uint8_t>(currentParams.subOscLevel * 127.0f);

    uint8_t sw1 = 0;
    if (currentParams.dcoRange == 0) sw1 |= (1 << 0);
    if (currentParams.dcoRange == 1) sw1 |= (1 << 1);
    if (currentParams.dcoRange == 2) sw1 |= (1 << 2);
    if (currentParams.pulseOn) sw1 |= (1 << 3);
    if (currentParams.sawOn) sw1 |= (1 << 4);
    if (!(currentParams.chorus1 || currentParams.chorus2)) sw1 |= (1 << 5); 
    if (currentParams.chorus1) sw1 |= (1 << 6); 

    uint8_t sw2 = 0;
    if (currentParams.pwmMode == 1) sw2 |= (1 << 0);
    if (currentParams.vcaMode == 1) sw2 |= (1 << 1);
    if (currentParams.vcfPolarity == 1) sw2 |= (1 << 2);
    
    int hpfVal = currentParams.hpfFreq;
    sw2 |= (static_cast<uint8_t>(hpfVal & 0x03) << 3);

    return JunoSysEx::createPatchDump(midiChannel - 1, 0, params, sw1, sw2);
}

PresetManager* SimpleJuno106AudioProcessor::getPresetManager() { return presetManager.get(); }

juce::AudioProcessorValueTreeState::ParameterLayout SimpleJuno106AudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto makeParam = [](juce::String id, juce::String name, float min, float max, float def) { return std::make_unique<juce::AudioParameterFloat>(id, name, min, max, def); };
    auto makeIntParam = [](juce::String id, juce::String name, int min, int max, int def) { return std::make_unique<juce::AudioParameterInt>(id, name, min, max, def); };
    auto makeBool = [](juce::String id, juce::String name, bool def) { return std::make_unique<juce::AudioParameterBool>(id, name, def); };

    params.push_back(makeIntParam("dcoRange", "DCO Range", 0, 2, 1));
    params.push_back(makeBool("sawOn", "DCO Saw", true));
    params.push_back(makeBool("pulseOn", "DCO Pulse", false));
    params.push_back(makeParam("pwm", "PWM Level", 0.0f, 1.0f, 0.0f));
    params.push_back(makeIntParam("pwmMode", "PWM Mode", 0, 1, 0));
    params.push_back(makeParam("subOsc", "Sub Osc Level", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("noise", "Noise Level", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("lfoToDCO", "LFO to DCO", 0.0f, 1.0f, 0.0f));
    params.push_back(makeIntParam("hpfFreq", "HPF Freq", 0, 3, 0));
    params.push_back(makeParam("vcfFreq", "VCF Freq", 0.0f, 1.0f, 1.0f));
    params.push_back(makeParam("resonance", "Resonance", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("envAmount", "Env Amount", 0.0f, 1.0f, 0.0f));
    params.push_back(makeIntParam("vcfPolarity", "VCF Polarity", 0, 1, 0));
    params.push_back(makeParam("kybdTracking", "VCF Kykd Track", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("lfoToVCF", "LFO to VCF", 0.0f, 1.0f, 0.0f));
    params.push_back(makeIntParam("vcaMode", "VCA Mode", 0, 1, 0));
    params.push_back(makeParam("vcaLevel", "VCA Level", 0.0f, 1.0f, 1.0f));
    params.push_back(makeParam("attack", "Attack", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("decay", "Decay", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("sustain", "Sustain", 0.0f, 1.0f, 1.0f));
    params.push_back(makeParam("release", "Release", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("lfoRate", "LFO Rate", 0.0f, 1.0f, 0.5f));
    params.push_back(makeParam("lfoDelay", "LFO Delay", 0.0f, 1.0f, 0.0f));
    params.push_back(makeBool("chorus1", "Chorus I", false));
    params.push_back(makeBool("chorus2", "Chorus II", false));
    params.push_back(makeIntParam("polyMode", "Poly Mode", 1, 3, 1));
    params.push_back(makeParam("portamentoTime", "Portamento Time", 0.0f, 1.0f, 0.0f));
    params.push_back(makeBool("portamentoOn", "Portamento On", false));
    params.push_back(makeParam("bender", "Bender", -1.0f, 1.0f, 0.0f));
    params.push_back(makeParam("benderToDCO", "Bender to DCO", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("benderToVCF", "Bender to VCF", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("benderToLFO", "Bender to LFO", 0.0f, 1.0f, 0.0f));
    params.push_back(makeParam("tune", "Master Tune", -50.0f, 50.0f, 0.0f));
    params.push_back(makeBool("midiOut", "MIDI Out Enabled", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SimpleJuno106AudioProcessor(); }

// Missing Editor & State implementation
bool SimpleJuno106AudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* SimpleJuno106AudioProcessor::createEditor() {
    return new SimpleJuno106AudioProcessorEditor(*this);
}

void SimpleJuno106AudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SimpleJuno106AudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType().toString()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
