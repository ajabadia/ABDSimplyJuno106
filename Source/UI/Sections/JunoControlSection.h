#pragma once
#include <JuceHeader.h>
#include "../JunoUIHelpers.h"
#include "../../UI/JunoBender.h"
#include "../../UI/PresetBrowser.h"
#include "../../Core/PluginProcessor.h"

// Bottom Panel: Bender, Key Control, Mode, Presets
class JunoControlSection : public juce::Component, public juce::Timer
{
public:
    JunoControlSection(juce::AudioProcessor& p, juce::AudioProcessorValueTreeState& apvts, PresetManager& pm, MidiLearnHandler& mlh)
        : presetBrowser(pm), processor(p)
    {
        // Bender
        addAndMakeVisible(bender);
        bender.attachToParameters(apvts);

        // Portamento
        JunoUI::setupRotarySlider(portSlider);
        addAndMakeVisible(portSlider);
        portAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "portamentoTime", portSlider);
        JunoUI::setupLabel(portLabel, "PORT", *this);

        addAndMakeVisible(portButton);
        portButton.setButtonText("ON");
        portButtonAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "portamentoOn", portButton);

        // Assign Mode
        addAndMakeVisible(modeCombo);
        modeCombo.addItem("POLY 1", 1);
        modeCombo.addItem("POLY 2", 2);
        modeCombo.addItem("UNISON", 3);
        modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "polyMode", modeCombo);
        JunoUI::setupLabel(modeLabel, "ASSIGN", *this);

        // Preset Browser
        addAndMakeVisible(presetBrowser);
        
        // MIDI Out
        midiOutButton.setButtonText("MIDI OUT");
        midiOutAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "midiOut", midiOutButton);

        // Transpose & Power (Secret combo for Test Mode)
        addAndMakeVisible(transposeButton);
        transposeButton.setButtonText("TRANSPOSE");
        transposeButton.setClickingTogglesState(true);

        addAndMakeVisible(powerButton);
        powerButton.setButtonText("POWER");
        powerButton.onClick = [this] {
            auto* proc = dynamic_cast<SimpleJuno106AudioProcessor*>(&processor);
            if (proc && transposeButton.getToggleState())
            {
                proc->enterTestMode(true);
            }
            else if (proc)
            {
                proc->enterTestMode(false);
            }
        };

        // Bank Buttons (Simulated by 1-8 keys or buttons)
        for (int i = 0; i < 8; ++i)
        {
            bankButtons[i].setButtonText(juce::String(i + 1));
            addAndMakeVisible(bankButtons[i]);
            bankButtons[i].onClick = [this, i] {
                auto* proc = dynamic_cast<SimpleJuno106AudioProcessor*>(&processor);
                if (proc)
                {
                    if (proc->isTestMode)
                    {
                        proc->triggerTestProgram(i);
                    }
                    else
                    {
                        auto names = proc->getPresetManager()->getPresetNames();
                        if (i < (int)names.size())
                        {
                            presetBrowser.loadPreset(i);
                            presetBrowser.refreshPresetList();
                        }
                    }
                }
            };
        }

        // Callbacks
        presetBrowser.onPresetChanged = [&](const juce::String& /*indexStr*/) {
            auto* p = dynamic_cast<SimpleJuno106AudioProcessor*>(&processor);
            if (onPresetLoad && p) {
                 int idx = p->getPresetManager()->getCurrentPresetIndex();
                 if (idx >= 0) onPresetLoad(idx);
            }
        };
        
         presetBrowser.onGetCurrentState = [&]() {
            return apvts.copyState();
        };

        // MIDI Learn
        JunoUI::setupMidiLearn(portSlider, mlh, "portamentoTime", midiLearnListeners);
        JunoUI::setupMidiLearn(portButton, mlh, "portamentoOn", midiLearnListeners);
        JunoUI::setupMidiLearn(modeCombo, mlh, "polyMode", midiLearnListeners);

        // Multi-Bank Controls
        addAndMakeVisible(decBankButton);
        addAndMakeVisible(incBankButton);
        decBankButton.setButtonText("< BK");
        incBankButton.setButtonText("BK >");
        
        decBankButton.onClick = [this] {
            auto* proc = dynamic_cast<SimpleJuno106AudioProcessor*>(&processor);
            if (proc) {
                auto* pm = proc->getPresetManager();
                pm->selectBank(pm->getActiveBankIndex() - 1);
                presetBrowser.refreshPresetList();
            }
        };
        
        incBankButton.onClick = [this] {
            auto* proc = dynamic_cast<SimpleJuno106AudioProcessor*>(&processor);
            if (proc) {
                auto* pm = proc->getPresetManager();
                pm->selectBank(pm->getActiveBankIndex() + 1);
                presetBrowser.refreshPresetList();
            }
        };

        // Load Tape Button
        addAndMakeVisible(loadTapeButton);
        loadTapeButton.setButtonText("LOAD TAPE");
        loadTapeButton.onClick = [this] {
            auto* proc = dynamic_cast<SimpleJuno106AudioProcessor*>(&processor);
            if (proc) {
                fileChooser = std::make_unique<juce::FileChooser>("Select a Juno-106 Tape WAV file...",
                    juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                    "*.wav");
                
                auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
                fileChooser->launchAsync(flags, [this, proc](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file.existsAsFile()) {
                        auto result = proc->getPresetManager()->loadTape(file);
                        if (result.wasOk()) {
                            juce::String msg = "Successfully decoded patches from: " + file.getFileNameWithoutExtension();
                            if (result.getErrorMessage().isNotEmpty())
                                msg += "\n\nDetails: " + result.getErrorMessage();
                                
                            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                "Tape Loaded", msg);
                        }
                        else {
                            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                "Load Failed",
                                "Error loading tape: " + file.getFileName() + "\n\nReason: " + result.getErrorMessage());
                        }
                    }
                });
            }
        };

        addAndMakeVisible(lcd);
        startTimer(50); // 20Hz update for LCD
    }

    void timerCallback() override
    {
        auto* proc = dynamic_cast<SimpleJuno106AudioProcessor*>(&processor);
        if (proc)
        {
            if (proc->isTestMode)
            {
                lcd.setText("CH " + juce::String(proc->getVoiceManager().getLastTriggeredVoiceIndex() + 1));
            }
            else
            {
                auto* pm = proc->getPresetManager();
                int bankIdx = pm->getActiveBankIndex() + 1;
                int patchIdx = pm->getCurrentPresetIndex();
                
                // Show Bank-Patch (e.g. 1-11)
                int group = (patchIdx / 8) + 1;
                int prog = (patchIdx % 8) + 1;
                lcd.setText(juce::String(bankIdx) + "-" + juce::String(group) + juce::String(prog));
            }
        }
    }
    
    // Setter for callbacks
    std::function<void(int)> onPresetLoad;
    std::function<void()> onDump;

    void resized() override
    {
        // Layout: Bender (Left) -> Key Control -> Mode -> Presets
        int benderW = 120;
        bender.setBounds(0, 0, benderW, 130);
        
        int x = benderW + 10;
        
        // Portamento
        portLabel.setBounds(x, 10, 50, 20);
        portSlider.setBounds(x, 30, 50, 60);
        portButton.setBounds(x, 95, 50, 25);
        x += 60;
        
        // Mode
        modeLabel.setBounds(x, 10, 80, 20);
        modeCombo.setBounds(x, 35, 80, 25);
        x += 90;

        // Bank / Tape Controls
        decBankButton.setBounds(x, 10, 45, 25);
        incBankButton.setBounds(x + 50, 10, 45, 25);
        loadTapeButton.setBounds(x, 40, 95, 30);
        x += 105;

        // LCD placement
        lcd.setBounds(x, 10, 80, 40);
        x += 90;
        
        // Presets
        presetBrowser.setBounds(x, 10, 350, 90);
        x += 360;
        
        // Buttons
        powerButton.setBounds(x, 10, 60, 25);
        transposeButton.setBounds(x, 40, 80, 25);
        dumpButton.setBounds(x, 70, 60, 25);
        midiOutButton.setBounds(x, 95, 75, 25);
        x += 90;

        // Bank Buttons (Authentication: 8 buttons in 2 rows of 4, Aqua Style)
        // Row 1: 1-4
        // Row 2: 5-8
        int btnW = 40; // Wider
        int btnH = 30;
        int gap = 5;
        int startY = 30;
        
        for (int i = 0; i < 8; ++i)
        {
            bankButtons[i].getProperties().set("isPatchButton", true);
            
            int row = i / 4; // 0 or 1
            int col = i % 4; // 0 to 3
            
            bankButtons[i].setBounds(x + (col * (btnW + gap)), startY + (row * (btnH + gap)), btnW, btnH);
        }
    }

    PresetBrowser presetBrowser;
    juce::TextButton dumpButton;
    juce::ToggleButton midiOutButton;
    juce::TextButton transposeButton;
    juce::TextButton powerButton;
    juce::TextButton bankButtons[8];
    juce::TextButton incBankButton, decBankButton, loadTapeButton;
    JunoUI::JunoLCD lcd;

private:
    JunoBender bender;
    juce::Slider portSlider;
    juce::Label portLabel, modeLabel;
    juce::ToggleButton portButton;
    juce::ComboBox modeCombo;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> portAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> portButtonAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midiOutAtt;
    
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::OwnedArray<JunoUI::MidiLearnMouseListener> midiLearnListeners;
    juce::AudioProcessor& processor;
};
