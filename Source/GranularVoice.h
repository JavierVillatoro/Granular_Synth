/*
  ==============================================================================

    GranularVoice.h
    Created: 15 Mar 2026 7:08:37pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

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
    // --- Posiciones de lectura (avanzan de 0 a grainLength) ---
    double currentReadPosition1 = 0.0;
    double currentReadPosition2 = 0.0;

    // --- "Fotos" de la posición en el archivo (para evitar chirridos) ---
    int grainStartSample1 = 0;
    int grainStartSample2 = 0;

    // --- Control del motor ---
    bool secondGrainActive = false;
    //double autoScanOffset = 0.0;
    // Aquí guardamos las llaves para usarlas luego en la cocina
    juce::AudioBuffer<float>* audioBuffer;
    juce::AudioProcessorValueTreeState* apvts;

    bool isPlaying = false;
    float currentVelocity = 0.0f;
    double currentReadPosition = 0.0; 
    double pitchRatio = 1.0;
    double autoScanOffset = 0.0;
};
