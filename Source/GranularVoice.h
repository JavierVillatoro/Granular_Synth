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
    // ¡NUEVO CONSTRUCTOR! Recibe el audio y los parámetros
    GranularVoice(juce::AudioBuffer<float>* buffer, juce::AudioProcessorValueTreeState* apvtsToUse);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    
    struct Grain
    {
        bool isActive = false;      // ¿Está sonando o está en la reserva?
        double currentPosition = 0; // Por dónde va leyendo el audio (de 0 a grainLength)
        int startSample = 0;        // La "foto" de dónde empezó a leer en el archivo
        //float randomPan = 0.5f;     // SPRAY de paneo
        float randomPitch = 1.0f;
        float pitchRandomRatio = 1.0f;// SPRAY_PITCH
        float panL = 1.0f; 
        float panR = 1.0f;
    };

    // --- EL EJÉRCITO ---
    static constexpr int maxGrains = 128;
    std::array<Grain, maxGrains> grains;

    // --- TEMPORIZADOR PARA DISPARAR GRANOS (DENSITY) ---
    double samplesUntilNextGrain = 0.0;

    // --- VARIABLES GLOBALES DEL MOTOR ---
    bool isPlaying = false;
    double autoScanOffset = 0.0;
    float currentVelocity = 0.0f;
    float pitchRatio = 1.0f;

    // Aquí guardamos las llaves para usarlas luego en la cocina
    juce::AudioBuffer<float>* audioBuffer;
    juce::AudioProcessorValueTreeState* apvts;
};
