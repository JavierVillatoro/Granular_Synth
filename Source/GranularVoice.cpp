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
    float grainSizeSeconds = apvts->getRawParameterValue("GRAIN_SIZE")->load();
    float scanSpeed = apvts->getRawParameterValue("SCAN_SPEED")->load();

    int grainLength = (int)(getSampleRate() * grainSizeSeconds);
    int overlapOffset = grainLength / 2; // El segundo grano nace a mitad del primero

    // 2. PROCESAR CADA SAMPLE
    for (int s = 0; s < numSamples; ++s)
    {
        // El cursor "fantasma" avanza siempre según el Scan Speed
        autoScanOffset += (double)scanSpeed / getSampleRate();
        float currentTargetPos = positionKnob + (float)autoScanOffset;

        // Loop del cursor (0.0 a 1.0)
        while (currentTargetPos > 1.0f) currentTargetPos -= 1.0f;
        while (currentTargetPos < 0.0f) currentTargetPos += 1.0f;

        float totalSampleSum = 0.0f;

        // --- LÓGICA GRANO 1 ---
        if (currentReadPosition1 == 0.0) // ¡Foto del punto de inicio al nacer!
            grainStartSample1 = (int)(currentTargetPos * (audioBuffer->getNumSamples() - 1));

        // Envolvente de Hann: $$w(n) = 0.5 \left(1 - \cos\left(\frac{2\pi n}{N-1}\right)\right)$$
        float win1 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (currentReadPosition1 / grainLength)));
        int read1 = grainStartSample1 + (int)(currentReadPosition1 * pitchRatio);

        if (read1 >= 0 && read1 < audioBuffer->getNumSamples())
            totalSampleSum += audioBuffer->getReadPointer(0)[read1] * win1;

        // --- LÓGICA GRANO 2 (Solo si el grano 1 ya ha recorrido la mitad) ---
        if (!secondGrainActive && currentReadPosition1 >= overlapOffset) {
            secondGrainActive = true;
            currentReadPosition2 = 0.0;
        }

        if (secondGrainActive) {
            if (currentReadPosition2 == 0.0) // Foto del punto de inicio para el segundo grano
                grainStartSample2 = (int)(currentTargetPos * (audioBuffer->getNumSamples() - 1));

            float win2 = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * (currentReadPosition2 / grainLength)));
            int read2 = grainStartSample2 + (int)(currentReadPosition2 * pitchRatio);

            if (read2 >= 0 && read2 < audioBuffer->getNumSamples())
                totalSampleSum += audioBuffer->getReadPointer(0)[read2] * win2;
        }

        // --- ENVIAR A SALIDA ---
        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            outputBuffer.addSample(ch, startSample + s, totalSampleSum * currentVelocity);

        // --- ACTUALIZAR POSICIONES ---
        currentReadPosition1 += 1.0;
        if (secondGrainActive) currentReadPosition2 += 1.0;

        // Reinicio de los granos al terminar su vida
        if (currentReadPosition1 >= grainLength) currentReadPosition1 = 0.0;
        if (currentReadPosition2 >= grainLength) currentReadPosition2 = 0.0;
    }
}
