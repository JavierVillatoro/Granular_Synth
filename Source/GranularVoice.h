/*
  ==============================================================================

    GranularVoice.h
    Created: 15 Mar 2026 7:08:37pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cmath>

class Granular_SynthAudioProcessor;

// ==============================================================================
// 1. LA CLASE SOUND
class GranularSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int midiNoteNumber) override { return true; }
    bool appliesToChannel(int midiChannel) override { return true; }
};

// ==============================================================================
// 2. LA CLASE VOICE
class GranularVoice : public juce::SynthesiserVoice
{
public:
    // EL NUEVO CONSTRUCTOR UNIVERSAL
    GranularVoice(juce::AudioBuffer<float>* buffer, juce::AudioProcessorValueTreeState* apvtsToUse, juce::String prefix);

    float currentLfo1Value = 0.0f;
    float currentLfo2Value = 0.0f;

    std::atomic<float> visualGrainPos[128];
    std::atomic<float> visualGrainEnv[128];

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    struct Grain
    {
        bool isActive = false;
        double currentPosition = 0;
        int startSample = 0;
        float randomPitch = 1.0f;
        float pitchRandomRatio = 1.0f;
        float panL = 1.0f;
        float panR = 1.0f;
        float activePitchRatio = 1.0f;
    };

    static constexpr int maxGrains = 128;
    std::array<Grain, maxGrains> grains;

    double samplesUntilNextGrain = 0.0;

    bool isPlaying = false;
    double autoScanOffset = 0.0;
    float currentVelocity = 0.0f;
    float pitchRatio = 1.0f;

    // --- LAS LLAVES PRIVADAS DE ESTA VOZ ---
    juce::AudioBuffer<float>* myBuffer;
    juce::AudioProcessorValueTreeState* apvts;
    juce::String myPrefix; // Guardará "L1_" o "L2_" o "L3_"

    juce::dsp::StateVariableTPTFilter<float> lpf[2];
    juce::dsp::StateVariableTPTFilter<float> hpf[2];

    juce::ADSR ampAdsr;
    juce::ADSR::Parameters ampAdsrParams;

    juce::dsp::IIR::Filter<float> eqLowFilter[2];
    juce::dsp::IIR::Filter<float> eqMidLowFilter[2];
    juce::dsp::IIR::Filter<float> eqMidHighFilter[2];
    juce::dsp::IIR::Filter<float> eqHighFilter[2];
};
