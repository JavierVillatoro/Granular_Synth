/*
  ==============================================================================

    GranularVoice.cpp
    Created: 15 Mar 2026 7:08:37pm
    Author:  franc

  ==============================================================================
*/

#include "GranularVoice.h"
#include "PluginProcessor.h"

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

    //  LEER PARÁMETROS GLOBALES
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
    float filterResLpf = apvts->getRawParameterValue("FILTER_RES_LPF")->load();
    float filterHpfFreq = apvts->getRawParameterValue("FILTER_HPF")->load();
    float filterResHpf = apvts->getRawParameterValue("FILTER_RES_HPF")->load();
    ampAdsrParams.attack = apvts->getRawParameterValue("AMP_A")->load();
    ampAdsrParams.decay = apvts->getRawParameterValue("AMP_D")->load();
    ampAdsrParams.sustain = apvts->getRawParameterValue("AMP_S")->load();
    ampAdsrParams.release = apvts->getRawParameterValue("AMP_R")->load();
    ampAdsr.setParameters(ampAdsrParams);

    // ==========================================================
    // --- INYECCIÓN TEMPORAL: LFO 1 + LFO 2 AL FILTRO LPF ---
    // ==========================================================
    float modFromLfo1 = currentLfo1Value * 2000.0f;
    float modFromLfo2 = (currentLfo2Value - 0.5f) * 4000.0f;
    float modulatedLpfFreq = filterLpfFreq + modFromLfo1 + modFromLfo2;
    modulatedLpfFreq = juce::jlimit(20.0f, 20000.0f, modulatedLpfFreq);

    for (int i = 0; i < 2; ++i) {
        lpf[i].setCutoffFrequency(modulatedLpfFreq);
        lpf[i].setResonance(filterResLpf);
        hpf[i].setCutoffFrequency(filterHpfFreq);
        hpf[i].setResonance(filterResHpf);
    }

    // LEER KNOBS DE PITCH
    float pitchTrans = apvts->getRawParameterValue("PITCH_TRANS")->load();
    float pitchFine = apvts->getRawParameterValue("PITCH_FINE")->load();
    float pitchScale = apvts->getRawParameterValue("PITCH_SCALE")->load();

    float currentTrans = pitchTrans + pitchFine;
    float finalBasePitchRatio = pitchRatio * std::pow(2.0f, currentTrans / 12.0f);

    float totalAudioSeconds = audioBuffer->getNumSamples() / getSampleRate();

    // LEER LA VENTANA DE ZOOM Y AJUSTAR EL TAMAÑO ---
    float winStart = 0.0f;
    float winLen = 1.0f;
    if (auto* processor = dynamic_cast<Granular_SynthAudioProcessor*>(&apvts->processor)) {
        winStart = processor->windowStartRatio.load();
        winLen = processor->windowLengthRatio.load();
    }
    float activeAudioSeconds = totalAudioSeconds * winLen;
    float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * activeAudioSeconds);

    int grainLength = (int)(getSampleRate() * grainSizeSeconds);
    double samplesBetweenGrains = getSampleRate() / juce::jmax(0.1f, density);


    // 🟢 MOVIDO AQUÍ AFUERA (ZONA SEGURA / CONTROL RATE) 🟢
    // ==============================================================================
    // Este bloque antes estaba metido en el bucle s. Al sacarlo aquí, la CPU
    // solo hace estas matemáticas pesadas 1 vez por bloque, no 44100 veces por segundo.
    // ==============================================================================
    float eqLowGain = apvts->getRawParameterValue("L1_EQ_LOW")->load();
    float eqMidLowGain = apvts->getRawParameterValue("L1_EQ_MID_LOW")->load();
    float eqMidHighGain = apvts->getRawParameterValue("L1_EQ_MID_HIGH")->load();
    float eqHighGain = apvts->getRawParameterValue("L1_EQ_HIGH")->load();
    float mixerVolDb = apvts->getRawParameterValue("L1_MIX_VOL")->load();

    auto sr = getSampleRate();

    for (int i = 0; i < 2; ++i) {
        eqLowFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 100.0f, 0.7f, juce::Decibels::decibelsToGain(eqLowGain));
        eqMidLowFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, 500.0f, 0.7f, juce::Decibels::decibelsToGain(eqMidLowGain));
        eqMidHighFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, 2500.0f, 0.7f, juce::Decibels::decibelsToGain(eqMidHighGain));
        eqHighFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 10000.0f, 0.7f, juce::Decibels::decibelsToGain(eqHighGain));
    }

    float mixerVolGain = juce::Decibels::decibelsToGain(mixerVolDb);
    // 🟢 ======================================================================= 🟢


    // ZONA ROJA (AUDIO RATE - 44100 Hz)  
    for (int s = 0; s < numSamples; ++s)
    {
        // --- 1.5 CÁLCULO DE POSICIÓN CON MODOS ---
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

                    // Spray de Posición (actúa sobre el porcentaje local)
                    float randomOffset = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * sprayPos;
                    float localPos = juce::jlimit(0.0f, 1.0f, currentTargetPos + randomOffset);

                    // Magia CROP
                    float finalPos = winStart + (localPos * winLen);
                    finalPos = juce::jlimit(0.0f, 1.0f, finalPos);

                    grain.startSample = (int)(finalPos * (audioBuffer->getNumSamples() - 1));

                    // Lógica de Pitch Spray
                    float rawPitchRand = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * sprayPitch;
                    int scaleModeInt = (int)pitchScale;

                    if (scaleModeInt == 1) {
                        rawPitchRand = std::round(rawPitchRand / 12.0f) * 12.0f;
                    }
                    else if (scaleModeInt == 2) {
                        int st = (int)std::round(rawPitchRand);
                        int oct = (st / 12) * 12;
                        int rem = std::abs(st % 12);
                        if (rem < 4) rem = 0;
                        else if (rem < 10) rem = 7;
                        else { rem = 0; oct += (st < 0 ? -12 : 12); }
                        rawPitchRand = oct + (st < 0 ? -rem : rem);
                    }
                    else if (scaleModeInt == 3) {
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
                float square = (progress < 0.005f) ? progress / 0.005f : (progress > 0.995f ? (1.0f - progress) / 0.005f : 1.0f);
                float window = (hann * (1.0f - shapeParam)) + (square * shapeParam);

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
        if (activeCount > 0) {
            float gainScale = 1.0f / std::sqrt((float)activeCount);
            totalL *= gainScale;
            totalR *= gainScale;
        }

        // Filtros globales (LPF y HPF)
        totalL = hpf[0].processSample(0, totalL);
        totalR = hpf[1].processSample(0, totalR);

        totalL = lpf[0].processSample(0, totalL);
        totalR = lpf[1].processSample(0, totalR);

        // Saturación analógica suave
        totalL = std::tanh(totalL);
        totalR = std::tanh(totalR);

        // 🟢 SE MANTIENE AQUÍ DENTRO: Pasar el audio por los filtros ya calculados
        totalL = eqLowFilter[0].processSample(totalL);
        totalR = eqLowFilter[1].processSample(totalR);

        totalL = eqMidLowFilter[0].processSample(totalL);
        totalR = eqMidLowFilter[1].processSample(totalR);

        totalL = eqMidHighFilter[0].processSample(totalL);
        totalR = eqMidHighFilter[1].processSample(totalR);

        totalL = eqHighFilter[0].processSample(totalL);
        totalR = eqHighFilter[1].processSample(totalR);

        // 🟢 SE MANTIENE AQUÍ DENTRO: Multiplicar el volumen ya calculado
        totalL *= mixerVolGain;
        totalR *= mixerVolGain;

        // ENVOLVENTE ADSR GENERAL
        float currentAdsrVolume = ampAdsr.getNextSample();
        totalL *= currentAdsrVolume;
        totalR *= currentAdsrVolume;

        // Salida al buffer de Ableton
        outputBuffer.addSample(0, startSample + s, totalL * currentVelocity);
        outputBuffer.addSample(1, startSample + s, totalR * currentVelocity);

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

