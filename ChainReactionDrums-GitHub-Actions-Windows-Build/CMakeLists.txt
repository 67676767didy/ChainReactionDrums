cmake_minimum_required(VERSION 3.22)
project(ChainReactionDrums VERSION 0.1.0)

option(CRD_FETCH_JUCE "Fetch JUCE if it is not installed" ON)
if(CRD_FETCH_JUCE)
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
else()
  find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_plugin(ChainReactionDrums
  COMPANY_NAME "Open Audio Lab"
  BUNDLE_ID "audio.openlab.chainreactiondrums"
  PLUGIN_MANUFACTURER_CODE OaLb
  PLUGIN_CODE Crd1
  FORMATS VST3 Standalone
  PRODUCT_NAME "Chain Reaction Drums"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(ChainReactionDrums PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginProcessor.h
  Source/PluginEditor.cpp
  Source/PluginEditor.h
)
target_compile_features(ChainReactionDrums PRIVATE cxx_std_17)
target_compile_definitions(ChainReactionDrums PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(ChainReactionDrums PRIVATE
  juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags
)
