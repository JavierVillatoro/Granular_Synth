/*
  ==============================================================================

    GranularVoice.cpp
    Created: 15 Mar 2026 7:08:37pm
    Author:  franc

  ==============================================================================
*/

#include "GranularVoice.h"

// 1. EL CONSTRUCTOR: Guardamos las llaves
GranularVoice::GranularVoice(juce::AudioBuffer<float>* buffer, juce::AudioProcessorValueTreeState* apvtsToUse)
{
    audioBuffer = buffer;
    apvts = apvtsToUse;
}

// 2. ¿PODEMOS TOCAR ESTO? Sí, aceptamos todo.
bool GranularVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<GranularSound*>(sound) != nullptr;
}

// 3. TECLA PULSADA (Note On)
void GranularVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    isPlaying = true;
    currentVelocity = velocity;

    // 1. Cálculo del tono (Pitch)
    // std::pow requiere incluir <cmath>, pero JUCE ya lo incluye de serie.
    pitchRatio = std::pow(2.0, (midiNoteNumber - 60) / 12.0);

    // 2. RESET DEL MOTOR DE DOBLE GRANO
    // Sustituimos 'currentReadPosition = 0.0' por estas nuevas:
    currentReadPosition1 = 0.0;
    currentReadPosition2 = 0.0;

    // El segundo grano debe nacer desactivado (esperará al relevo del primero)
    secondGrainActive = false;

    // El escaneo automático siempre empieza desde donde diga el knob 'Position'
    autoScanOffset = 0.0;
}

// 4. TECLA SOLTADA (Note Off)
void GranularVoice::stopNote(float velocity, bool allowTailOff)
{
    // Por ahora, el sonido se corta de golpe (más adelante le pondremos la 'R' de ADSR)
    isPlaying = false;
    clearCurrentNote();
}

void GranularVoice::pitchWheelMoved(int newPitchWheelValue) {}
void GranularVoice::controllerMoved(int controllerNumber, int newControllerValue) {}

// 5. LAS MATEMÁTICAS DEL AUDIO
void GranularVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isPlaying || audioBuffer->getNumSamples() == 0) return;

    // 1. LEER PARÁMETROS
    float positionKnob = apvts->getRawParameterValue("POSITION")->load();

    // --- NUEVO: TAMAÑO DE GRANO PROPORCIONAL ---
    float sizeRatio = apvts->getRawParameterValue("GRAIN_SIZE")->load();
    float totalAudioSeconds = audioBuffer->getNumSamples() / getSampleRate();
    float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * totalAudioSeconds);
    // -------------------------------------------

    float scanSpeed = apvts->getRawParameterValue("SCAN_SPEED")->load();
    float sprayPos = apvts->getRawParameterValue("SPRAY_POS")->load();

    // Calculamos la longitud en samples usando los nuevos segundos calculados
    int grainLength = (int)(getSampleRate() * grainSizeSeconds);
    int overlapOffset = grainLength / 2;

    for (int s = 0; s < numSamples; ++s)
    {
        autoScanOffset += (double)scanSpeed / getSampleRate();
        float currentTargetPos = positionKnob + (float)autoScanOffset;

        while (currentTargetPos > 1.0f) currentTargetPos -= 1.0f;
        while (currentTargetPos < 0.0f) currentTargetPos += 1.0f;

        float totalSampleSum = 0.0f;

        // --- LÓGICA GRANO 1 ---
        if (currentReadPosition1 == 0.0)
        {
            float randomOffset = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * sprayPos;
            float finalPos1 = juce::jlimit(0.0f, 1.0f, currentTargetPos + randomOffset);
            grainStartSample1 = (int)(finalPos1 * (audioBuffer->getNumSamples() - 1));
        }

        float win1 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (currentReadPosition1 / grainLength)));
        int read1 = grainStartSample1 + (int)(currentReadPosition1 * pitchRatio);
        if (read1 >= 0 && read1 < audioBuffer->getNumSamples())
            totalSampleSum += audioBuffer->getReadPointer(0)[read1] * win1;

        // --- LÓGICA GRANO 2 ---
        if (!secondGrainActive && currentReadPosition1 >= overlapOffset) {
            secondGrainActive = true;
            currentReadPosition2 = 0.0;
        }

        if (secondGrainActive)
        {
            if (currentReadPosition2 == 0.0)
            {
                float randomOffset = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * sprayPos;
                float finalPos2 = juce::jlimit(0.0f, 1.0f, currentTargetPos + randomOffset);
                grainStartSample2 = (int)(finalPos2 * (audioBuffer->getNumSamples() - 1));
            }

            float win2 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (currentReadPosition2 / grainLength)));
            int read2 = grainStartSample2 + (int)(currentReadPosition2 * pitchRatio);
            if (read2 >= 0 && read2 < audioBuffer->getNumSamples())
                totalSampleSum += audioBuffer->getReadPointer(0)[read2] * win2;
        }

        // --- ENVIAR A SALIDA Y ACTUALIZAR ---
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + s, totalSampleSum * currentVelocity);

        currentReadPosition1 += 1.0;
        if (secondGrainActive) currentReadPosition2 += 1.0;

        if (currentReadPosition1 >= grainLength) currentReadPosition1 = 0.0;
        if (currentReadPosition2 >= grainLength) currentReadPosition2 = 0.0;
    }
}

