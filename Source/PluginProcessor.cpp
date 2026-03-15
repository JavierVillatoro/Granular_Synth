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
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
    formatManager.registerBasicFormats();
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

void Granular_SynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
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
    // Vector  para guardar parámetros temporalmente
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 1. GRAIN SIZE (Tamaño del grano en milisegundos: de 1ms a 500ms)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "GRAIN_SIZE", "Grain Size", juce::NormalisableRange<float>(1.0f, 500.0f, 1.0f, 0.5f), 50.0f));

    // 2. SPRAY (Aleatoriedad de posición: de 0 a 1)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SPRAY", "Spray", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // 3. SPIKE (Afilado de la envolvente: de 0 a 1)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SPIKE", "Spike", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // Devolvemos la lista completa al APVTS
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
