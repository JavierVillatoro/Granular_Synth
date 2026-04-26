/*
  ==============================================================================
    GranularVoice.cpp
    Created: 15 Mar 2026 7:08:37pm
    Author:  franc
  ==============================================================================
*/

#include "GranularVoice.h"
#include "PluginProcessor.h"

GranularVoice::GranularVoice(juce::AudioBuffer<float>* buffer, juce::AudioProcessorValueTreeState* apvtsToUse, juce::String prefix)
{
    myBuffer = buffer;
    apvts = apvtsToUse;
    myPrefix = prefix;
}

bool GranularVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<GranularSound*>(sound) != nullptr;
}

void GranularVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff) {
        ampAdsr.noteOff();
    }
    else {
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

    for (int i = 0; i < 128; ++i) {
        grains[i].isActive = false;
        visualGrainEnv[i].store(0.0f);
    }

    samplesUntilNextGrain = 0.0;
    autoScanOffset = 0.0;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = getSampleRate();
    spec.maximumBlockSize = 1;
    spec.numChannels = 1;

    // PREPARACIÓN DE LOS FILTROS BÁSICOS Y FORMANTES
    for (int i = 0; i < 2; ++i) {
        lpf[i].prepare(spec);
        lpf[i].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lpf[i].reset();

        hpf[i].prepare(spec);
        hpf[i].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        hpf[i].reset();

        for (int m = 0; m < 4; ++m) {
            for (int f = 0; f < 3; ++f) {
                formantFilters[m][f][i].prepare(spec);
                formantFilters[m][f][i].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                formantFilters[m][f][i].reset();
            }
        }

        // 1. PREPARAR LA TIERRA (Crossover Pasa-Bajos a 200Hz fijo para los subs)
        subCrossoverFilter[i].prepare(spec);
        subCrossoverFilter[i].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        subCrossoverFilter[i].setCutoffFrequency(200.0f);
        subCrossoverFilter[i].reset();

        // 3. PREPARAR EL CIELO (Filtro Pasa-Altos para limpiar el Halo)
        haloHighpass[i].prepare(spec);
        haloHighpass[i].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        haloHighpass[i].reset();
    }

    // 2. PREPARAR LA HUMANIDAD (El Ensemble)
    juce::dsp::ProcessSpec stereoSpec = spec;
    stereoSpec.numChannels = 2; // El Chorus y el Delay necesitan 2 canales (L y R)
    ensembleFX.prepare(stereoSpec);
    ensembleFX.reset();

    // 3. Preparar la Línea de Delay del Halo (¡AHORA SÍ, EN ESTÉREO!)
    haloDelay.prepare(stereoSpec);
    haloDelay.reset();

    ampAdsr.setSampleRate(getSampleRate());
    ampAdsr.noteOn();
}

void GranularVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isPlaying || getSampleRate() <= 0.0) return;

    juce::AudioBuffer<float>* currentBuffer = nullptr;
    float winStart = 0.0f;
    float winLen = 1.0f;

    // EL FIX: Declaramos processor FUERA de un if para que esté disponible en toda la función
    auto* processor = dynamic_cast<Granular_SynthAudioProcessor*>(&apvts->processor);

    // Solo continuamos si hemos podido conectarnos al cerebro del plugin
    if (processor != nullptr)
    {
        if (myPrefix == "L1_") {
            if (processor->isUpdatingBufferL1.load()) return;
            currentBuffer = &processor->audioBufferL1; winStart = processor->windowStartRatioL1.load(); winLen = processor->windowLengthRatioL1.load();
        }
        else if (myPrefix == "L2_") {
            if (processor->isUpdatingBufferL2.load()) return;
            currentBuffer = &processor->audioBufferL2; winStart = processor->windowStartRatioL2.load(); winLen = processor->windowLengthRatioL2.load();
        }
        else if (myPrefix == "L3_") {
            if (processor->isUpdatingBufferL3.load()) return;
            currentBuffer = &processor->audioBufferL3; winStart = processor->windowStartRatioL3.load(); winLen = processor->windowLengthRatioL3.load();
        }
        else if (myPrefix == "L4_") {
            if (processor->isUpdatingBufferL4.load()) return;
            currentBuffer = &processor->audioBufferL4; winStart = processor->windowStartRatioL4.load(); winLen = processor->windowLengthRatioL4.load();
        }
    }

    if (currentBuffer == nullptr || currentBuffer->getNumSamples() == 0) return;

    // =========================================================================================
    // --- 1. LECTURA BASE DE PARÁMETROS (TODOS LOS MÓDULOS DE LA VOZ) ---
    // =========================================================================================

    // Granular
    float positionKnob = apvts->getRawParameterValue(myPrefix + "POSITION")->load();
    float sizeRatio = apvts->getRawParameterValue(myPrefix + "GRAIN_SIZE")->load();
    float scanSpeed = apvts->getRawParameterValue(myPrefix + "SCAN_SPEED")->load();
    float sprayPos = apvts->getRawParameterValue(myPrefix + "SPRAY_POS")->load();
    float density = apvts->getRawParameterValue(myPrefix + "DENSITY")->load();
    float shapeParam = apvts->getRawParameterValue(myPrefix + "SHAPE")->load();
    float sprayPan = apvts->getRawParameterValue(myPrefix + "SPRAY_PAN")->load();
    float sprayPitch = apvts->getRawParameterValue(myPrefix + "SPRAY_PITCH")->load();
    float scanMode = apvts->getRawParameterValue(myPrefix + "SCAN_MODE")->load();

    // Filtros Maestro
    float filterLpfFreq = apvts->getRawParameterValue(myPrefix + "FILTER_LPF")->load();
    float filterResLpf = apvts->getRawParameterValue(myPrefix + "FILTER_RES_LPF")->load();
    float filterHpfFreq = apvts->getRawParameterValue(myPrefix + "FILTER_HPF")->load();
    float filterResHpf = apvts->getRawParameterValue(myPrefix + "FILTER_RES_HPF")->load();

    // Pitch
    float pitchTrans = apvts->getRawParameterValue(myPrefix + "PITCH_TRANS")->load();
    float pitchFine = apvts->getRawParameterValue(myPrefix + "PITCH_FINE")->load();
    float pitchScale = apvts->getRawParameterValue(myPrefix + "PITCH_SCALE")->load();

    // Envolvente de Amplificador (AMP)
    float ampA = apvts->getRawParameterValue(myPrefix + "AMP_A")->load();
    float ampD = apvts->getRawParameterValue(myPrefix + "AMP_D")->load();
    float ampS = apvts->getRawParameterValue(myPrefix + "AMP_S")->load();
    float ampR = apvts->getRawParameterValue(myPrefix + "AMP_R")->load();

    // Efectos Internos (Ensemble y Halo)
    float ensRateVal = apvts->getRawParameterValue(myPrefix + "ENS_RATE")->load();
    float ensDepthVal = apvts->getRawParameterValue(myPrefix + "ENS_DEPTH")->load();
    float ensWidthVal = apvts->getRawParameterValue(myPrefix + "ENS_WIDTH")->load();
    float ensMixVal = apvts->getRawParameterValue(myPrefix + "ENS_MIX")->load();

    float haloPitchVal = apvts->getRawParameterValue(myPrefix + "HALO_PITCH")->load();
    float haloShimmerVal = apvts->getRawParameterValue(myPrefix + "HALO_SHIMMER")->load();
    float haloColorVal = apvts->getRawParameterValue(myPrefix + "HALO_COLOR")->load();
    float haloMixVal = apvts->getRawParameterValue(myPrefix + "HALO_MIX")->load();

    // EQ y Volumen
    float eqLow = apvts->getRawParameterValue(myPrefix + "EQ_LOW")->load();
    float eqMidL = apvts->getRawParameterValue(myPrefix + "EQ_MID_LOW")->load();
    float eqMidH = apvts->getRawParameterValue(myPrefix + "EQ_MID_HIGH")->load();
    float eqHigh = apvts->getRawParameterValue(myPrefix + "EQ_HIGH")->load();
    float mixVol = apvts->getRawParameterValue(myPrefix + "MIX_VOL")->load();

    // Formantes / Monjes (Leemos los 4 aquí arriba para poder modularlos)
    float mX[4], mY[4], monkMixes[4];
    for (int m = 0; m < 4; ++m) {
        juce::String vPref = myPrefix + "M" + juce::String(m + 1);
        mX[m] = apvts->getRawParameterValue(vPref + "_X")->load();
        mY[m] = apvts->getRawParameterValue(vPref + "_Y")->load();
        monkMixes[m] = apvts->getRawParameterValue(vPref + "_MIX")->load();
    }

    // =========================================================================================
    // --- 2. LA MATRIZ MAESTRA: APLICACIÓN DE MODULACIÓN TOTAL ---
    // =========================================================================================
    if (processor)
    {
        float vVel = currentVelocity;
        float vEnv2 = 0.0f; // (Lo conectaremos pronto)
        float vLfo1 = currentLfo1Value;
        float vLfo2 = currentLfo2Value;

        auto applyMod = [&](const juce::String& paramName, float& targetVar, float minVal, float maxVal, float scaleAmount)
            {
                float mod = processor->getMatrixModulation(myPrefix + paramName, vVel, vEnv2, vLfo1, vLfo2);
                if (mod != 0.0f) targetVar = juce::jlimit(minVal, maxVal, targetVar + (mod * scaleAmount));
            };

        // Granular Engine
        applyMod("POSITION", positionKnob, 0.0f, 1.0f, 0.5f);
        applyMod("GRAIN_SIZE", sizeRatio, 0.01f, 1.0f, 0.5f);
        applyMod("SCAN_SPEED", scanSpeed, -2.0f, 2.0f, 2.0f);
        applyMod("DENSITY", density, 1.0f, 120.0f, 60.0f);
        applyMod("SHAPE", shapeParam, 0.0f, 1.0f, 0.5f);
        applyMod("SPRAY_POS", sprayPos, 0.0f, 1.0f, 0.5f);
        applyMod("SPRAY_PAN", sprayPan, 0.0f, 1.0f, 0.5f);
        applyMod("SPRAY_PITCH", sprayPitch, 0.0f, 12.0f, 12.0f);

        // Pitch & Filter
        applyMod("PITCH_TRANS", pitchTrans, -24.0f, 24.0f, 24.0f);
        applyMod("PITCH_FINE", pitchFine, -1.0f, 1.0f, 1.0f);
        applyMod("FILTER_LPF", filterLpfFreq, 20.0f, 20000.0f, 8000.0f);
        applyMod("FILTER_HPF", filterHpfFreq, 20.0f, 20000.0f, 8000.0f);
        applyMod("FILTER_RES_LPF", filterResLpf, 0.7f, 2.5f, 1.0f);
        applyMod("FILTER_RES_HPF", filterResHpf, 0.7f, 2.5f, 1.0f);

        // Envolvente AMP
        applyMod("AMP_A", ampA, 0.01f, 5.0f, 2.0f);
        applyMod("AMP_D", ampD, 0.01f, 5.0f, 2.0f);
        applyMod("AMP_S", ampS, 0.0f, 1.0f, 0.5f);
        applyMod("AMP_R", ampR, 0.01f, 5.0f, 2.0f);

        // Halo & Ensemble
        applyMod("HALO_PITCH", haloPitchVal, 0.0f, 1.0f, 0.5f);
        applyMod("HALO_SHIMMER", haloShimmerVal, 0.0f, 1.0f, 0.5f);
        applyMod("HALO_COLOR", haloColorVal, 0.0f, 1.0f, 0.5f);
        applyMod("HALO_MIX", haloMixVal, 0.0f, 1.0f, 0.5f);

        applyMod("ENS_RATE", ensRateVal, 0.0f, 1.0f, 0.5f);
        applyMod("ENS_DEPTH", ensDepthVal, 0.0f, 1.0f, 0.5f);
        applyMod("ENS_WIDTH", ensWidthVal, 0.0f, 1.0f, 0.5f);
        applyMod("ENS_MIX", ensMixVal, 0.0f, 1.0f, 0.5f);

        // Mixer & EQ
        applyMod("MIX_VOL", mixVol, -60.0f, 6.0f, 12.0f);
        applyMod("EQ_LOW", eqLow, -60.0f, 15.0f, 15.0f);
        applyMod("EQ_MID_LOW", eqMidL, -60.0f, 15.0f, 15.0f);
        applyMod("EQ_MID_HIGH", eqMidH, -60.0f, 15.0f, 15.0f);
        applyMod("EQ_HIGH", eqHigh, -60.0f, 15.0f, 15.0f);

        // Formantes (Monjes)
        for (int m = 0; m < 4; ++m) {
            juce::String mStr = "M" + juce::String(m + 1);
            applyMod(mStr + "_X", mX[m], 0.0f, 1.0f, 0.5f);
            applyMod(mStr + "_Y", mY[m], 0.0f, 1.0f, 0.5f);
            applyMod(mStr + "_MIX", monkMixes[m], 0.0f, 1.0f, 0.5f);
        }
    }

    // =========================================================================================
    // --- 3. CONVERSIÓN FINAL POST-MATRIZ (Configurar Filtros y Efectos) ---
    // =========================================================================================

    for (int m = 0; m < 4; ++m) {
        if (monkMixes[m] > 0.001f) {
            float pos = mX[m] * 4.0f; int index = (int)pos; float frac = pos - index;
            const float f1_t[5] = { 730.0f, 530.0f, 270.0f, 400.0f, 320.0f };
            const float f2_t[5] = { 1090.0f, 1840.0f, 2290.0f, 840.0f, 800.0f };
            const float f3_t[5] = { 2440.0f, 2480.0f, 3010.0f, 2400.0f, 2250.0f };
            float f1 = (index >= 4) ? f1_t[4] : f1_t[index] + frac * (f1_t[index + 1] - f1_t[index]);
            float f2 = (index >= 4) ? f2_t[4] : f2_t[index] + frac * (f2_t[index + 1] - f2_t[index]);
            float f3 = (index >= 4) ? f3_t[4] : f3_t[index] + frac * (f3_t[index + 1] - f3_t[index]);

            float shiftMap = 0.5f + mY[m];
            f1 = juce::jlimit(20.0f, 20000.0f, f1 * shiftMap);
            f2 = juce::jlimit(20.0f, 20000.0f, f2 * shiftMap);
            f3 = juce::jlimit(20.0f, 20000.0f, f3 * shiftMap);
            float q = 1.0f + (mY[m] * 15.0f);

            for (int i = 0; i < 2; ++i) {
                formantFilters[m][0][i].setCutoffFrequency(f1); formantFilters[m][0][i].setResonance(q);
                formantFilters[m][1][i].setCutoffFrequency(f2); formantFilters[m][1][i].setResonance(q);
                formantFilters[m][2][i].setCutoffFrequency(f3); formantFilters[m][2][i].setResonance(q);
            }
        }
    }

    ensembleFX.setRate(0.1f + (ensRateVal * 3.9f));
    ensembleFX.setDepth(ensDepthVal);
    ensembleFX.setMix(1.0f);

    for (int i = 0; i < 2; ++i) {
        haloHighpass[i].setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, 500.0f + (haloColorVal * 5000.0f)));
    }

    ampAdsrParams.attack = ampA; ampAdsrParams.decay = ampD;
    ampAdsrParams.sustain = ampS; ampAdsrParams.release = ampR;
    ampAdsr.setParameters(ampAdsrParams);

    for (int i = 0; i < 2; ++i) {
        lpf[i].setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, filterLpfFreq)); lpf[i].setResonance(filterResLpf);
        hpf[i].setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, filterHpfFreq)); hpf[i].setResonance(filterResHpf);
    }

    float finalBasePitchRatio = pitchRatio * std::pow(2.0f, (pitchTrans + pitchFine) / 12.0f);

    float activeAudioSeconds = (currentBuffer->getNumSamples() / getSampleRate()) * winLen;
    float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * activeAudioSeconds);
    int grainLength = (int)(getSampleRate() * grainSizeSeconds);
    double samplesBetweenGrains = getSampleRate() / juce::jmax(0.1f, density);

    auto sr = getSampleRate();
    for (int i = 0; i < 2; ++i) {
        eqLowFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 100.0f, 0.7f, juce::Decibels::decibelsToGain(eqLow));
        eqMidLowFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, 500.0f, 0.7f, juce::Decibels::decibelsToGain(eqMidL));
        eqMidHighFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, 2500.0f, 0.7f, juce::Decibels::decibelsToGain(eqMidH));
        eqHighFilter[i].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 10000.0f, 0.7f, juce::Decibels::decibelsToGain(eqHigh));
    }

    float mixerVolGain = juce::Decibels::decibelsToGain(mixVol);

    // =======================================================================
    // BUCLE DE AUDIO
    // =======================================================================
    for (int s = 0; s < numSamples; ++s)
    {
        autoScanOffset += (double)scanSpeed / (double)currentBuffer->getNumSamples();
        float rawPos = positionKnob + (float)autoScanOffset;
        if (rawPos < 0.0f) rawPos = std::fmod(rawPos, 1.0f) + 1.0f;

        float currentTargetPos = 0.0f;
        if (scanMode < 0.5f) currentTargetPos = std::fmod(rawPos, 1.0f);
        else if (scanMode < 1.5f) currentTargetPos = 1.0f - std::fmod(rawPos, 1.0f);
        else currentTargetPos = juce::jlimit(0.0f, 1.0f, std::abs(std::fmod(rawPos * 2.0f, 2.0f) - 1.0f));

        samplesUntilNextGrain -= 1.0;
        if (samplesUntilNextGrain <= 0.0) {
            for (auto& grain : grains) {
                if (!grain.isActive) {
                    grain.isActive = true; grain.currentPosition = 0.0;
                    float finalPos = juce::jlimit(0.0f, 1.0f, winStart + (juce::jlimit(0.0f, 1.0f, currentTargetPos + ((juce::Random::getSystemRandom().nextFloat() - 0.5f) * sprayPos)) * winLen));
                    grain.startSample = (int)(finalPos * (currentBuffer->getNumSamples() - 1));

                    //float rawPitchRand = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * sprayPitch;
                    //if ((int)pitchScale == 1) rawPitchRand = std::round(rawPitchRand / 12.0f) * 12.0f;

                    //grain.activePitchRatio = finalBasePitchRatio * std::pow(2.0f, rawPitchRand / 12.0f);

                    float rawPitchRand = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * sprayPitch;

                    // --- CORRECCIÓN DEL BUG DE ESCALA ---
                    int scaleMode = juce::roundToInt(pitchScale); // Redondeo seguro, adiós a los decimales trampa

                    if (scaleMode == 1) {
                        // Modo 1: Octavas (Solo salta de 12 en 12)
                        rawPitchRand = std::round(rawPitchRand / 12.0f) * 12.0f;
                    }
                    else if (scaleMode == 2) {
                        // Modo 2: Quintas (Solo salta de 7 en 7 semitonos)
                        rawPitchRand = std::round(rawPitchRand / 7.0f) * 7.0f;
                    }
                    else if (scaleMode == 3) {
                        // Modo 3: Semitonos (Cromático exacto)
                        rawPitchRand = std::round(rawPitchRand);
                    }
                    // Si scaleMode es 0 (Libre), se queda como float continuo sin cuantizar.

                    grain.activePitchRatio = finalBasePitchRatio * std::pow(2.0f, rawPitchRand / 12.0f);
                    float randomPan = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * sprayPan;
                    grain.panL = std::cos(juce::MathConstants<float>::pi * (randomPan + 1.0f) / 4.0f);
                    grain.panR = std::sin(juce::MathConstants<float>::pi * (randomPan + 1.0f) / 4.0f);
                    break;
                }
            }
            samplesUntilNextGrain += samplesBetweenGrains;
        }

        float totalL = 0.0f; float totalR = 0.0f; int activeCount = 0; int grainIndex = 0;
        for (auto& grain : grains) {
            if (grain.isActive) {
                activeCount++;
                float progress = (float)(grain.currentPosition / grainLength);
                float hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * progress));
                float square = (progress < 0.005f) ? progress / 0.005f : (progress > 0.995f ? (1.0f - progress) / 0.005f : 1.0f);
                float window = (hann * (1.0f - shapeParam)) + (square * shapeParam);
                int readPos = juce::jlimit(0, currentBuffer->getNumSamples() - 1, grain.startSample + (int)(grain.currentPosition * grain.activePitchRatio));

                visualGrainPos[grainIndex].store((float)readPos / (float)currentBuffer->getNumSamples());
                visualGrainEnv[grainIndex].store(window);

                float sample = currentBuffer->getReadPointer(0)[readPos] * window;
                totalL += sample * grain.panL; totalR += sample * grain.panR;

                grain.currentPosition += 1.0;
                if (grain.currentPosition >= grainLength) grain.isActive = false;
            }
            else { visualGrainEnv[grainIndex].store(0.0f); }
            grainIndex++;
        }

        if (activeCount > 0) {
            float gainScale = 1.0f / std::sqrt((float)activeCount);
            totalL *= gainScale; totalR *= gainScale;
        }

        // =========================================================
        // 1. CROSSOVER PERFECTO (Cero picos de volumen, cero pérdida de graves)
        // =========================================================
        float rawL = totalL;
        float rawR = totalR;

        // A) Extraemos los graves puros (Sub)
        float subL = subCrossoverFilter[0].processSample(0, rawL);
        float subR = subCrossoverFilter[1].processSample(0, rawR);

        // B) Extraemos Medios/Agudos restando el Sub (Fase perfecta)
        float highL = rawL - subL;
        float highR = rawR - subR;

        // =========================================================
        // 2. LOS MONJES (Procesan SOLO los medios/agudos)
        // =========================================================
        float monksWetL = 0.0f; float monksWetR = 0.0f; float totalMonkMix = 0.0f;
        for (int m = 0; m < 4; ++m) {
            if (monkMixes[m] > 0.001f) {
                // Filtramos SOLO la parte alta de la señal (highL / highR)
                float mL = formantFilters[m][0][0].processSample(0, highL) + formantFilters[m][1][0].processSample(0, highL) + formantFilters[m][2][0].processSample(0, highL);
                float mR = formantFilters[m][0][1].processSample(0, highR) + formantFilters[m][1][1].processSample(0, highR) + formantFilters[m][2][1].processSample(0, highR);

                mL *= 1.2f; mR *= 1.2f; // Compensación suave
                monksWetL += mL * monkMixes[m];
                monksWetR += mR * monkMixes[m];
                totalMonkMix += monkMixes[m];
            }
        }

        // Crossfade Constante: Bajan los medios limpios, suben los monjes (Volumen idéntico)
        float mixParam = juce::jlimit(0.0f, 1.0f, totalMonkMix);
        if (totalMonkMix > 1.0f) { monksWetL /= totalMonkMix; monksWetR /= totalMonkMix; }

        float dryGain = std::cos(mixParam * juce::MathConstants<float>::halfPi);
        float wetGain = std::sin(mixParam * juce::MathConstants<float>::halfPi);

        float mixedHighsL = (highL * dryGain) + (monksWetL * wetGain);
        float mixedHighsR = (highR * dryGain) + (monksWetR * wetGain);

        // Sumamos Sub intacto + Medios procesados = Señal Central Perfecta
        float coreSoundL = subL + mixedHighsL;
        float coreSoundR = subR + mixedHighsR;

        // =========================================================
        // 3. ENSEMBLE INDEPENDIENTE
        // =========================================================
        float ensWetL = 0.0f; float ensWetR = 0.0f;
        if (ensMixVal > 0.001f) {
            ensWetL = coreSoundL; ensWetR = coreSoundR;
            float* chorusChannels[] = { &ensWetL, &ensWetR };
            juce::dsp::AudioBlock<float> chorusBlock(chorusChannels, 2, 1);
            juce::dsp::ProcessContextReplacing<float> chorusContext(chorusBlock);
            ensembleFX.process(chorusContext);
        }

        // =========================================================
        // 4. HALO SHIMMER (Dual-Delay Pitch Shifter Real +1 Octava)
        // =========================================================
        float haloL = 0.0f; float haloR = 0.0f;
        if (haloMixVal > 0.001f) {
            // Ventana granular de 80ms para el Shimmer
            float windowSamples = (80.0f / 1000.0f) * sr;

            // El reloj avanza 1 muestra por muestra (Velocidad x2 = +1 Octava)
            haloPhasor += 1.0f / windowSamples;
            if (haloPhasor >= 1.0f) haloPhasor -= 1.0f;

            // Fase desdoblada 180 grados
            float phase1 = haloPhasor;
            float phase2 = phase1 + 0.5f;
            if (phase2 >= 1.0f) phase2 -= 1.0f;

            // Las cabezas leen hacia el presente
            float delay1 = (1.0f - phase1) * windowSamples;
            float delay2 = (1.0f - phase2) * windowSamples;

            // Curvas de Crossfade (Sine)
            float gain1 = std::sin(phase1 * juce::MathConstants<float>::pi);
            float gain2 = std::sin(phase2 * juce::MathConstants<float>::pi);

            // Se alimenta de todo a la vez (Señal cruda + Monjes)
            float haloInputL = coreSoundL;
            float haloInputR = coreSoundR;

            // Lectura de los dos cabezales y suma (Magia de la Octava)
            float shimmerL = (haloDelay.popSample(0, delay1) * gain1) + (haloDelay.popSample(0, delay2) * gain2);
            float shimmerR = (haloDelay.popSample(1, delay1) * gain1) + (haloDelay.popSample(1, delay2) * gain2);

            // Filtro de Color para quitar dureza
            shimmerL = haloHighpass[0].processSample(0, shimmerL);
            shimmerR = haloHighpass[1].processSample(0, shimmerR);

            // Inyectar el sonido input + feedback a la línea de delay infinita
            haloDelay.pushSample(0, haloInputL + (shimmerL * haloShimmerVal));
            haloDelay.pushSample(1, haloInputR + (shimmerR * haloShimmerVal));

            haloL = shimmerL;
            haloR = shimmerR;
        }

        // =========================================================
        // 5. SUMADOR MAESTRO Y FILTRO
        // =========================================================
        totalL = coreSoundL + (ensWetL * ensMixVal) + (haloL * haloMixVal);
        totalR = coreSoundR + (ensWetR * ensMixVal) + (haloR * haloMixVal);

        // Filtro LPF/HPF Maestro (se lo traga todo)
        totalL = hpf[0].processSample(0, totalL); totalR = hpf[1].processSample(0, totalR);
        totalL = lpf[0].processSample(0, totalL); totalR = lpf[1].processSample(0, totalR);

        // EQ y Envolvente (AMP)
        totalL = eqLowFilter[0].processSample(totalL); totalR = eqLowFilter[1].processSample(totalR);
        totalL = eqMidLowFilter[0].processSample(totalL); totalR = eqMidLowFilter[1].processSample(totalR);
        totalL = eqMidHighFilter[0].processSample(totalL); totalR = eqMidHighFilter[1].processSample(totalR);
        totalL = eqHighFilter[0].processSample(totalL); totalR = eqHighFilter[1].processSample(totalR);

        float currentAdsrVolume = ampAdsr.getNextSample() * mixerVolGain;
        totalL *= currentAdsrVolume; totalR *= currentAdsrVolume;

        if (std::isnan(totalL) || std::isinf(totalL)) totalL = 0.0f;
        if (std::isnan(totalR) || std::isinf(totalR)) totalR = 0.0f;

        outputBuffer.addSample(0, startSample + s, totalL * currentVelocity);
        outputBuffer.addSample(1, startSample + s, totalR * currentVelocity);

        if (!ampAdsr.isActive()) {
            for (int i = 0; i < 128; ++i) visualGrainEnv[i].store(0.0f);
            clearCurrentNote(); isPlaying = false;
        }
    }
}

