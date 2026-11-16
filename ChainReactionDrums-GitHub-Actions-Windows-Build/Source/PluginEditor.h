#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ChainReactionDrumsAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer, private juce::FileDragAndDropTarget
{
public:
    explicit ChainReactionDrumsAudioProcessorEditor(ChainReactionDrumsAudioProcessor&);
    ~ChainReactionDrumsAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped(const juce::StringArray&, int, int) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    ChainReactionDrumsAudioProcessor& processor;
    std::array<std::unique_ptr<juce::Slider>,8> knobs;
    std::array<std::unique_ptr<juce::Label>,8> knobLabels;
    std::array<std::unique_ptr<juce::TextButton>,8> sampleButtons;
    std::unique_ptr<juce::Slider> chaos, damage, dropout, tape, half, verb, tempo, master;
    std::unique_ptr<juce::Label> title, subtitle, status;
    std::unique_ptr<juce::TextButton> playLock, randomize, save, load;
    std::array<juce::Point<float>,8> nodePoints;
    int selectedNode=-1, dragFrom=-1;
    bool draggingEdge=false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosA,damageA,dropoutA,tapeA,halfA,verbA,tempoA,masterA;
    void timerCallback() override;
    void setupKnob(juce::Slider&,juce::Label&,const juce::String&,double,double,double);
    juce::Rectangle<float> graphArea() const;
    int hitNode(juce::Point<float>) const;
    void chooseSample(int node);
    void choosePreset(bool saving);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainReactionDrumsAudioProcessorEditor)
};
