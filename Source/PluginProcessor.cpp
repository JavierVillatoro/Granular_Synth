/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Granular_SynthAudioProcessor::Granular_SynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
    formatManager.registerBasicFormats();

    // --- INICIALIZAR EL SINTETIZADOR MIDI ---

    // 1. Sound , permite tocar cualquier nota 
    synthL1.addSound(new GranularSound());
    synthL2.addSound(new GranularSound());
    synthL3.addSound(new GranularSound());
    synthL4.addSound(new GranularSound());


    // 2. Voces
    for (int i = 0; i < 8; ++i) {
        synthL1.addVoice(new GranularVoice(&audioBufferL1, &apvts, "L1_"));
    }
    for (int i = 0; i < 8; ++i) {
        synthL2.addVoice(new GranularVoice(&audioBufferL2, &apvts, "L2_"));
    }
    for (int i = 0; i < 8; ++i) {
        synthL3.addVoice(new GranularVoice(&audioBufferL3, &apvts, "L3_"));
    }
    for (int i = 0; i < 8; ++i) {
        synthL4.addVoice(new GranularVoice(&audioBufferL4, &apvts, "L4_"));
    }

    tcpReceiver = std::make_unique<TcpReceiver>(*this);

    apvts.addParameterListener("L1_REC", this);
    apvts.addParameterListener("L2_REC", this);
    apvts.addParameterListener("L3_REC", this);
    apvts.addParameterListener("L4_REC", this);

    // --- NUEVO: SERVIDOR OSC (Control en Tiempo Real) ---
    if (connect(9000))
    {
        // Le decimos a JUCE que escuche TODOS los mensajes OSC que lleguen.
        // (La magia de separarlos la hacemos abajo en oscMessageReceived)
        juce::OSCReceiver::addListener(this);

        DBG("Servidor OSC Iniciado en el puerto 9000");
    }

    //apvts.state = juce::ValueTree("GRANULAR_SYNTH_STATE");
    for (auto* param : getParameters()) {
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
            apvts.addParameterListener(p->paramID, this);
        }
    }
    
}

Granular_SynthAudioProcessor::~Granular_SynthAudioProcessor()
{
    apvts.removeParameterListener("L1_REC", this);
    apvts.removeParameterListener("L2_REC", this);
    apvts.removeParameterListener("L3_REC", this);
    apvts.removeParameterListener("L4_REC", this);
}

//==============================================================================
const juce::String Granular_SynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Granular_SynthAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool Granular_SynthAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool Granular_SynthAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double Granular_SynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Granular_SynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int Granular_SynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Granular_SynthAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String Granular_SynthAudioProcessor::getProgramName(int index)
{
    return {};
}

void Granular_SynthAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void Granular_SynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    // 1. Damos el tamaño correcto a nuestros cables internos
    renderBufferL1.setSize(spec.numChannels, samplesPerBlock);
    renderBufferL2.setSize(spec.numChannels, samplesPerBlock);
    renderBufferL3.setSize(spec.numChannels, samplesPerBlock);
    renderBufferL4.setSize(spec.numChannels, samplesPerBlock);

    // 2. Preparamos las Reverbs
    reverbL1.prepare(spec);
    reverbL2.prepare(spec);
    reverbL3.prepare(spec);
    reverbL4.prepare(spec);

    // 3. Preparamos el Limitador
    masterLimiter.prepare(spec);
    masterLimiter.setRelease(10.0f);

    // 4. Preparamos el sintetizador
    synthL1.setCurrentPlaybackSampleRate(sampleRate);
    synthL2.setCurrentPlaybackSampleRate(sampleRate);
    synthL3.setCurrentPlaybackSampleRate(sampleRate);
    synthL4.setCurrentPlaybackSampleRate(sampleRate);
}

void Granular_SynthAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Granular_SynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void Granular_SynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // --- 1. ECHAMOS EL CANDADO MIENTRAS EL AUDIO SUENA ---
    const juce::ScopedLock sl(audioMutex);

    // 1. CAPTURAR MODWHEEL Y AFTERTOUCH
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 1) { // CC 1 es ModWheel
            globalModWheel.store(msg.getControllerValue() / 127.0f); // Lo normalizamos de 0.0 a 1.0
        }
        else if (msg.isChannelPressure() || msg.isAftertouch()) {
            globalAftertouch.store(msg.getChannelPressureValue() / 127.0f);
        }
    }
    // ==========================================================
    // --- 1. LEER EL RELOJ MAESTRO (DAW o MANUAL) ---
    // ==========================================================
    bool syncToDaw = apvts.getRawParameterValue("SYNC_TO_DAW")->load() > 0.5f;
    float manualBpm = apvts.getRawParameterValue("MANUAL_BPM")->load();

    currentBPM = manualBpm;
    isPlaying = true;

    if (syncToDaw)
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto positionInfo = playHead->getPosition())
            {
                if (positionInfo->getBpm().hasValue()) {
                    currentBPM = *positionInfo->getBpm();
                }
                isPlaying = positionInfo->getIsPlaying();
            }
        }
    }

    // ==========================================================
    // --- LÓGICA DE CAPA 1 (CYAN) ---
    // ==========================================================
    bool isPlayOnL1 = apvts.getRawParameterValue("L1_PLAY")->load() > 0.5f;
    bool isMidiOnL1 = apvts.getRawParameterValue("L1_MIDI")->load() > 0.5f;
    bool isHoldOnL1 = apvts.getRawParameterValue("L1_HOLD")->load() > 0.5f;

    juce::MidiBuffer processedMidiL1;
    for (const auto metadata : midiMessages) {
        auto message = metadata.getMessage();
        if (isMidiOnL1) {
            if (isHoldOnL1 && message.isNoteOff()) continue;
            processedMidiL1.addEvent(message, metadata.samplePosition);
        }
    }

    if (isPlayOnL1 && !lastPlayState) processedMidiL1.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
    else if (!isPlayOnL1 && lastPlayState) processedMidiL1.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    lastPlayState = isPlayOnL1;

    if (!isHoldOnL1 && lastHoldState) synthL1.allNotesOff(0, true);
    lastHoldState = isHoldOnL1;

    // ==========================================================
    // --- LÓGICA DE CAPA 2 (MAGENTA) ---
    // ==========================================================
    bool isPlayOnL2 = apvts.getRawParameterValue("L2_PLAY")->load() > 0.5f;
    bool isMidiOnL2 = apvts.getRawParameterValue("L2_MIDI")->load() > 0.5f;
    bool isHoldOnL2 = apvts.getRawParameterValue("L2_HOLD")->load() > 0.5f;

    juce::MidiBuffer processedMidiL2;
    for (const auto metadata : midiMessages) {
        auto message = metadata.getMessage();
        if (isMidiOnL2) {
            if (isHoldOnL2 && message.isNoteOff()) continue;
            processedMidiL2.addEvent(message, metadata.samplePosition);
        }
    }

    if (isPlayOnL2 && !lastPlayStateL2) processedMidiL2.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
    else if (!isPlayOnL2 && lastPlayStateL2) processedMidiL2.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    lastPlayStateL2 = isPlayOnL2;

    if (!isHoldOnL2 && lastHoldStateL2) synthL2.allNotesOff(0, true);
    lastHoldStateL2 = isHoldOnL2;

    // ==========================================================
    // --- LÓGICA DE CAPA 3 (NARANJA) ---
    // ==========================================================
    bool isPlayOnL3 = apvts.getRawParameterValue("L3_PLAY")->load() > 0.5f;
    bool isMidiOnL3 = apvts.getRawParameterValue("L3_MIDI")->load() > 0.5f;
    bool isHoldOnL3 = apvts.getRawParameterValue("L3_HOLD")->load() > 0.5f;

    juce::MidiBuffer processedMidiL3;
    for (const auto metadata : midiMessages) {
        auto message = metadata.getMessage();
        if (isMidiOnL3) {
            if (isHoldOnL3 && message.isNoteOff()) continue;
            processedMidiL3.addEvent(message, metadata.samplePosition);
        }
    }

    if (isPlayOnL3 && !lastPlayStateL3) processedMidiL3.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
    else if (!isPlayOnL3 && lastPlayStateL3) processedMidiL3.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    lastPlayStateL3 = isPlayOnL3;

    if (!isHoldOnL3 && lastHoldStateL3) synthL3.allNotesOff(0, true);
    lastHoldStateL3 = isHoldOnL3;

    // ==========================================================
    // --- LÓGICA DE CAPA 4 (VERDE/LIMA) ---
    // ==========================================================
    bool isPlayOnL4 = apvts.getRawParameterValue("L4_PLAY")->load() > 0.5f;
    bool isMidiOnL4 = apvts.getRawParameterValue("L4_MIDI")->load() > 0.5f;
    bool isHoldOnL4 = apvts.getRawParameterValue("L4_HOLD")->load() > 0.5f;

    juce::MidiBuffer processedMidiL4;
    for (const auto metadata : midiMessages) {
        auto message = metadata.getMessage();
        if (isMidiOnL4) {
            if (isHoldOnL4 && message.isNoteOff()) continue;
            processedMidiL4.addEvent(message, metadata.samplePosition);
        }
    }

    if (isPlayOnL4 && !lastPlayStateL4) processedMidiL4.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8)127), 0);
    else if (!isPlayOnL4 && lastPlayStateL4) processedMidiL4.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    lastPlayStateL4 = isPlayOnL4;

    if (!isHoldOnL4 && lastHoldStateL4) synthL4.allNotesOff(0, true);
    lastHoldStateL4 = isHoldOnL4;


    // ==========================================================
    // --- LÓGICA DE RETRIGGER DEL LFO ---
    // ==========================================================
    bool retrigLfo = apvts.getRawParameterValue("LFO_RETRIG")->load() > 0.5f;
    if (retrigLfo) {
        for (const auto metadata : processedMidiL1) { if (metadata.getMessage().isNoteOn()) { lfo1Phase = 0.0f; lfo2Phase = 0.0f; } }
        for (const auto metadata : processedMidiL2) { if (metadata.getMessage().isNoteOn()) { lfo1Phase = 0.0f; lfo2Phase = 0.0f; } }
        for (const auto metadata : processedMidiL3) { if (metadata.getMessage().isNoteOn()) { lfo1Phase = 0.0f; lfo2Phase = 0.0f; } }
        for (const auto metadata : processedMidiL4) { if (metadata.getMessage().isNoteOn()) { lfo1Phase = 0.0f; lfo2Phase = 0.0f; } }
    }

    // ==========================================================
    // --- 1.5 CÁLCULO DE LOS LFOs (CONTROL RATE) ---
    // ==========================================================
    float blockDuration = buffer.getNumSamples() / (float)currentSampleRate;
    float beatsPerSecond = currentBPM / 60.0f;
    auto getBpsMultiplier = [](int idx) -> float {
        switch (idx) {
        case 0: return 0.03125f; case 1: return 0.0625f;  case 2: return 0.125f;
        case 3: return 0.25f;    case 4: return 0.5f;     case 5: return 1.0f;
        case 6: return 2.0f;     case 7: return 4.0f;     case 8: return 8.0f;
        default: return 1.0f;
        }
        };

    int beatIdx1 = (int)apvts.getRawParameterValue("LFO1_BEAT")->load();
    float freq1 = beatsPerSecond * getBpsMultiplier(beatIdx1);
    lfo1Phase += freq1 * blockDuration;
    if (lfo1Phase >= 1.0f) lfo1Phase -= 1.0f;

    int beatIdx2 = (int)apvts.getRawParameterValue("LFO2_BEAT")->load();
    float freq2 = beatsPerSecond * getBpsMultiplier(beatIdx2);
    lfo2Phase += freq2 * blockDuration;
    if (lfo2Phase >= 1.0f) lfo2Phase -= 1.0f;

    float tableIndex = lfo2Phase * (LFO_TABLE_SIZE - 1);
    int indexA = (int)tableIndex;
    int indexB = indexA + 1;
    if (indexB >= LFO_TABLE_SIZE) indexB = 0;
    float frac = tableIndex - (float)indexA;
    float lfo2Output = lfo2Table[indexA] + frac * (lfo2Table[indexB] - lfo2Table[indexA]);
    globalLfo2Value = lfo2Output;

    int waveType = (int)apvts.getRawParameterValue("LFO1_WAVE")->load();
    float lfo1Output = 0.0f;

    if (waveType == 0)      lfo1Output = std::sin(lfo1Phase * juce::MathConstants<float>::twoPi);
    else if (waveType == 1) lfo1Output = 2.0f * std::abs(2.0f * lfo1Phase - 1.0f) - 1.0f;
    else if (waveType == 2) lfo1Output = 1.0f - 2.0f * lfo1Phase;
    else if (waveType == 3) lfo1Output = (lfo1Phase < 0.5f) ? 1.0f : -1.0f;
    else if (waveType == 4) {
        static float lastRand = 0.0f;
        static float lastPhase = 0.0f;
        if (lfo1Phase < lastPhase) lastRand = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
        lastPhase = lfo1Phase;
        lfo1Output = lastRand;
    }

    float jitter1 = apvts.getRawParameterValue("LFO1_JITTER")->load();
    if (jitter1 > 0.0f) {
        float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
        lfo1Output += noise * jitter1 * 0.3f;
        lfo1Output = juce::jlimit(-1.0f, 1.0f, lfo1Output);
    }

    float amp1 = apvts.getRawParameterValue("LFO1_DEPTH")->load();
    globalLfo1Value = lfo1Output * amp1;

    for (int i = 0; i < synthL1.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<GranularVoice*>(synthL1.getVoice(i))) { voice->currentLfo1Value = globalLfo1Value; voice->currentLfo2Value = globalLfo2Value; }
    }
    for (int i = 0; i < synthL2.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<GranularVoice*>(synthL2.getVoice(i))) { voice->currentLfo1Value = globalLfo1Value; voice->currentLfo2Value = globalLfo2Value; }
    }
    for (int i = 0; i < synthL3.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<GranularVoice*>(synthL3.getVoice(i))) { voice->currentLfo1Value = globalLfo1Value; voice->currentLfo2Value = globalLfo2Value; }
    }
    for (int i = 0; i < synthL4.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<GranularVoice*>(synthL4.getVoice(i))) { voice->currentLfo1Value = globalLfo1Value; voice->currentLfo2Value = globalLfo2Value; }
    }

    // ==========================================================
    // --- 2. PROCESAMIENTO INDEPENDIENTE DE AUDIO Y EFECTOS ---
    // ==========================================================
    buffer.clear();

    renderBufferL1.clear();
    renderBufferL2.clear();
    renderBufferL3.clear();
    renderBufferL4.clear();

    synthL1.renderNextBlock(renderBufferL1, processedMidiL1, 0, buffer.getNumSamples());
    synthL2.renderNextBlock(renderBufferL2, processedMidiL2, 0, buffer.getNumSamples());
    synthL3.renderNextBlock(renderBufferL3, processedMidiL3, 0, buffer.getNumSamples());
    synthL4.renderNextBlock(renderBufferL4, processedMidiL4, 0, buffer.getNumSamples());

    // ==========================================================
    // --- PANEO Y EFECTOS EXTERNOS (CON MATRIZ INYECTADA) ---
    // ==========================================================

    auto applyLayerPan = [this](juce::AudioBuffer<float>& layerBuffer, juce::String prefix) {
        float panVal = apvts.getRawParameterValue(prefix + "PAN")->load();

        // Matriz para PAN (Se mueve de -100 a +100)
        float modPan = getMatrixModulation(prefix + "PAN", 0.0f, 0.0f, globalLfo1Value, globalLfo2Value);
        if (modPan != 0.0f) panVal = juce::jlimit(-100.0f, 100.0f, panVal + (modPan * 100.0f));

        float panMapped = (panVal / 100.0f + 1.0f) * 0.5f;
        float gainL = std::cos(panMapped * juce::MathConstants<float>::halfPi);
        float gainR = std::sin(panMapped * juce::MathConstants<float>::halfPi);

        layerBuffer.applyGain(0, 0, layerBuffer.getNumSamples(), gainL);
        layerBuffer.applyGain(1, 0, layerBuffer.getNumSamples(), gainR);
        };

    applyLayerPan(renderBufferL1, "L1_");
    applyLayerPan(renderBufferL2, "L2_");
    applyLayerPan(renderBufferL3, "L3_");
    applyLayerPan(renderBufferL4, "L4_");

    auto applyEffectsToLayer = [this](juce::AudioBuffer<float>& layerBuffer, juce::String prefix, juce::dsp::Reverb& reverb)
        {
            float driveParam = apvts.getRawParameterValue(prefix + "DIST_DRIVE")->load();
            float mixDist = apvts.getRawParameterValue(prefix + "DIST_MIX")->load();
            int typeParam = (int)apvts.getRawParameterValue(prefix + "DIST_TYPE")->load();
            float mixReverb = apvts.getRawParameterValue(prefix + "SPACE_MIX")->load();

            // Lambda local para la matriz externa
            auto applyExtMod = [&](const juce::String& paramName, float& targetVar, float minVal, float maxVal, float scale) {
                float mod = getMatrixModulation(prefix + paramName, 0.0f, 0.0f, globalLfo1Value, globalLfo2Value);
                if (mod != 0.0f) targetVar = juce::jlimit(minVal, maxVal, targetVar + (mod * scale));
                };

            applyExtMod("DIST_DRIVE", driveParam, 0.0f, 100.0f, 50.0f);
            applyExtMod("DIST_MIX", mixDist, 0.0f, 100.0f, 50.0f);
            applyExtMod("SPACE_MIX", mixReverb, 0.0f, 1.0f, 0.5f);

            mixDist /= 100.0f; // Convertimos mix de Distorsión a 0.0 - 1.0

            if (mixDist > 0.001f) {
                float driveMult = juce::jmap(driveParam, 0.0f, 100.0f, 1.0f, 15.0f);
                float autoGain = 1.0f / std::sqrt(driveMult);
                float bitCrushDepth = juce::jmap(driveParam, 0.0f, 100.0f, 16.0f, 2.0f);
                float bitStep = std::pow(2.0f, bitCrushDepth - 1.0f);

                for (int ch = 0; ch < layerBuffer.getNumChannels(); ++ch) {
                    auto* chData = layerBuffer.getWritePointer(ch);
                    for (int s = 0; s < layerBuffer.getNumSamples(); ++s) {
                        float clean = chData[s];
                        float input = clean * driveMult;
                        float dist = clean;

                        switch (typeParam) {
                        case 0: dist = std::tanh(input); break;
                        case 1: dist = juce::jlimit(-1.0f, 1.0f, input); break;
                        case 2: dist = std::sin(input * juce::MathConstants<float>::halfPi); break;
                        case 3: dist = juce::jlimit(-1.0f, 1.0f, std::round(input * bitStep) / bitStep); break;
                        }
                        dist *= autoGain;
                        chData[s] = (clean * (1.0f - mixDist)) + (dist * mixDist);
                    }
                }
            }

            // ESPACIO REVERBERANTE GLOBAL
            float spaceSize = apvts.getRawParameterValue("SPACE_SIZE")->load();
            float spaceFback = apvts.getRawParameterValue("SPACE_FBACK")->load();

            juce::Reverb::Parameters params;
            params.roomSize = spaceSize;
            params.damping = 1.0f - spaceFback;
            params.width = 1.0f;
            params.freezeMode = 0.0f;
            params.dryLevel = std::cos(mixReverb * juce::MathConstants<float>::halfPi);
            params.wetLevel = std::sin(mixReverb * juce::MathConstants<float>::halfPi);

            reverb.setParameters(params);

            juce::dsp::AudioBlock<float> block(layerBuffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            reverb.process(context);
        };

    applyEffectsToLayer(renderBufferL1, "L1_", reverbL1);
    applyEffectsToLayer(renderBufferL2, "L2_", reverbL2);
    applyEffectsToLayer(renderBufferL3, "L3_", reverbL3);
    applyEffectsToLayer(renderBufferL4, "L4_", reverbL4);

    // ==========================================================
    // --- LEEMOS LOS MUTES Y SOLOS (Lógica Universal) ---
    // ==========================================================
    bool m1 = apvts.getRawParameterValue("L1_MUTE")->load() > 0.5f;
    bool m2 = apvts.getRawParameterValue("L2_MUTE")->load() > 0.5f;
    bool m3 = apvts.getRawParameterValue("L3_MUTE")->load() > 0.5f;
    bool m4 = apvts.getRawParameterValue("L4_MUTE")->load() > 0.5f;

    bool s1 = apvts.getRawParameterValue("L1_SOLO")->load() > 0.5f;
    bool s2 = apvts.getRawParameterValue("L2_SOLO")->load() > 0.5f;
    bool s3 = apvts.getRawParameterValue("L3_SOLO")->load() > 0.5f;
    bool s4 = apvts.getRawParameterValue("L4_SOLO")->load() > 0.5f;

    // Si ALGUNA capa está en Solo, entramos en modo Solo-Exclusivo
    bool anySoloActive = s1 || s2 || s3 || s4;

    // El objetivo (1.0f ON, 0.0f OFF). Una capa suena si NO está muteada Y (no hay solos OR ella tiene el solo)
    float targetMuteL1 = (!m1 && (!anySoloActive || s1)) ? 1.0f : 0.0f;
    float targetMuteL2 = (!m2 && (!anySoloActive || s2)) ? 1.0f : 0.0f;
    float targetMuteL3 = (!m3 && (!anySoloActive || s3)) ? 1.0f : 0.0f;
    float targetMuteL4 = (!m4 && (!anySoloActive || s4)) ? 1.0f : 0.0f;

    // Fades súper rápidos de 5ms. ¡Adiós clicks!
    renderBufferL1.applyGainRamp(0, buffer.getNumSamples(), lastMuteGainL1, targetMuteL1);
    renderBufferL2.applyGainRamp(0, buffer.getNumSamples(), lastMuteGainL2, targetMuteL2);
    renderBufferL3.applyGainRamp(0, buffer.getNumSamples(), lastMuteGainL3, targetMuteL3);
    renderBufferL4.applyGainRamp(0, buffer.getNumSamples(), lastMuteGainL4, targetMuteL4);

    lastMuteGainL1 = targetMuteL1;
    lastMuteGainL2 = targetMuteL2;
    lastMuteGainL3 = targetMuteL3;
    lastMuteGainL4 = targetMuteL4;

    // Sumamos todas las capas SIEMPRE (Si no tienen que sonar, ya estarán a volumen 0)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.addFrom(ch, 0, renderBufferL1, ch, 0, buffer.getNumSamples());
        buffer.addFrom(ch, 0, renderBufferL2, ch, 0, buffer.getNumSamples());
        buffer.addFrom(ch, 0, renderBufferL3, ch, 0, buffer.getNumSamples());
        buffer.addFrom(ch, 0, renderBufferL4, ch, 0, buffer.getNumSamples());
    }

    // ==========================================================
    // --- 3. MASTER VOLUME & BRICKWALL LIMITER ---
    // ==========================================================
    float masterVolDb = apvts.getRawParameterValue("MASTER_VOL")->load();
    float limiterThresh = apvts.getRawParameterValue("LIMITER_THRESH")->load();

    float volumeMultiplier = juce::Decibels::decibelsToGain(masterVolDb);
    buffer.applyGain(volumeMultiplier);

    masterLimiter.setThreshold(limiterThresh);
    juce::dsp::AudioBlock<float> limiterBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> limiterContext(limiterBlock);
    masterLimiter.process(limiterContext);

    // ==========================================================
    // --- 4. ACTUALIZAR LOS ESPÍAS VISUALES ---
    // ==========================================================
    float peakL = buffer.getMagnitude(0, 0, buffer.getNumSamples());
    float peakR = buffer.getMagnitude(1, 0, buffer.getNumSamples());

    float dbL = juce::Decibels::gainToDecibels(peakL, -60.0f);
    float dbR = juce::Decibels::gainToDecibels(peakR, -60.0f);

    visualMeterL.store(dbL);
    visualMeterR.store(dbR);
}

//==============================================================================
bool Granular_SynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* Granular_SynthAudioProcessor::createEditor()
{
    return new Granular_SynthAudioProcessorEditor(*this);
}

//==============================================================================
void Granular_SynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, true);

    // 1. Inyectamos las rutas de los audios antes de que el DAW guarde
    apvts.state.setProperty("AUDIO_PATH_L1", lastLoadedFilePathL1, nullptr);
    apvts.state.setProperty("AUDIO_PATH_L2", lastLoadedFilePathL2, nullptr);
    apvts.state.setProperty("AUDIO_PATH_L3", lastLoadedFilePathL3, nullptr);
    apvts.state.setProperty("AUDIO_PATH_L4", lastLoadedFilePathL4, nullptr);

    // 2. Hacemos la fotografía
    auto state = apvts.copyState();

    if (state.isValid()) {
        state.writeToStream(stream);
    }
}

void Granular_SynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);

    if (tree.isValid()) {
        // 1. Restauramos los knobs guardados por el DAW
        apvts.replaceState(tree);

        // 2. Recuperamos los audios y los cargamos a la memoria RAM
        juce::String path1 = tree.getProperty("AUDIO_PATH_L1").toString();
        juce::String path2 = tree.getProperty("AUDIO_PATH_L2").toString();
        juce::String path3 = tree.getProperty("AUDIO_PATH_L3").toString();
        juce::String path4 = tree.getProperty("AUDIO_PATH_L4").toString();

        if (path1.isNotEmpty()) loadFile(path1, 1); else clearFile(1);
        if (path2.isNotEmpty()) loadFile(path2, 2); else clearFile(2);
        if (path3.isNotEmpty()) loadFile(path3, 3); else clearFile(3);
        if (path4.isNotEmpty()) loadFile(path4, 4); else clearFile(4);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Granular_SynthAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout Granular_SynthAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ==============================================================================
    // 1. PARÁMETROS GLOBALES
    // ==============================================================================
    juce::StringArray beatDivisions = { "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };
    juce::StringArray waveShapes = { "Sine", "Triangle", "Saw", "Square", "S&H" };

    params.push_back(std::make_unique<juce::AudioParameterChoice>("LFO1_BEAT", "LFO 1 Rate", beatDivisions, 5));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("LFO1_WAVE", "LFO 1 Wave", waveShapes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LFO1_DEPTH", "LFO 1 Amp", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LFO1_JITTER", "LFO 1 Jitter", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("LFO2_BEAT", "LFO 2 Rate", beatDivisions, 5));
    params.push_back(std::make_unique<juce::AudioParameterBool>("LFO_RETRIG", "LFO Retrig", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("MASTER_VOL", "Master Vol", juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f, 2.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LIMITER_THRESH", "Limiter", juce::NormalisableRange<float>(-6.0f, 0.0f, 0.1f), -0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPACE_SIZE", "Reverb Size", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPACE_FBACK", "Reverb Fback", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("MANUAL_BPM", "BPM", juce::NormalisableRange<float>(20.0f, 300.0f, 0.1f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("SYNC_TO_DAW", "DAW Sync", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_A", "Env2 A", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_D", "Env2 D", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_S", "Env2 S", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_R", "Env2 R", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 1.0f));

    // ==============================================================================
    // 2. FUNCIÓN GENERADORA DE CAPAS
    // ==============================================================================
    auto addLayerParameters = [&](juce::String prefix)
        {
            // Controles Base (NUEVO: SOLO AÑADIDO)
            params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_PLAY", "Play", false));
            params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_MIDI", "MIDI", true));
            params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_HOLD", "Hold", false));
            params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_MUTE", "Mute", false));
            params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_SOLO", "Solo", false));

            // Granular Engine
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_POSITION", "Position", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_GRAIN_SIZE", "Grain Size", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.3f), 0.1f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_SCAN_SPEED", "Scan Speed", juce::NormalisableRange<float>(-2.0f, 2.0f, 0.01f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_SPRAY_POS", "Pos Spray", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_DENSITY", "Density", juce::NormalisableRange<float>(1.0f, 120.0f, 0.1f, 0.5f), 20.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_SHAPE", "Shape", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_SPRAY_PAN", "Pan Spray", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_SPRAY_PITCH", "Pitch Spray", juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_SCAN_MODE", "Scan Mode", juce::NormalisableRange<float>(0.0f, 2.0f, 1.0f), 0.0f));

            // Pitch
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_PITCH_TRANS", "Transpose", juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_PITCH_FINE", "Fine", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_PITCH_SCALE", "Scale", juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 0.0f));

            // Filter
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_FILTER_LPF", "LPF", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_FILTER_RES_LPF", "LPF Res", juce::NormalisableRange<float>(0.707f, 2.5f, 0.01f), 0.707f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_FILTER_HPF", "HPF", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_FILTER_RES_HPF", "HPF Res", juce::NormalisableRange<float>(0.707f, 2.5f, 0.01f), 0.707f));

            // Envelopes
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_AMP_A", "Amp A", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.1f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_AMP_D", "Amp D", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.5f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_AMP_S", "Amp S", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_AMP_R", "Amp R", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 1.0f));

            // Effects
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_DIST_DRIVE", "Drive", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_DIST_MIX", "Dist Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "_DIST_TYPE", "Type", juce::StringArray{ "Soft Clip", "Hard Clip", "Foldback", "Bitcrush" }, 0));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_SPACE_MIX", "Reverb Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

            // FX: 4 choral voices
            for (int v = 1; v <= 4; ++v) {
                juce::String vPref = prefix + "_M" + juce::String(v);
                params.push_back(std::make_unique<juce::AudioParameterFloat>(vPref + "_X", "Voice " + juce::String(v) + " X", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
                params.push_back(std::make_unique<juce::AudioParameterFloat>(vPref + "_Y", "Voice " + juce::String(v) + " Y", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
                params.push_back(std::make_unique<juce::AudioParameterFloat>(vPref + "_MIX", "Voice " + juce::String(v) + " Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
            }

            // HALO & ENSEMBLE
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_HALO_PITCH", "Halo Pitch", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_HALO_SHIMMER", "Halo Shimmer", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_HALO_COLOR", "Halo Color", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_HALO_MIX", "Halo Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_ENS_RATE", "Ensemble Rate", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_ENS_DEPTH", "Ensemble Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_ENS_WIDTH", "Ensemble Width", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_ENS_MIX", "Ensemble Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

            // Mixer & EQ
            juce::NormalisableRange<float> eqRange(-60.0f, 15.0f, 0.1f);
            eqRange.setSkewForCentre(0.0f);
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_MIX_VOL", "Vol", juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f, 2.0f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_PAN", "Pan", juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_EQ_LOW", "EQ Low", eqRange, 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_EQ_MID_LOW", "EQ Mid-L", eqRange, 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_EQ_MID_HIGH", "EQ Mid-H", eqRange, 0.0f));
            params.push_back(std::make_unique<juce::AudioParameterFloat>(prefix + "_EQ_HIGH", "EQ High", eqRange, 0.0f));

            // --- NUEVOS PARÁMETROS DE GRABACIÓN ---
            params.push_back(std::make_unique<juce::AudioParameterBool>(prefix + "_REC", "Record", false));
            // Actualizado para TCP Dual (WiFi y USB)
            //params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "_REC_MODE", "Rec Mode",
                //juce::StringArray{ "DAW / MIC IN", "WIFI FILE (TCP)", "USB FILE (TCP)" }, 0));

            // Actualizado: Solo DAW y WIFI (Adiós USB)
            params.push_back(std::make_unique<juce::AudioParameterChoice>(prefix + "_R_MODE", "Rec Mode",
                juce::StringArray{ "DAW / MIC IN", "WIFI FILE (TCP)" }, 0));
        };

    addLayerParameters("L1"); addLayerParameters("L2"); addLayerParameters("L3"); addLayerParameters("L4");

    return { params.begin(), params.end() };
}

void Granular_SynthAudioProcessor::loadFile(const juce::String& path, int layerIndex)
{
    juce::File file(path);
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader != nullptr)
    {
        juce::AudioBuffer<float> tempBuffer((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&tempBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

        const juce::ScopedLock sl(audioMutex);

        if (layerIndex == 1) {
            isUpdatingBufferL1.store(true); // SEMÁFORO EN ROJO (Para el audio)
            audioBufferL1.makeCopyOf(tempBuffer);
            isAudioLoadedL1 = true;
            lastLoadedFilePathL1 = path;
            isUpdatingBufferL1.store(false); // SEMÁFORO EN VERDE
        }
        else if (layerIndex == 2) {
            isUpdatingBufferL2.store(true);
            audioBufferL2.makeCopyOf(tempBuffer);
            isAudioLoadedL2 = true;
            lastLoadedFilePathL2 = path;
            isUpdatingBufferL2.store(false);
        }
        else if (layerIndex == 3) {
            isUpdatingBufferL3.store(true);
            audioBufferL3.makeCopyOf(tempBuffer);
            isAudioLoadedL3 = true;
            lastLoadedFilePathL3 = path;
            isUpdatingBufferL3.store(false);
        }
        else if (layerIndex == 4) {
            isUpdatingBufferL4.store(true);
            audioBufferL4.makeCopyOf(tempBuffer);
            isAudioLoadedL4 = true;
            lastLoadedFilePathL4 = path;
            isUpdatingBufferL4.store(false);
        }
        
        sendChangeMessage();
        DBG("Archivo cargado en Capa " + juce::String(layerIndex) + ": " + file.getFileName());
    }
}

void Granular_SynthAudioProcessor::clearFile(int layerIndex)
{
    // 1. Apagamos solo las notas de la capa que vamos a borrar
    if (layerIndex == 1) synthL1.allNotesOff(0, false);
    else if (layerIndex == 2) synthL2.allNotesOff(0, false);
    else if (layerIndex == 3) synthL3.allNotesOff(0, false);
    else if (layerIndex == 4) synthL4.allNotesOff(0, false);

    // 2. Preparamos la orden "BORRAR CAPA" (Acción 2)
    pendingAction = 2;
    pendingLayerToClear = layerIndex;

    // 3. Arrancamos el reloj
    startTimer(40);
}

void Granular_SynthAudioProcessor::timerCallback()
{
    stopTimer(); // Apagamos el cronómetro para que no se repita en bucle
    const juce::ScopedLock sl(audioMutex);

    if (pendingAction == 1)
    {
        // === EJECUTAR EL INIT TOTAL ===
        for (auto* param : getParameters()) {
            if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
                p->setValueNotifyingHost(p->getDefaultValue());
            }
        }

        // Vaciamos la RAM ahora que hay silencio total
        audioBufferL1.clear(); isAudioLoadedL1 = false; lastLoadedFilePathL1 = "";
        audioBufferL2.clear(); isAudioLoadedL2 = false; lastLoadedFilePathL2 = "";
        audioBufferL3.clear(); isAudioLoadedL3 = false; lastLoadedFilePathL3 = "";
        audioBufferL4.clear(); isAudioLoadedL4 = false; lastLoadedFilePathL4 = "";

        sendChangeMessage(); // Repintar pantalla en negro
        DBG("INIT ejecutado de forma segura y sin clicks.");
    }
    else if (pendingAction == 2)
    {
        // === EJECUTAR EL BORRADO DE UNA SOLA CAPA (La "X") ===
        if (pendingLayerToClear == 1) { audioBufferL1.clear(); isAudioLoadedL1 = false; lastLoadedFilePathL1 = ""; }
        else if (pendingLayerToClear == 2) { audioBufferL2.clear(); isAudioLoadedL2 = false; lastLoadedFilePathL2 = ""; }
        else if (pendingLayerToClear == 3) { audioBufferL3.clear(); isAudioLoadedL3 = false; lastLoadedFilePathL3 = ""; }
        else if (pendingLayerToClear == 4) { audioBufferL4.clear(); isAudioLoadedL4 = false; lastLoadedFilePathL4 = ""; }

        sendChangeMessage(); // Repintar pantalla
        DBG("Capa " + juce::String(pendingLayerToClear) + " borrada de forma segura.");
    }
}

void Granular_SynthAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // --- TRAMPA CLICK-TO-MAP ---
    int mappingCol = activeMappingColumn.load();
    if (mappingCol != -1)
    {
        // Evitamos mapear botones de sistema
        if (!parameterID.contains("_MUTE") && !parameterID.contains("_PLAY") &&
            !parameterID.contains("_REC") && !parameterID.contains("_SOLO"))
        {
            targetColumns[mappingCol] = parameterID; // Asignamos el parámetro a la columna
            activeMappingColumn.store(-1);           // Desarmamos la trampa

            juce::MessageManager::callAsync([this] { sendChangeMessage(); });
            DBG("Columna " + juce::String(mappingCol) + " ahora controla: " + parameterID);
            return; // Salimos
        }
    }
    if (parameterID.endsWith("_REC"))
    {
        int layerIndex = parameterID.substring(1, 2).getIntValue();
        bool isRecording = (newValue > 0.5f);

        if (isRecording)
        {
            int currentMode = (int)apvts.getRawParameterValue("L" + juce::String(layerIndex) + "_R_MODE")->load();

            // Si es 1 (WIFI), encendemos el servidor 8080
            if (currentMode == 1)
            {
                tcpReceiver->startListening(layerIndex);
            }
            else
            {
                tcpReceiver->stopListening();
            }
        }
        else
        {
            tcpReceiver->stopListening();
        }
    }
}

void Granular_SynthAudioProcessor::oscMessageReceived(const juce::OSCMessage& message)
{
    juce::String address = message.getAddressPattern().toString();

    // 1. SINCRONIZACIÓN DE CAPA (Acepta tanto Float como Int)
    if (address == "/SELECT_LAYER" && message.size() == 1)
    {
        int layer = 0;
        if (message[0].isFloat32()) layer = (int)message[0].getFloat32();
        else if (message[0].isInt32()) layer = message[0].getInt32();

        if (layer > 0) {
            uiLayerRequested.store(layer);
            // Avisamos directamente a la interfaz que repinte ya
            juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
        }
        return;
    }

    // 2. MAPEO DE PARÁMETROS NORMALES
    if (message.size() == 1)
    {
        float rawValue = 0.0f;
        if (message[0].isFloat32()) rawValue = message[0].getFloat32();
        else if (message[0].isInt32()) rawValue = (float)message[0].getInt32();

        juce::String paramID = address.substring(1);
        if (auto* param = apvts.getParameter(paramID))
        {
            param->setValueNotifyingHost(rawValue);
        }
    }
}

// ==============================================================================
// --- SISTEMA DE PRESETS (NIVEL 3 - BINARIO CUSTOM) ---
// ==============================================================================

// Función auxiliar: Crea/Busca la carpeta en Mis Documentos
juce::File getPresetFolder()
{
    juce::File docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    juce::File presetFolder = docsDir.getChildFile("Granular_Synth_Presets");

    if (!presetFolder.exists()) {
        presetFolder.createDirectory();
    }
    return presetFolder;
}

void Granular_SynthAudioProcessor::savePreset(int presetIndex)
{
    juce::String fileName = "Preset_" + juce::String(presetIndex + 1) + ".gsp";
    juce::File presetFile = getPresetFolder().getChildFile(fileName);
    juce::FileOutputStream stream(presetFile);

    if (stream.openedOk()) {
        stream.setPosition(0);
        stream.truncate();

        // 1. INYECTAMOS LOS TEXTOS (Rutas de los audios) en el árbol VIVO
        apvts.state.setProperty("AUDIO_PATH_L1", lastLoadedFilePathL1, nullptr);
        apvts.state.setProperty("AUDIO_PATH_L2", lastLoadedFilePathL2, nullptr);
        apvts.state.setProperty("AUDIO_PATH_L3", lastLoadedFilePathL3, nullptr);
        apvts.state.setProperty("AUDIO_PATH_L4", lastLoadedFilePathL4, nullptr);

        // --- INYECTAR LA MATRIZ DE MODULACIÓN ---
        for (int c = 0; c < 6; ++c) {
            // Guardamos el nombre del destino (ej: "L1_FILTER_LPF")
            apvts.state.setProperty("MOD_TARGET_" + juce::String(c), targetColumns[c], nullptr);

            // Guardamos las 6 profundidades de esa columna
            for (int r = 0; r < 6; ++r) {
                apvts.state.setProperty("MOD_DEPTH_" + juce::String(r) + "_" + juce::String(c), modDepths[r][c], nullptr);
            }
        }

        // 2. Hacemos la fotografía con los knobs + los textos
        auto state = apvts.copyState();

        if (state.isValid()) {
            state.writeToStream(stream);
            DBG("Preset BINARIO guardado con audio en: " + presetFile.getFullPathName());
        }
        else {
            DBG("Error: La copia del estado es inválida.");
        }
    }
    else {
        DBG("Error: No se pudo abrir el archivo para escribir.");
    }
}

void Granular_SynthAudioProcessor::loadPreset(int presetIndex)
{
    juce::String fileName = "Preset_" + juce::String(presetIndex + 1) + ".gsp";
    juce::File presetFile = getPresetFolder().getChildFile(fileName);

    if (presetFile.existsAsFile()) {
        juce::FileInputStream stream(presetFile);

        if (stream.openedOk()) {
            auto tree = juce::ValueTree::readFromStream(stream);

            if (tree.isValid()) {
                // 1. Restauramos los knobs
                apvts.replaceState(tree);

                // 2. RECUPERAMOS LOS TEXTOS y cargamos los archivos físicamente
                juce::String path1 = tree.getProperty("AUDIO_PATH_L1").toString();
                juce::String path2 = tree.getProperty("AUDIO_PATH_L2").toString();
                juce::String path3 = tree.getProperty("AUDIO_PATH_L3").toString();
                juce::String path4 = tree.getProperty("AUDIO_PATH_L4").toString();

                if (path1.isNotEmpty())
                    loadFile(path1, 1);
                else
                    clearFile(1);

                // Repite lo mismo para path2, path3 y path4
                if (path2.isNotEmpty()) loadFile(path2, 2); else clearFile(2);
                if (path3.isNotEmpty()) loadFile(path3, 3); else clearFile(3);
                if (path4.isNotEmpty()) loadFile(path4, 4); else clearFile(4);

                // --- RECUPERAR LA MATRIZ DE MODULACIÓN ---
                for (int c = 0; c < 6; ++c) {
                    targetColumns[c] = tree.getProperty("MOD_TARGET_" + juce::String(c)).toString();

                    for (int r = 0; r < 6; ++r) {
                        modDepths[r][c] = (float)tree.getProperty("MOD_DEPTH_" + juce::String(r) + "_" + juce::String(c));
                    }
                }

                // Forzamos a la pantalla a repintarse para mostrar los nuevos datos de la matriz
                juce::MessageManager::callAsync([this]() { sendChangeMessage(); });

                DBG("Preset BINARIO cargado con exito: " + fileName);
            }
        }
    }
    else {
        DBG("El preset no existe en disco: " + fileName);
    }
}

void Granular_SynthAudioProcessor::initSynth()
{
    // 1. Apagamos TODAS las notas para que hagan un micro fade-out
    synthL1.allNotesOff(0, false);
    synthL2.allNotesOff(0, false);
    synthL3.allNotesOff(0, false);
    synthL4.allNotesOff(0, false);

    // --- NUEVO: LIMPIAR LA MATRIZ DE MEMORIA ---
    for (int c = 0; c < 6; ++c) {
        targetColumns[c] = "";
        for (int r = 0; r < 6; ++r) {
            modDepths[r][c] = 0.0f;
        }
    }

    // 2. Preparamos la orden "INIT TOTAL" (Acción 1)
    pendingAction = 1;

    // 3. Arrancamos el reloj (40 ms de espera para evitar el click)
    startTimer(40);
}

bool Granular_SynthAudioProcessor::doesPresetExist(int presetIndex)
{
    juce::String fileName = "Preset_" + juce::String(presetIndex + 1) + ".gsp";
    juce::File presetFile = getPresetFolder().getChildFile(fileName);
    return presetFile.existsAsFile();
}

void Granular_SynthAudioProcessor::deletePreset(int presetIndex)
{
    juce::String fileName = "Preset_" + juce::String(presetIndex + 1) + ".gsp";
    juce::File presetFile = getPresetFolder().getChildFile(fileName);

    if (presetFile.existsAsFile()) {
        presetFile.deleteFile(); // Destruye el archivo físico
        DBG("Preset eliminado del disco: " + fileName);
    }
}

//void Granular_SynthAudioProcessor::setMappingMode(int slotIndex) {
    //activeMappingSlot.store(slotIndex); // Armamos la trampa
//}

//void Granular_SynthAudioProcessor::setModSource(int slotIndex, int sourceID) {
    //if (slotIndex >= 0 && slotIndex < 6) modSources[slotIndex] = sourceID;
//}

//void Granular_SynthAudioProcessor::setModDepth(int slotIndex, float depth) {
    //if (slotIndex >= 0 && slotIndex < 6) modDepths[slotIndex] = depth;
//}

void Granular_SynthAudioProcessor::setMappingColumn(int colIndex) {
    activeMappingColumn.store(colIndex);
}

void Granular_SynthAudioProcessor::setGridDepth(int sourceRow, int targetCol, float depth) {
    if (sourceRow >= 0 && sourceRow < 6 && targetCol >= 0 && targetCol < 6) {
        modDepths[sourceRow][targetCol] = depth;
    }
}

float Granular_SynthAudioProcessor::getMatrixModulation(const juce::String& paramID, float voiceVelocity, float voiceEnv2, float voiceLFO1, float voiceLFO2)
{
    float totalModOffset = 0.0f;

    // Buscamos en las 6 columnas a ver si el parámetro está asignado
    for (int c = 0; c < 6; ++c)
    {
        if (targetColumns[c] == paramID)
        {
            // ¡BINGO! El parámetro está en esta columna 'c'. Sumamos las 6 fuentes (filas).
            // modDepths va de -100 a 100. Lo dividimos por 100.0f para convertirlo en un multiplicador de -1.0 a 1.0

            totalModOffset += (voiceVelocity * (modDepths[0][c] / 100.0f));
            totalModOffset += (globalModWheel.load() * (modDepths[1][c] / 100.0f));
            totalModOffset += (globalAftertouch.load() * (modDepths[2][c] / 100.0f));
            totalModOffset += (voiceEnv2 * (modDepths[3][c] / 100.0f));
            totalModOffset += (voiceLFO1 * (modDepths[4][c] / 100.0f));
            totalModOffset += (voiceLFO2 * (modDepths[5][c] / 100.0f));

            break; // Ya lo hemos encontrado, salimos del bucle rápido
        }
    }

    return totalModOffset; // Esto nos devolverá un número entre -6.0 y +6.0 (normalmente entre -1.0 y 1.0)
}