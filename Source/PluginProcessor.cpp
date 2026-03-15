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
{
    // 1. Limpiamos los altavoces por si había ruido viejo
    buffer.clear();

    // 2. ¡Dejamos que el Director de Orquesta (el Sintetizador) se encargue de todo!
    // Él leerá el MIDI, verá qué teclas has pulsado, y le dirá a las Voces que rellenen el buffer de audio.
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
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

    // 1. SOLO UN PARÁMETRO DE POSITION
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "POSITION", "Position", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    // 2. SOLO UN PARÁMETRO DE GRAIN SIZE
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_SIZE", "Grain Size", juce::NormalisableRange<float>(0.01f, 2.0f, 0.001f), 0.1f)); //Cambiar rango de grain size a 2 sec

    // SCAN SPEED: -2.0 a 2.0 (negativo es reverse, 0 es parado, 1 es velocidad real)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SCAN_SPEED", "Scan Speed", juce::NormalisableRange<float>(-2.0f, 2.0f, 0.01f), 0.0f));

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
