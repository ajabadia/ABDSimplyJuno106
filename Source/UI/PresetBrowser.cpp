#include "PresetBrowser.h"

PresetBrowser::PresetBrowser(PresetManager& pm)
    : presetManager(pm) {
    
    // Preset combo box
    addAndMakeVisible(presetCombo);
    presetCombo.addListener(this);
    refreshPresetList();
    
    // Previous button
    addAndMakeVisible(prevButton);
    prevButton.setButtonText("<");
    prevButton.addListener(this);
    
    // Next button
    addAndMakeVisible(nextButton);
    nextButton.setButtonText(">");
    nextButton.addListener(this);
    
    // Save button
    addAndMakeVisible(saveButton);
    saveButton.setButtonText("SAVE");
    saveButton.addListener(this);
}

void PresetBrowser::paint(juce::Graphics& g) {
    // Background
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
    
    // Title
    g.setColour(juce::Colours::lightblue);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("PRESETS", 0, 5, getWidth(), 20, juce::Justification::centred);
}

void PresetBrowser::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromTop(25); // Title space
    bounds.reduce(5, 5);
    
    int buttonWidth = 30;
    int saveWidth = 50;
    
    // Layout: [<] [Combo] [>] [SAVE]
    prevButton.setBounds(bounds.removeFromLeft(buttonWidth));
    bounds.removeFromLeft(5);
    
    saveButton.setBounds(bounds.removeFromRight(saveWidth));
    bounds.removeFromRight(5);
    
    nextButton.setBounds(bounds.removeFromRight(buttonWidth));
    bounds.removeFromRight(5);
    
    presetCombo.setBounds(bounds);
}

void PresetBrowser::refreshPresetList() {
    presetCombo.clear();
    
    auto names = presetManager.getPresetNames();
    for (int i = 0; i < names.size(); ++i) {
        presetCombo.addItem(names[i], i + 1);
    }
    
    // Select current preset by index
    int index = presetManager.getCurrentPresetIndex();
    presetCombo.setSelectedId(index + 1, juce::dontSendNotification);
}

void PresetBrowser::comboBoxChanged(juce::ComboBox* box) {
    if (box == &presetCombo) {
        loadPreset(presetCombo.getSelectedItemIndex());
    }
}

void PresetBrowser::buttonClicked(juce::Button* button) {
    if (button == &prevButton) {
        int currentIndex = presetCombo.getSelectedItemIndex();
        if (currentIndex > 0) {
            loadPreset(currentIndex - 1);
            presetCombo.setSelectedId(currentIndex, juce::dontSendNotification);
        }
    }
    else if (button == &nextButton) {
        int currentIndex = presetCombo.getSelectedItemIndex();
        if (currentIndex < presetCombo.getNumItems() - 1) {
            loadPreset(currentIndex + 1);
            presetCombo.setSelectedId(currentIndex + 2, juce::dontSendNotification);
        }
    }
    else if (button == &saveButton) {
        showSaveDialog();
    }
}

void PresetBrowser::loadPreset(int index) {
    presetManager.setCurrentPreset(index);
    
    if (onPresetChanged) {
        onPresetChanged(juce::String(index));
    }
}

void PresetBrowser::showSaveDialog() {
    auto presetName = juce::String("Preset_") + juce::String(juce::Random::getSystemRandom().nextInt(100));
    
    saveDialog = std::make_unique<juce::AlertWindow>("Save Preset", "", juce::MessageBoxIconType::NoIcon);
    saveDialog->addTextEditor("name", presetName);
    saveDialog->addButton("Save", 1);
    saveDialog->addButton("Cancel", 0);
    
    saveDialog->enterModalState(true, juce::ModalCallbackFunction::create([this](int result) {
        if (result == 1 && saveDialog != nullptr) {
            auto name = saveDialog->getTextEditorContents("name");
            
            if (name.isNotEmpty()) {
                if (onGetCurrentState) {
                    auto state = onGetCurrentState();
                    presetManager.saveUserPreset(name, state);
                    refreshPresetList();
                    
                    // Note: User presets now go to a specific bank in the new refactor
                }
            }
        }
        saveDialog.reset();
    }));
}
