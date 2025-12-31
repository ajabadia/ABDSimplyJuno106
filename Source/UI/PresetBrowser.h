#pragma once

#include <JuceHeader.h>
#include "../Core/PresetManager.h"

/**
 * PresetBrowser - UI component for preset selection
 * 
 * Features:
 * - Preset combo box
 * - Previous/Next buttons
 * - Save button
 * - Integrates with PresetManager
 */
class PresetBrowser : public juce::Component,
                      public juce::ComboBox::Listener,
                      public juce::Button::Listener {
public:
    PresetBrowser(PresetManager& pm);
    ~PresetBrowser() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Callbacks
    void comboBoxChanged(juce::ComboBox* box) override;
    void buttonClicked(juce::Button* button) override;
    
    // Update preset list
    void refreshPresetList();
    juce::String getSelectedPreset() const { return presetCombo.getText(); }
    void loadPreset(int index);
    
    // Set callback for preset change
    std::function<void(const juce::String&)> onPresetChanged;
    std::function<juce::ValueTree()> onGetCurrentState;
    
private:
    PresetManager& presetManager;
    
    juce::ComboBox presetCombo;
    juce::TextButton prevButton;
    juce::TextButton nextButton;
    juce::TextButton saveButton;
    
    void showSaveDialog();
    
    std::unique_ptr<juce::AlertWindow> saveDialog;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
