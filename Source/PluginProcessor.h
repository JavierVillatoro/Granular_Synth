/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GranularVoice.h"
#include "LfoModule.h"

class Granular_SynthAudioProcessor : public juce::AudioProcessor
{
public:
    Granular_SynthAudioProcessor();
    ~Granular_SynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void loadFile(const juce::String& path, int layerIndex);

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // --- LOS 3 JEFES ---
    juce::Synthesiser& getSynthesiserL1() { return synthL1; }
    juce::Synthesiser& getSynthesiserL2() { return synthL2; }
    juce::Synthesiser& getSynthesiserL3() { return synthL3; }

    // --- DISCOS DUROS PARA LAS CAPAS ---
    juce::AudioBuffer<float> audioBufferL1;
    juce::AudioBuffer<float> audioBufferL2;
    juce::AudioBuffer<float> audioBufferL3;

    juce::AudioBuffer<float>& getAudioBufferL1() { return audioBufferL1; }
    juce::AudioBuffer<float>& getAudioBufferL2() { return audioBufferL2; }
    juce::AudioBuffer<float>& getAudioBufferL3() { return audioBufferL3; }

    bool isAudioLoadedL1 = false;
    bool isAudioLoadedL2 = false;
    bool isAudioLoadedL3 = false;

    juce::String lastLoadedFilePathL1 = "";
    juce::String lastLoadedFilePathL2 = "";
    juce::String lastLoadedFilePathL3 = "";

    // --- CÁMARAS DE ZOOM INDEPENDIENTES ---
    std::atomic<float> windowStartRatioL1{ 0.0f };
    std::atomic<float> windowLengthRatioL1{ 1.0f };

    std::atomic<float> windowStartRatioL2{ 0.0f };
    std::atomic<float> windowLengthRatioL2{ 1.0f };

    std::atomic<float> windowStartRatioL3{ 0.0f };
    std::atomic<float> windowLengthRatioL3{ 1.0f };

    std::vector<LfoNode> savedLfoNodes;
    bool isLfoSaved = false;

    float globalLfo1Value = 0.0f;
    float globalLfo2Value = 0.0f;

    static constexpr int LFO_TABLE_SIZE = 2048;
    std::array<float, LFO_TABLE_SIZE> lfo2Table = { 0.0f };

    std::atomic<float> visualLpfCutoff{ 20000.0f };
    std::atomic<float> visualHpfCutoff{ 20.0f };
    std::atomic<float> visualMeterL{ -60.0f };
    std::atomic<float> visualMeterR{ -60.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::AudioFormatManager formatManager;

    juce::Synthesiser synthL1;
    juce::Synthesiser synthL2;
    juce::Synthesiser synthL3;

    juce::dsp::Reverb reverbL1;
    juce::dsp::Reverb reverbL2;
    juce::dsp::Reverb reverbL3;

    juce::AudioBuffer<float> renderBufferL1;
    juce::AudioBuffer<float> renderBufferL2;
    juce::AudioBuffer<float> renderBufferL3;

    juce::dsp::Limiter<float> masterLimiter;

    float lfo1Phase = 0.0f;
    float lfo2Phase = 0.0f;

    double currentSampleRate = 44100.0;
    double currentBPM = 120.0;
    bool isPlaying = false;

    bool lastPlayState = false;
    bool lastHoldState = false;

    bool lastPlayStateL2 = false;
    bool lastHoldStateL2 = false;

    bool lastPlayStateL3 = false;
    bool lastHoldStateL3 = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Granular_SynthAudioProcessor)
};
