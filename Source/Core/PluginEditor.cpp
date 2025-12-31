#include "PluginEditor.h"

SimpleJuno106AudioProcessorEditor::SimpleJuno106AudioProcessorEditor(SimpleJuno106AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      lfoSection(p.getAPVTS(), p.midiLearnHandler),
      dcoSection(p.getAPVTS(), p.midiLearnHandler),
      hpfSection(p.getAPVTS(), p.midiLearnHandler),
      vcfSection(p.getAPVTS(), p.midiLearnHandler),
      vcaSection(p.getAPVTS(), p.midiLearnHandler),
      envSection(p.getAPVTS(), p.midiLearnHandler),
      chorusSection(p.getAPVTS(), p.midiLearnHandler),
      controlSection(p, p.getAPVTS(), *p.getPresetManager(), p.midiLearnHandler),
      midiKeyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);
    setSize(1300, 600);

    addAndMakeVisible(lfoSection);
    addAndMakeVisible(dcoSection);
    addAndMakeVisible(hpfSection);
    addAndMakeVisible(vcfSection);
    addAndMakeVisible(vcaSection);
    addAndMakeVisible(envSection);
    addAndMakeVisible(chorusSection);
    addAndMakeVisible(controlSection);
    addAndMakeVisible(midiKeyboard);

    audioProcessor.keyboardState.addListener(&audioProcessor);

    // Callbacks
    controlSection.onPresetLoad = [this](int index) {
        audioProcessor.loadPreset(index);
    };
    
    controlSection.dumpButton.onClick = [this] {
        audioProcessor.sendPatchDump();
    };

    // Keyboard configuration: 4 octaves for significantly wider keys
    midiKeyboard.setAvailableRange(36, 84); // C2 to C6 (49 keys)
    midiKeyboard.setScrollButtonsVisible(false);
}

SimpleJuno106AudioProcessorEditor::~SimpleJuno106AudioProcessorEditor()
{
    audioProcessor.keyboardState.removeListener(&audioProcessor);
}

void SimpleJuno106AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(JunoUI::kJunoDarkGrey);
    
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("ABDSimplyJuno106", 20, 10, 300, 30, juce::Justification::left);
}

void SimpleJuno106AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    int topY = 50;
    int topH = 260;
    int curX = 10;
    int gap = 5;

    // Top Strip
    lfoSection.setBounds(curX, topY, 120, topH); curX += 120 + gap;
    dcoSection.setBounds(curX, topY, 420, topH); curX += 420 + gap;
    hpfSection.setBounds(curX, topY, 60, topH); curX += 60 + gap;
    vcfSection.setBounds(curX, topY, 320, topH); curX += 320 + gap;
    vcaSection.setBounds(curX, topY, 110, topH); curX += 110 + gap;
    envSection.setBounds(curX, topY, 210, topH); 

    // Middle Strip
    int midY = 320;
    int chorusW = 140;
    controlSection.setBounds(10, midY, getWidth() - chorusW - 30, 130);
    chorusSection.setBounds(getWidth() - chorusW - 10, midY, chorusW, 130); 
    
    // Keyboard placement - reaching bottom, ultra-wide notes
    int keyY = 445; // Moved up slightly to give more height
    midiKeyboard.setBounds(10, keyY, getWidth() - 20, getHeight() - keyY - 5);
    
    // Explicitly set key width to fill the 1280px area (29 white keys)
    // 1280 / 29 = ~44.1f per white key
    midiKeyboard.setKeyWidth(44.0f);
    
    // Ensure keyboard is on top
    midiKeyboard.toFront(false);
}
