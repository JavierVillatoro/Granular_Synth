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

    juce::AudioBuffer<float>* myBuffer;
    juce::AudioProcessorValueTreeState* apvts;
    juce::String myPrefix;

    juce::dsp::StateVariableTPTFilter<float> lpf[2];
    juce::dsp::StateVariableTPTFilter<float> hpf[2];

    // --- NUEVO: 4 MONJES x 3 PICOS FORMANTES x 2 CANALES ---
    // formantF[monje][pico][canal]
    juce::dsp::StateVariableTPTFilter<float> formantFilters[4][3][2];

    juce::ADSR ampAdsr;
    juce::ADSR::Parameters ampAdsrParams;

    juce::dsp::IIR::Filter<float> eqLowFilter[2];
    juce::dsp::IIR::Filter<float> eqMidLowFilter[2];
    juce::dsp::IIR::Filter<float> eqMidHighFilter[2];
    juce::dsp::IIR::Filter<float> eqHighFilter[2];

    // =======================================================================
    // ARQUITECTURA  (Ruteo Paralelo)
    // =======================================================================

    // 1. LA TIERRA: Filtro Pasa-Bajos para aislar el subgrave puro
    juce::dsp::StateVariableTPTFilter<float> subCrossoverFilter[2];

    // 2. LA HUMANIDAD: Algoritmo de Chorus Multilínea de JUCE
    juce::dsp::Chorus<float> ensembleFX;

    // 3. EL CIELO: Delay Line y Filtro Pasa-Altos para el Halo (Shimmer)
    // Reservamos memoria para 2 segundos de delay máximo a 48kHz o 96kHz (100000 muestras)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> haloDelay{ 100000 };
    juce::dsp::StateVariableTPTFilter<float> haloHighpass[2];

    //LFO del Halo
    //float haloLfoPhase = 0.0f;
    float haloPhasor = 0.0f;
};
