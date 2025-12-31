cmake_minimum_required(VERSION 3.22)

project(ABDSimpleJuno106 VERSION 0.0.1)

# JUCE path
set(JUCE_PATH "C:/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory(${JUCE_PATH} _juce)

juce_add_plugin(ABDSimpleJuno106
    # VERSION 0.0.1                     # The plugin's version
    # ICON_BIG ""                       # Relevant for Standalone
    # ICON_SMALL ""                     # Relevant for Standalone
    # COMPANY_NAME "ABD"                # Your company name
    IS_SYNTH TRUE                       # Is this a synth?
    NEEDS_MIDI_INPUT TRUE               # Does it need MIDI input?
    NEEDS_MIDI_OUTPUT TRUE              # Does it need MIDI output?
    IS_MIDI_EFFECT FALSE                # Is it a MIDI effect?
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE    # Does the editor need keyboard focus?
    # COPY_PLUGIN_AFTER_BUILD TRUE      # Whether to copy the plugin after building
    PLUGIN_MANUFACTURER_CODE Abda       # A four-character manufacturer id
    PLUGIN_CODE J106                    # A four-character unique plugin id
    FORMATS Standalone                  # Only Standalone for now
    PRODUCT_NAME "ABDSimpleJuno106"     # The name of the final executable/plugin
)

target_sources(ABDSimpleJuno106 PRIVATE
    Source/Core/PluginProcessor.h
    Source/Core/PluginProcessor.cpp
    Source/Core/PluginEditor.h
    Source/Core/PluginEditor.cpp
    Source/Core/PresetManager.h
    Source/Core/PresetManager.cpp
    Source/Core/SynthParams.h
    Source/Core/JunoVoiceManager.h
    Source/Core/JunoVoiceManager.cpp
    Source/Core/JunoTapeDecoder.h
    Source/Core/JunoTapeDecoder.cpp
    
    Source/Synth/JunoADSR.h
    Source/Synth/JunoADSR.cpp
    Source/Synth/JunoDCO.h
    Source/Synth/JunoDCO.cpp
    Source/Synth/JunoLFO.h
    Source/Synth/JunoLFO.cpp
    Source/Synth/Voice.h
    Source/Synth/Voice.cpp
    
    Source/UI/JunoBender.h
    Source/UI/JunoBender.cpp
    Source/UI/JunoUIHelpers.h
    Source/UI/PresetBrowser.h
    Source/UI/PresetBrowser.cpp
    
    Source/UI/Sections/JunoChorusSection.h
    Source/UI/Sections/JunoControlSection.h
    Source/UI/Sections/JunoDCOSection.h
    Source/UI/Sections/JunoENVSection.h
    Source/UI/Sections/JunoHPFSection.h
    Source/UI/Sections/JunoLFOSection.h
    Source/UI/Sections/JunoVCASection.h
    Source/UI/Sections/JunoVCFSection.h
)

target_include_directories(ABDSimpleJuno106 PRIVATE Source)

juce_generate_juce_header(ABDSimpleJuno106)

target_compile_definitions(ABDSimpleJuno106
    PUBLIC
        # JUCE_WEB_BROWSER=0  # If you don't need a web browser
        JUCE_USE_CURL=0    # If you don't need CURL
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ABDSimpleJuno106
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_audio_basics
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_events
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
