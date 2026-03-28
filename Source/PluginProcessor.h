/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GranularVoice.h"
#include "LfoModule.h"

//==============================================================================
/**
*/
class Granular_SynthAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Granular_SynthAudioProcessor();
    ~Granular_SynthAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void loadFile(const juce::String& path);

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    juce::Synthesiser& getSynthesiser() { return synth; }

    juce::AudioBuffer<float>& getAudioBuffer() { return audioBuffer; }
    bool isAudioLoaded = false;

    // Memoria ventana de zoom
    std::atomic<float> windowStartRatio{ 0.0f };
    std::atomic<float> windowLengthRatio{ 1.0f };

    std::vector<LfoNode> savedLfoNodes;
    bool isLfoSaved = false;

    juce::String lastLoadedFilePath = "";

    // Estas variables guardan el valor exacto del LFO en este preciso instante.
    // Las Voces Granulares las leerán para saber cómo tienen que moverse.
    float globalLfo1Value = 0.0f; // Oscilará entre -1.0 y 1.0
    float globalLfo2Value = 0.0f; // Oscilará entre 0.0 y 1.0 (Vectorial)

    // ==========================================================
    // --- MEMORIA WAVETABLE PARA EL LFO 2 
    // ==========================================================
    static constexpr int LFO_TABLE_SIZE = 2048; // High resolution 
    std::array<float, LFO_TABLE_SIZE> lfo2Table = { 0.0f }; // Zeros table

    // --- VARIABLES VISUALES (Los espías para la Interfaz) ---
    std::atomic<float> visualLpfCutoff{ 20000.0f };
    std::atomic<float> visualHpfCutoff{ 20.0f };

    // Mueve estas dos líneas AQUÍ a la zona pública:
    std::atomic<float> visualMeterL{ -60.0f };
    std::atomic<float> visualMeterR{ -60.0f };

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    //==============================================================================
    //juce::AudioProcessorValueTreeState apvts;
    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> audioBuffer;
    //==============================================================================

    juce::Synthesiser synth;

    juce::dsp::Reverb masterReverb;
    juce::Reverb::Parameters reverbParams;

    // LIMITER ---
    juce::dsp::Limiter<float> masterLimiter;

    // max_vol channel
    //std::atomic<float> visualMeterL{ -60.0f };
    //std::atomic<float> visualMeterR{ -60.0f };

    // ==========================================================
    // --- RELOJES DSP INTERNOS (PHASE ACCUMULATORS) ---
    // ==========================================================
    float lfo1Phase = 0.0f; // El contador de tiempo del LFO 1 (va de 0.0 a 1.0)
    float lfo2Phase = 0.0f; // El contador de tiempo del LFO 2 (va de 0.0 a 1.0)

    // ==========================================================
    // --- DATOS DEL ENTORNO (DAW / STANDALONE) ---
    // ==========================================================
    double currentSampleRate = 44100.0; // Cuántas "fotos" de audio hacemos por segundo
    double currentBPM = 120.0;          // El tempo actual (por defecto 120)
    bool isPlaying = false;             // ¿El DAW está dándole al Play?

    bool lastPlayState = false;
    bool lastHoldState = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Granular_SynthAudioProcessor)
};
