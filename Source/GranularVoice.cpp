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
    if (allowTailOff)
    {
        // NO cortamos el sonido. Le decimos al ADSR que inicie el Release.
        ampAdsr.noteOff();
    }
    else
    {
        // Solo cortamos de golpe si el programa anfitrión entra en pánico (Panic Stop)
        clearCurrentNote();
        ampAdsr.reset();
        isPlaying = false;
    }
}

void GranularVoice::pitchWheelMoved(int newPitchWheelValue) {}
void GranularVoice::controllerMoved(int controllerNumber, int newControllerValue) {}

void GranularVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    isPlaying = true;
    currentVelocity = velocity;
    pitchRatio = std::pow(2.0, (midiNoteNumber - 60) / 12.0);

    // Reset grains
    for (int i = 0; i < 128; ++i) {
        grains[i].isActive = false;
        visualGrainEnv[i].store(0.0f); // Apaga los fantasmas
    }

    samplesUntilNextGrain = 0.0;
    autoScanOffset = 0.0;


    // Reset Filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = getSampleRate();
    spec.maximumBlockSize = 1; // Procesamos muestra a muestra
    spec.numChannels = 1;      // Un filtro por canal

    for (int i = 0; i < 2; ++i)
    {
        // Low Pass
        lpf[i].prepare(spec);
        lpf[i].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lpf[i].reset();

        // High Pass
        hpf[i].prepare(spec);
        hpf[i].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        hpf[i].reset();
    }
    
    ampAdsr.setSampleRate(getSampleRate());
    ampAdsr.noteOn();
}

// --------------------------------------------------------------------------

void GranularVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isPlaying || audioBuffer->getNumSamples() == 0) return;

    //  LEER PARÁMETROS
    float positionKnob = apvts->getRawParameterValue("POSITION")->load();
    float sizeRatio = apvts->getRawParameterValue("GRAIN_SIZE")->load();
    float scanSpeed = apvts->getRawParameterValue("SCAN_SPEED")->load();
    float sprayPos = apvts->getRawParameterValue("SPRAY_POS")->load();
    float density = apvts->getRawParameterValue("DENSITY")->load();
    float shapeParam = apvts->getRawParameterValue("SHAPE")->load();
    float sprayPan = apvts->getRawParameterValue("SPRAY_PAN")->load();
    float sprayPitch = apvts->getRawParameterValue("SPRAY_PITCH")->load();
    float scanMode = apvts->getRawParameterValue("SCAN_MODE")->load();
    float filterLpfFreq = apvts->getRawParameterValue("FILTER_LPF")->load();
    float filterRes = apvts->getRawParameterValue("FILTER_RES")->load();
    float filterHpfFreq = apvts->getRawParameterValue("FILTER_HPF")->load();
    ampAdsrParams.attack = apvts->getRawParameterValue("AMP_A")->load();
    ampAdsrParams.decay = apvts->getRawParameterValue("AMP_D")->load();
    ampAdsrParams.sustain = apvts->getRawParameterValue("AMP_S")->load();
    ampAdsrParams.release = apvts->getRawParameterValue("AMP_R")->load();
    ampAdsr.setParameters(ampAdsrParams);

    //for (int i = 0; i < 2; ++i) {
        //lpf[i].setCutoffFrequency(filterLpfFreq);
        //lpf[i].setResonance(filterRes);
        //hpf[i].setCutoffFrequency(filterHpfFreq);
        //hpf[i].setResonance(filterRes);
    //}

    // ==========================================================
// --- INYECCIÓN TEMPORAL: LFO 1 + LFO 2 AL FILTRO LPF ---
// ==========================================================
// Calculamos cuántos Hz mueve cada LFO por separado
    float modFromLfo1 = currentLfo1Value * 2000.0f;
    float modFromLfo2 = (currentLfo2Value - 0.5f) * 4000.0f; // LFO2 es vectorial (0 a 1), lo centramos con -0.5

    // LA MAGIA DE LA MATRIZ: Sumamos el valor base del knob + LFO 1 + LFO 2
    float modulatedLpfFreq = filterLpfFreq + modFromLfo1 + modFromLfo2;

    // Evitamos que el filtro explote bajando de 20Hz o subiendo de 20kHz
    modulatedLpfFreq = juce::jlimit(20.0f, 20000.0f, modulatedLpfFreq);

    // Aplicamos el resultado final a los filtros estéreo
    for (int i = 0; i < 2; ++i) {
        lpf[i].setCutoffFrequency(modulatedLpfFreq); // <-- USAMOS LA FRECUENCIA MODULADA SUMADA
        lpf[i].setResonance(filterRes);
        hpf[i].setCutoffFrequency(filterHpfFreq);
        hpf[i].setResonance(filterRes);
    }

    // LEEr KNOBS DE PITCH
    float pitchTrans = apvts->getRawParameterValue("PITCH_TRANS")->load();
    float pitchFine = apvts->getRawParameterValue("PITCH_FINE")->load();
    float pitchScale = apvts->getRawParameterValue("PITCH_SCALE")->load();

    // TONO BASE
    float currentTrans = pitchTrans + pitchFine;
    float finalBasePitchRatio = pitchRatio * std::pow(2.0f, currentTrans / 12.0f);

    float totalAudioSeconds = audioBuffer->getNumSamples() / getSampleRate();
    float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * totalAudioSeconds);
    int grainLength = (int)(getSampleRate() * grainSizeSeconds);
    double samplesBetweenGrains = getSampleRate() / juce::jmax(0.1f, density);

    for (int s = 0; s < numSamples; ++s)
    {
        // --- 1.5 CÁLCULO DE POSICIÓN CON MODOS ---
        //autoScanOffset += (double)scanSpeed / getSampleRate();
        autoScanOffset += (double)scanSpeed / (getSampleRate() * totalAudioSeconds);
        float rawPos = positionKnob + (float)autoScanOffset;

        if (rawPos < 0.0f) rawPos = std::fmod(rawPos, 1.0f) + 1.0f;

        float currentTargetPos = 0.0f;

        if (scanMode < 0.5f) currentTargetPos = std::fmod(rawPos, 1.0f);
        else if (scanMode < 1.5f) currentTargetPos = 1.0f - std::fmod(rawPos, 1.0f);
        else {
            float triangle = std::abs(std::fmod(rawPos * 2.0f, 2.0f) - 1.0f);
            currentTargetPos = juce::jlimit(0.0f, 1.0f, triangle);
        }

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

                    // --- LÓGICA DE PITCH SPRAY CON ESCALAS  ---
                    float rawPitchRand = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * sprayPitch;
                    int scaleModeInt = (int)pitchScale;

                    if (scaleModeInt == 1) // Octavas
                    {
                        rawPitchRand = std::round(rawPitchRand / 12.0f) * 12.0f;
                    }
                    else if (scaleModeInt == 2) // Octavas y Quintas
                    {
                        int st = (int)std::round(rawPitchRand);
                        int oct = (st / 12) * 12;
                        int rem = std::abs(st % 12);

                        if (rem < 4) rem = 0;
                        else if (rem < 10) rem = 7;
                        else { rem = 0; oct += (st < 0 ? -12 : 12); }

                        rawPitchRand = oct + (st < 0 ? -rem : rem);
                    }
                    else if (scaleModeInt == 3) // Pentatónica Menor
                    {
                        int st = (int)std::round(rawPitchRand);
                        int oct = (st / 12) * 12;
                        int rem = std::abs(st % 12);
                        int q = 0;

                        if (rem <= 1) q = 0;
                        else if (rem <= 4) q = 3;
                        else if (rem <= 6) q = 5;
                        else if (rem <= 8) q = 7;
                        else q = 10;

                        rawPitchRand = oct + (st < 0 ? -q : q);
                    }

                    grain.pitchRandomRatio = std::pow(2.0f, rawPitchRand / 12.0f);
                    grain.activePitchRatio = finalBasePitchRatio * grain.pitchRandomRatio;

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
        int grainIndex = 0;

        for (auto& grain : grains)
        {
            if (grain.isActive)
            {
                activeCount++;
                float progress = (float)(grain.currentPosition / grainLength);

                float hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * progress));
                float square = (progress < 0.02f) ? progress / 0.02f : (progress > 0.98f ? (1.0f - progress) / 0.02f : 1.0f);
                float window = (hann * (1.0f - shapeParam)) + (square * shapeParam);

                // --- USAMOS EL NUEVO finalBasePitchRatio AQUÍ ---
                //int readPos = grain.startSample + (int)(grain.currentPosition * finalBasePitchRatio * grain.pitchRandomRatio);
                int readPos = grain.startSample + (int)(grain.currentPosition * grain.activePitchRatio);

                int safeReadPos = juce::jlimit(0, audioBuffer->getNumSamples() - 1, readPos);
                float normalizedPos = (float)safeReadPos / (float)audioBuffer->getNumSamples();

                visualGrainPos[grainIndex].store(normalizedPos);
                visualGrainEnv[grainIndex].store(window);

                if (readPos >= 0 && readPos < audioBuffer->getNumSamples())
                {
                    float sample = audioBuffer->getReadPointer(0)[readPos] * window;
                    totalL += sample * grain.panL;
                    totalR += sample * grain.panR;
                }

                grain.currentPosition += 1.0;
                if (grain.currentPosition >= grainLength) grain.isActive = false;
            }
            else
            {
                visualGrainEnv[grainIndex].store(0.0f);
            }

            grainIndex++;
        }

        // --- 4. FILTROS Y SALIDA ANALÓGICA ---

        // 1º Ajustamos el volumen general según el número de granos vivos
        if (activeCount > 0) {
            float gainScale = 1.0f / std::sqrt((float)activeCount);
            totalL *= gainScale;
            totalR *= gainScale;
        }

        // 2º Pasamos el sonido por los FILTROS
        // Al hacerlo aquí, el filtro actúa sobre el sonido dinámico puro
        totalL = hpf[0].processSample(0, totalL);
        totalR = hpf[1].processSample(0, totalR);

        totalL = lpf[0].processSample(0, totalL);
        totalR = lpf[1].processSample(0, totalR);

        // 3º AHORA SÍ: Aplicamos la saturación/limitador analógico (std::tanh)
        // Esto "abraza" cualquier pico de resonancia del filtro y lo convierte en 
        // calidez de cinta, asegurando que el volumen nunca se descontrole.
        totalL = std::tanh(totalL);
        totalR = std::tanh(totalR);

        // ENVOLVENTE ADSR AL VOLUMEN
        float currentAdsrVolume = ampAdsr.getNextSample();
        totalL *= currentAdsrVolume;
        totalR *= currentAdsrVolume;

        outputBuffer.addSample(0, startSample + s, totalL* currentVelocity);
        outputBuffer.addSample(1, startSample + s, totalR* currentVelocity);

        // Si el ADSR ha llegado a cero (el Release ha terminado), matamos la voz por fin
        if (!ampAdsr.isActive())
        {
            for (int i = 0; i < 128; ++i) {
                visualGrainEnv[i].store(0.0f);
            }
            clearCurrentNote();
            isPlaying = false;
        }
    }
}

