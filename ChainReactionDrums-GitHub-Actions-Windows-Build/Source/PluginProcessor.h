#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class ChainReactionDrumsAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int nodeCount = 8;
    ChainReactionDrumsAudioProcessor();
    ~ChainReactionDrumsAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Chain Reaction Drums"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 10; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    void loadSample(int node, const juce::File& file);
    juce::String getSampleName(int node) const;
    void setEdge(int from, int to, float probability);
    float getEdge(int from, int to) const;
    void randomizePattern();
    void setPatternLocked(bool shouldLock) { patternLocked.store(shouldLock); }
    bool isPatternLocked() const { return patternLocked.load(); }
    void savePreset(const juce::File& file);
    void loadPreset(const juce::File& file);

private:
    struct Voice { juce::AudioBuffer<float> data; int position = -1; double sourceRate = 44100.0; float gain = 1.0f; };
    std::array<Voice, nodeCount> voices;
    std::array<std::array<std::atomic<float>, nodeCount>, nodeCount> edges;
    std::array<std::atomic<float>, nodeCount> nodeChance;
    std::array<juce::String, nodeCount> sampleNames;
    juce::Random random;
    double sr = 44100.0;
    int64_t sampleClock = 0;
    int64_t nextTick = 0;
    int step = 0;
    int currentProgram = 0;
    std::atomic<bool> patternLocked { false };
    float fxEnvelope = 0.0f;
    juce::dsp::StateVariableTPTFilter<float> lowpass;
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0;
    void triggerNode(int node, juce::MidiBuffer& midi, int sampleOffset, int depth = 0);
    void renderVoice(Voice& voice, juce::AudioBuffer<float>& out, int start, int count);
    float param(const char* id) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainReactionDrumsAudioProcessor)
};
