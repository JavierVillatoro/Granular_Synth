/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Granular_SynthAudioProcessorEditor::Granular_SynthAudioProcessorEditor (Granular_SynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      thumbnailCache(5), // Guardamos hasta 5 audios en la memoria gráfica
      thumbnail(512, p.getFormatManager(), thumbnailCache) // 512 píxeles de resolución
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    thumbnail.addChangeListener(this);
    setSize (1200, 750);
}

Granular_SynthAudioProcessorEditor::~Granular_SynthAudioProcessorEditor()
{
}

//==============================================================================
void Granular_SynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // 1. PINTAMOS EL FONDO
    g.fillAll(juce::Colour(0xff121212));

    // 2. DEFINIMOS NUESTRAS GRANDES ÁREAS
    auto bounds = getLocalBounds();

    // Cortamos la parte de abajo para los 8 módulos (dejamos 300px de alto)
    auto bottomModulesArea = bounds.removeFromBottom(300);

    // De lo que queda arriba, cortamos la parte derecha para el Mixer / Envelopes (350px de ancho)
    auto rightMixerArea = bounds.removeFromRight(350);

    // Lo que queda es el bloque superior izquierdo: aquí irán las 4 capas de audio
    auto wavesArea = bounds;

    // 3. DIBUJAMOS LA ZONA DE LAS 4 CAPAS (Ondas)
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRect(wavesArea, 1);

    // Dividimos mentalmente esa zona en 4 tiras horizontales para el futuro
    int layerHeight = wavesArea.getHeight() / 4;

    // Solo dibujamos la Capa 1 por ahora (donde cargamos el audio)
    auto layer1Area = wavesArea.removeFromTop(layerHeight);
    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.drawRect(layer1Area, 2); // Resaltamos la Capa 1 que es la que estamos construyendo

    // Si SÍ hay audio, lo dibujamos en la tira de la Capa 1
    if (thumbnail.getNumChannels() > 0)
    {
        g.setColour(juce::Colours::cyan);
        thumbnail.drawChannel(g, layer1Area.reduced(2), 0.0, thumbnail.getTotalLength(), 0, 1.0f);
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(20.0f);
        g.drawText("Capa 1: Arrastra Audio", layer1Area, juce::Justification::centred, false);
    }

    // Dibujamos las casillas vacías para las futuras Capas 2, 3 y 4
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (int i = 0; i < 3; ++i)
    {
        g.drawRect(wavesArea.removeFromTop(layerHeight), 1);
    }

    // 4. DIBUJAMOS LA ZONA DEL MIXER / ENVELOPES (Derecha)
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(rightMixerArea);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText("ADSR & LAYER MIXER", rightMixerArea, juce::Justification::centred, false);

    // 5. DIBUJAMOS LOS 8 MÓDULOS INFERIORES
    g.setColour(juce::Colours::cyan.withAlpha(0.2f));
    int numColumns = 4;
    int numRows = 2;
    int moduleWidth = bottomModulesArea.getWidth() / numColumns;
    int moduleHeight = bottomModulesArea.getHeight() / numRows;

    for (int row = 0; row < numRows; ++row)
    {
        for (int col = 0; col < numColumns; ++col)
        {
            juce::Rectangle<int> moduleRect(bottomModulesArea.getX() + (col * moduleWidth),
                bottomModulesArea.getY() + (row * moduleHeight),
                moduleWidth,
                moduleHeight);
            g.drawRect(moduleRect, 1);
        }
    }
}

void Granular_SynthAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

// ==============================================================================
// --- FUNCIONES DE DRAG & DROP ---

// 1. Decide si el archivo que arrastras nos sirve
bool Granular_SynthAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files)
    {
        // containsIgnoreCase le da igual si pone .WAV, .wav o .WaV
        if (file.containsIgnoreCase(".wav") || file.containsIgnoreCase(".mp3") ||
            file.containsIgnoreCase(".aif") || file.containsIgnoreCase(".flac") ||
            file.containsIgnoreCase(".aiff"))
        {
            return true;
        }
    }
    return false;
}

// 2. ¿Qué pasa cuando finalmente sueltas el clic del ratón?
void Granular_SynthAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    // Nos aseguramos de que no esté vacío
    if (files.isEmpty())
        return;

    // Nos quedamos con el primer archivo que hayas soltado (su ruta en el disco duro)
    juce::String filePath = files[0];

    // TODO: ¡Aquí le enviaremos esta ruta al Processor para que la lea y dibuje la forma de onda!
    audioProcessor.loadFile(filePath);

    thumbnail.setSource(new juce::FileInputSource(juce::File(filePath)));
}

//==============================================================================

void Granular_SynthAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    repaint(); // Audio listo, actualiza pantalla
}

