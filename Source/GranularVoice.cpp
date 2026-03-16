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


// 4. TECLA SOLTADA (Note Off)
void GranularVoice::stopNote(float velocity, bool allowTailOff)
{
    // Por ahora, el sonido se corta de golpe (más adelante le pondremos la 'R' de ADSR)
    isPlaying = false;
    clearCurrentNote();
}

void GranularVoice::pitchWheelMoved(int newPitchWheelValue) {}
void GranularVoice::controllerMoved(int controllerNumber, int newControllerValue) {}

void GranularVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    isPlaying = true;
    currentVelocity = velocity;
    pitchRatio = std::pow(2.0, (midiNoteNumber - 60) / 12.0);

    // 1. Apagamos a todos los soldados para empezar en limpio
    for (auto& grain : grains) {
        grain.isActive = false;
    }

    // 2. Reiniciamos temporizadores
    samplesUntilNextGrain = 0.0; // Que dispare un grano inmediatamente
    autoScanOffset = 0.0;
}

// --------------------------------------------------------------------------

void GranularVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isPlaying || audioBuffer->getNumSamples() == 0) return;

    // --- 1. LEER PARÁMETROS ---
    float positionKnob = apvts->getRawParameterValue("POSITION")->load();
    float sizeRatio = apvts->getRawParameterValue("GRAIN_SIZE")->load();
    float scanSpeed = apvts->getRawParameterValue("SCAN_SPEED")->load();
    float sprayPos = apvts->getRawParameterValue("SPRAY_POS")->load();
    float density = apvts->getRawParameterValue("DENSITY")->load();
    float shapeParam = apvts->getRawParameterValue("SHAPE")->load();
    float sprayPan = apvts->getRawParameterValue("SPRAY_PAN")->load();
    float sprayPitch = apvts->getRawParameterValue("SPRAY_PITCH")->load(); // <--- NUEVO

    float totalAudioSeconds = audioBuffer->getNumSamples() / getSampleRate();
    float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * totalAudioSeconds);
    int grainLength = (int)(getSampleRate() * grainSizeSeconds);
    double samplesBetweenGrains = getSampleRate() / juce::jmax(0.1f, density);

    for (int s = 0; s < numSamples; ++s)
    {
        autoScanOffset += (double)scanSpeed / getSampleRate();
        float currentTargetPos = juce::jlimit(0.0f, 1.0f, positionKnob + (float)autoScanOffset);

        // --- 2. SCHEDULER (NACIMIENTO DE GRANOS) ---
        samplesUntilNextGrain -= 1.0;
        if (samplesUntilNextGrain <= 0.0)
        {
            for (auto& grain : grains)
            {
                if (!grain.isActive)
                {
                    grain.isActive = true;
                    grain.currentPosition = 0.0;

                    // Spray de Posición
                    float randomOffset = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * sprayPos;
                    float finalPos = juce::jlimit(0.0f, 1.0f, currentTargetPos + randomOffset);
                    grain.startSample = (int)(finalPos * (audioBuffer->getNumSamples() - 1));

                    // --- LÓGICA DE PITCH SPRAY (NUEVO) ---
                    float pitchRand = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * sprayPitch;
                    grain.pitchRandomRatio = std::pow(2.0f, pitchRand / 12.0f);

                    // Spray de Pan (Estéreo)
                    float randomPan = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * sprayPan;
                    grain.panL = std::cos(juce::MathConstants<float>::pi * (randomPan + 1.0f) / 4.0f);
                    grain.panR = std::sin(juce::MathConstants<float>::pi * (randomPan + 1.0f) / 4.0f);

                    break;
                }
            }
            samplesUntilNextGrain += samplesBetweenGrains;
        }

        // --- 3. PROCESAMIENTO DE AUDIO ---
        float totalL = 0.0f;
        float totalR = 0.0f;
        int activeCount = 0;

        for (auto& grain : grains)
        {
            if (grain.isActive)
            {
                activeCount++;
                float progress = (float)(grain.currentPosition / grainLength);

                float hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * progress));
                float square = (progress < 0.02f) ? progress / 0.02f : (progress > 0.98f ? (1.0f - progress) / 0.02f : 1.0f);
                float window = (hann * (1.0f - shapeParam)) + (square * shapeParam);

                // --- POSICIÓN DE LECTURA (¡Aquí multiplicamos por el nuevo ratio!) ---
                int readPos = grain.startSample + (int)(grain.currentPosition * pitchRatio * grain.pitchRandomRatio); // <--- NUEVO

                if (readPos >= 0 && readPos < audioBuffer->getNumSamples())
                {
                    float sample = audioBuffer->getReadPointer(0)[readPos] * window;
                    totalL += sample * grain.panL;
                    totalR += sample * grain.panR;
                }

                grain.currentPosition += 1.0;
                if (grain.currentPosition >= grainLength) grain.isActive = false;
            }
        }

        // --- 4. SALIDA ---
        if (activeCount > 0) {
            float gainScale = 1.0f / std::sqrt((float)activeCount);
            totalL = std::tanh(totalL * gainScale);
            totalR = std::tanh(totalR * gainScale);
        }

        outputBuffer.addSample(0, startSample + s, totalL * currentVelocity);
        outputBuffer.addSample(1, startSample + s, totalR * currentVelocity);
    }
}

