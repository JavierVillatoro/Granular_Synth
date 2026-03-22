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

    // 1. Le damos a la orquesta la partitura (nuestra clase Sound que permite tocar cualquier nota)
    synth.addSound(new GranularSound());

    // 2. Contratamos a 8 "Voces" 
    for (int i = 0; i < 8; ++i)
    {
        synth.addVoice(new GranularVoice(&audioBuffer, &apvts));
    }
}

Granular_SynthAudioProcessor::~Granular_SynthAudioProcessor()
{
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

void Granular_SynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Granular_SynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void Granular_SynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Granular_SynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    // Preparamos la Reverb
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    masterReverb.prepare(spec);

    synth.setCurrentPlaybackSampleRate(sampleRate);
}

void Granular_SynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Granular_SynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Granular_SynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{   // ==========================================================
    // --- 1. LEER EL RELOJ DEL DAW (SYNC) ---
    // ==========================================================
    // Invocamos a la antena de JUCE
    if (auto* playHead = getPlayHead())
    {
        // Le pedimos al DAW la información de posición actual
        if (auto positionInfo = playHead->getPosition())
        {
            // Extraemos los BPM
            if (positionInfo->getBpm().hasValue()) {
                currentBPM = *positionInfo->getBpm();
            }

            // Comprobamos si el DAW está reproduciendo (Play)
            isPlaying = positionInfo->getIsPlaying();

            // (En el futuro aquí extraeremos el ppqPosition para sincronizar la fase al milímetro)
        }
    }
    else
    {
        // Si no hay DAW (modo Standalone), volveremos a un reloj interno.
        // De momento lo dejamos a 120 fijo y siempre reproduciendo.
        currentBPM = 120.0;
        isPlaying = true;
    }

    // ==========================================================
    // --- 1.5 CÁLCULO DE LOS LFOs (CONTROL RATE) ---
    // ==========================================================

    // 1. Calculamos cuánto tiempo "dura" este bloque de audio en segundos
    float blockDuration = buffer.getNumSamples() / (float)currentSampleRate;
    float beatsPerSecond = currentBPM / 60.0f; // Ej: 120 BPM = 2 beats por segundo

    // Tabla de conversión (El índice del ComboBox a multiplicador de tempo)
    // 1/4 (índice 5) significa 1 ciclo por cada beat (negra).
    auto getBpsMultiplier = [](int idx) -> float {
        switch (idx) {
        case 0: return 0.03125f; // 8/1
        case 1: return 0.0625f;  // 4/1
        case 2: return 0.125f;   // 2/1
        case 3: return 0.25f;    // 1/1 (Redonda)
        case 4: return 0.5f;     // 1/2 (Blanca)
        case 5: return 1.0f;     // 1/4 (Negra - Valor estándar)
        case 6: return 2.0f;     // 1/8 (Corchea)
        case 7: return 4.0f;     // 1/16 (Semicorchea)
        case 8: return 8.0f;     // 1/32 (Fusa)
        default: return 1.0f;
        }
        };

    // --- AVANZAMOS EL RELOJ DEL LFO 1 ---
    int beatIdx1 = (int)apvts.getRawParameterValue("LFO1_BEAT")->load();
    float freq1 = beatsPerSecond * getBpsMultiplier(beatIdx1);

    lfo1Phase += freq1 * blockDuration;
    if (lfo1Phase >= 1.0f) lfo1Phase -= 1.0f; // Bucle infinito de 0.0 a 1.0

    // --- AVANZAMOS EL RELOJ DEL LFO 2 ---
    //int beatIdx2 = (int)apvts.getRawParameterValue("LFO2_BEAT")->load();
    //float freq2 = beatsPerSecond * getBpsMultiplier(beatIdx2);

    //lfo2Phase += freq2 * blockDuration;
    //if (lfo2Phase >= 1.0f) lfo2Phase -= 1.0f;

    // --- AVANZAMOS EL RELOJ DEL LFO 2 ---
    int beatIdx2 = (int)apvts.getRawParameterValue("LFO2_BEAT")->load();
    float freq2 = beatsPerSecond * getBpsMultiplier(beatIdx2);

    lfo2Phase += freq2 * blockDuration;
    if (lfo2Phase >= 1.0f) lfo2Phase -= 1.0f;

    // --- MAGIA: WAVETABLE LOOKUP PARA EL LFO 2 ---
    // 1. Convertimos la fase (0.0 a 1.0) en un índice de la tabla (0 a 2047)
    float tableIndex = lfo2Phase * (LFO_TABLE_SIZE - 1);

    // 2. Buscamos entre qué dos números exactos hemos caído
    int indexA = (int)tableIndex;
    int indexB = indexA + 1;
    if (indexB >= LFO_TABLE_SIZE) indexB = 0; // Por seguridad

    float frac = tableIndex - (float)indexA; // El decimal (ej: si es 500.2, frac es 0.2)

    // 3. Interpolación Lineal (Mezclamos los dos números para una calidad de audio de estudio)
    float lfo2Output = lfo2Table[indexA] + frac * (lfo2Table[indexB] - lfo2Table[indexA]);

    // Lo mandamos al escaparate público
    globalLfo2Value = lfo2Output;

    // --- GENERAMOS LA FORMA DE ONDA DEL LFO 1 ---
    int waveType = (int)apvts.getRawParameterValue("LFO1_WAVE")->load();
    float lfo1Output = 0.0f;

    if (waveType == 0)      lfo1Output = std::sin(lfo1Phase * juce::MathConstants<float>::twoPi);
    else if (waveType == 1) lfo1Output = 2.0f * std::abs(2.0f * lfo1Phase - 1.0f) - 1.0f; // Triángulo
    else if (waveType == 2) lfo1Output = 1.0f - 2.0f * lfo1Phase; // Sierra invertida
    else if (waveType == 3) lfo1Output = (lfo1Phase < 0.5f) ? 1.0f : -1.0f; // Cuadrada
    else if (waveType == 4) { // Sample & Hold (Ruido escalonado)
        // Solo cambia de valor cuando la fase se reinicia
        static float lastRand = 0.0f;
        static float lastPhase = 0.0f;
        if (lfo1Phase < lastPhase) {
            lastRand = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
        }
        lastPhase = lfo1Phase;
        lfo1Output = lastRand;
    }

    // Aplicamos el Jitter (Ruido orgánico) al LFO 1
    float jitter1 = apvts.getRawParameterValue("LFO1_JITTER")->load();
    if (jitter1 > 0.0f) {
        float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f) - 1.0f;
        lfo1Output += noise * jitter1 * 0.3f; // 0.3f para que el ruido no sea ensordecedor
        lfo1Output = juce::jlimit(-1.0f, 1.0f, lfo1Output);
    }

    // Aplicamos la amplitud y lo mandamos al escaparate público
    float amp1 = apvts.getRawParameterValue("LFO1_DEPTH")->load();
    globalLfo1Value = lfo1Output * amp1;

    // --- NUEVO: Enviamos el LFO 1 y LFO 2 a las Voces ---
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<GranularVoice*>(synth.getVoice(i))) {
            voice->currentLfo1Value = globalLfo1Value; 
            voice->currentLfo2Value = globalLfo2Value;
        }
    }

    // (El cálculo del LFO 2 lo haremos en el siguiente paso, requiere leer los vectores Bézier)

    // ==========================================================
    // --- 2. PROCESAMIENTO DE AUDIO NORMAL ---
    // ==========================================================
    // 1. Limpiamos los altavoces por si había ruido viejo
    buffer.clear();

    // 2. ¡Dejamos que el Director de Orquesta (el Sintetizador) se encargue de todo!
    // Él leerá el MIDI, verá qué teclas has pulsado, y le dirá a las Voces que rellenen el buffer de audio.
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // ==========================================================
    // 2. APLICAMOS LA REVERB GLOBAL (ESTILO IMMERSIVE/SHIMMER)
    // ==========================================================
    float size = apvts.getRawParameterValue("SPACE_SIZE")->load();
    float fback = apvts.getRawParameterValue("SPACE_FBACK")->load();
    float mix = apvts.getRawParameterValue("SPACE_MIX")->load();

    // Configuramos los parámetros para un sonido "Lush" y abierto
    reverbParams.roomSize = size;
    // Invertimos fback: Si está a tope (1.0), el damping es 0 (brillante e infinito)
    reverbParams.damping = 1.0f - fback;
    reverbParams.width = 1.0f; // Estéreo abierto al máximo (inmersión total)
    reverbParams.freezeMode = 0.0f;

    // EL TRUCO DE POTENCIA CONSTANTE (Para evitar subidas de volumen extrañas)
    // Usamos seno y coseno para cruzar el volumen seco y mojado de forma perfecta
    reverbParams.dryLevel = std::cos(mix * juce::MathConstants<float>::halfPi);
    reverbParams.wetLevel = std::sin(mix * juce::MathConstants<float>::halfPi);

    masterReverb.setParameters(reverbParams);

    // Pasamos el audio del buffer por el motor de Reverb
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    masterReverb.process(context);
}

//==============================================================================
bool Granular_SynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Granular_SynthAudioProcessor::createEditor()
{
    return new Granular_SynthAudioProcessorEditor (*this);
}

//==============================================================================
void Granular_SynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Granular_SynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Granular_SynthAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout Granular_SynthAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. POSITION
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "POSITION", "Position", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // 2. GRAIN SIZE (De 0.0 a 1.0 del archivo)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_SIZE", "Grain Size", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.3f), 0.1f));

    // 3. SCAN SPEED
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SCAN_SPEED", "Scan Speed", juce::NormalisableRange<float>(-2.0f, 2.0f, 0.01f), 0.0f));

    // 4. SPRAY POSITION
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SPRAY_POS", "Position Spray", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    // 5. DENSITY
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DENSITY", "Density", juce::NormalisableRange<float>(1.0f, 120.0f, 0.1f, 0.5f), 20.0f));

    // 6. SHAPE (¡ESTE ES EL QUE FALTABA!)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SHAPE", "Shape", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // SPRAY PAN: 0.0 (Todo al centro) a 1.0 (Repartido por todo el estéreo)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SPRAY_PAN", "Pan Spray", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SPRAY_PITCH", "Pitch Spray", juce::NormalisableRange<float>(0.0f, 12.0f, 0.01f), 0.0f));

    // SCAN MODE (Dir): 0 = Forward, 1 = Reverse, 2 = Ping-Pong
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SCAN_MODE", "Scan Mode", juce::NormalisableRange<float>(0.0f, 2.0f, 1.0f), 0.0f));

    // --- PITCH MODULE ---
    // Transpose: de -24 a +24 semitonos (usamos pasos de 1 para que sean notas exactas)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "PITCH_TRANS", "Transpose", juce::NormalisableRange<float>(-24.0f, 24.0f, 1.0f), 0.0f));

    // Fine: de -1.0 a +1.0 (representa -100 a +100 cents)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "PITCH_FINE", "Fine Tune", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    // Scale: Modos de cuantización (0 = Libre, 1 = Octavas, 2 = Quintas, 3 = Pentatónica)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "PITCH_SCALE", "Scale Mode", juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f), 0.0f));

    // --- FILTER MODULE ---
    // LPF Freq: Empieza abierto del todo (20000 Hz) para dejar pasar el sonido
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_LPF", "LPF Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));

    // 
   // params.push_back(std::make_unique<juce::AudioParameterFloat>(
        //"FILTER_RES", "Resonance", juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 0.707f));

    // Resonance: De 0.707 (Plano/Neutro) a 2.5 (Pico)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_RES", "Resonance", juce::NormalisableRange<float>(0.707f, 2.5f, 0.01f), 0.707f));

    // HPF Freq: Empieza cerrado abajo (20 Hz) para no cortar los graves al principio
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FILTER_HPF", "HPF Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 20.0f));

    // --- SPACE MODULE (REVERB) ---
    // Size: Tamaño de la sala (0.0 = armario, 1.0 = catedral infinita)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPACE_SIZE", "Size", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

    // Fback (Damping/Absorción): Define si la cola es oscura y cálida (0.0) o brillante y metálica (1.0)
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPACE_FBACK", "Feedback", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // Mix: 0.0 = 100% Seco, 1.0 = 100% Mojado
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SPACE_MIX", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    // --- AMP ENVELOPE (Tiempos en segundos, Sustain de 0 a 1) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("AMP_A", "Amp Attack", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("AMP_D", "Amp Decay", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("AMP_S", "Amp Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("AMP_R", "Amp Release", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 1.0f));

    // --- ENVELOPE 2 (Para la futura Matriz de Modulación) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_A", "Env2 Attack", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_D", "Env2 Decay", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_S", "Env2 Sustain", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ENV2_R", "Env2 Release", juce::NormalisableRange<float>(0.01f, 5.0f, 0.01f, 0.3f), 1.0f));

    // ==============================================================================
    // --- LFO 1 (GLOBAL ---
    // ==============================================================================

    // Subdivisiones Musicales (El reloj principal por ahora)
    juce::StringArray beatDivisions = { "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "LFO1_BEAT", "Rate", beatDivisions, 5)); // El 5 es "1/4" por defecto

    // Forma de Onda
    juce::StringArray waveShapes = { "Sine", "Triangle", "Saw", "Square", "S&H" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "LFO1_WAVE", "Waveform", waveShapes, 0)); // El 0 es Sine

    // Amplitude (Depth) - Controla el tamaño de la onda
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "LFO1_DEPTH", "Amp", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    // Jitter 
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "LFO1_JITTER", "Jitter", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // --- LFO 2 (VECTOR) ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "LFO2_BEAT", "LFO 2 Rate", beatDivisions, 5)); // El 5 es "1/4" por defecto

    return { params.begin(), params.end() };
}

void Granular_SynthAudioProcessor::loadFile(const juce::String& path)
{
    // 1. Convertimos la ruta de texto en un "Archivo" real que JUCE entienda
    juce::File file(path);

    // 2. Le pedimos a nuestro manager que intente crear un "lector" para este archivo
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    // 3. Si el lector se ha creado con éxito (es decir, si el archivo era un wav, mp3, flac válido)
    if (reader != nullptr)
    {
        // Preparamos nuestro "disco duro" (audioBuffer) con el número de canales y la longitud del audio
        audioBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);

        // Volcamos toda la información del lector dentro de nuestro audioBuffer
        reader->read(&audioBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

        // Mensaje interno para que sepamos que todo ha ido bien (esto no lo ve el usuario)
        DBG("Archivo cargado en la memoria: " + file.getFileName());
    }
}
