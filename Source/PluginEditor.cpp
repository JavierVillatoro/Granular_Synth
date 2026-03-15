/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Granular_SynthAudioProcessorEditor::Granular_SynthAudioProcessorEditor(Granular_SynthAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    thumbnailCache(5),
    thumbnail(512, p.getFormatManager(), thumbnailCache)
{
    thumbnail.addChangeListener(this);
    setSize(1200, 750);

    // --- NUESTRA NUEVA ARQUITECTURA MODULAR ---

    // 1. Hacemos visible nuestro nuevo módulo independiente
    addAndMakeVisible(scanModule);
    addAndMakeVisible(engineModule);

    // 2. Le decimos a esta pantalla principal que "escuche" si el parámetro POSITION cambia
    // (para que sepa cuándo tiene que mover la línea blanca)
    audioProcessor.apvts.addParameterListener("POSITION", this);
    audioProcessor.apvts.addParameterListener("GRAIN_SIZE", this);
}

Granular_SynthAudioProcessorEditor::~Granular_SynthAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener("POSITION", this);
    audioProcessor.apvts.removeParameterListener("GRAIN_SIZE", this);
    thumbnail.removeChangeListener(this);
}

//==============================================================================
void Granular_SynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // 1. PINTAMOS EL FONDO
    g.fillAll(juce::Colour(0xff121212));

    // 2. DEFINIMOS NUESTRAS GRANDES ÁREAS
    auto bounds = getLocalBounds();
    auto bottomModulesArea = bounds.removeFromBottom(300);
    auto rightMixerArea = bounds.removeFromRight(350);
    auto wavesArea = bounds;

    // 3. DIBUJAMOS LA ZONA DE LAS 4 CAPAS (Ondas)
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRect(wavesArea, 1);

    int layerHeight = wavesArea.getHeight() / 4;
    auto layer1Area = wavesArea.removeFromTop(layerHeight);
    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.drawRect(layer1Area, 2);

    if (thumbnail.getNumChannels() > 0)
    {
        g.setColour(juce::Colours::cyan);
        thumbnail.drawChannel(g, layer1Area.reduced(2), 0.0, thumbnail.getTotalLength(), 0, 1.0f);

        auto positionParam = audioProcessor.apvts.getRawParameterValue("POSITION");
        auto grainSizeParam = audioProcessor.apvts.getRawParameterValue("GRAIN_SIZE");

        if (positionParam != nullptr && grainSizeParam != nullptr)
        {
            float currentPosition = positionParam->load();
            float grainSizeSeconds = grainSizeParam->load();
            double totalAudioSeconds = thumbnail.getTotalLength();
            float cursorX = layer1Area.getX() + (currentPosition * layer1Area.getWidth());
            float grainWidthPixels = (grainSizeSeconds / totalAudioSeconds) * layer1Area.getWidth();
            grainWidthPixels = juce::jmax(3.0f, grainWidthPixels);

            juce::Rectangle<float> grainWindow(cursorX - (grainWidthPixels / 2.0f),
                layer1Area.getY(),
                grainWidthPixels,
                layer1Area.getHeight());

            g.setColour(juce::Colours::cyan.withAlpha(0.3f));
            g.fillRect(grainWindow);

            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawLine(cursorX, layer1Area.getY(), cursorX, layer1Area.getBottom(), 3.0f);
        }
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(20.0f);
        g.drawText("Capa 1: Arrastra Audio", layer1Area, juce::Justification::centred, false);
    }

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

    // ==============================================================================
    // 5. NUEVO: DIBUJAMOS LOS 8 MÓDULOS CON NOMBRES Y ETIQUETAS
    // ==============================================================================
    std::vector<juce::StringArray> knobNames = {
        juce::StringArray {"Size", "Density", "Shape"},    // Engine
        juce::StringArray {"Pos", "Speed", "Dir"},         // Scan
        juce::StringArray {"Pos", "Pitch", "Pan"},         // Spray
        juce::StringArray {"Trans", "Fine", "Scale"},      // Pitch
        juce::StringArray {"Rate", "Depth", "Wave"},       // LFO
        juce::StringArray {"A", "D", "S", "R"},            // Envelope
        juce::StringArray {"Freq", "Res", "Type"},         // Filter
        juce::StringArray {"Size", "Fback", "Mix"}         // Space
    };

    juce::StringArray moduleNames = { "ENGINE", "SCAN", "SPRAY", "PITCH", "LFO", "ENVELOPE", "FILTER", "SPACE" };

    int numColumns = 4;
    int numRows = 2;
    int moduleWidth = bottomModulesArea.getWidth() / numColumns;
    int moduleHeight = bottomModulesArea.getHeight() / numRows;

    int modIndex = 0;
    for (int row = 0; row < numRows; ++row)
    {
        for (int col = 0; col < numColumns; ++col)
        {
            // Creamos el rectángulo del módulo
            juce::Rectangle<int> moduleRect(bottomModulesArea.getX() + (col * moduleWidth),
                bottomModulesArea.getY() + (row * moduleHeight),
                moduleWidth, moduleHeight);

            // Dibujamos el borde cyan sutil
            g.setColour(juce::Colours::cyan.withAlpha(0.2f));
            g.drawRect(moduleRect, 1);

            // --- TÍTULO DEL MÓDULO (Arriba) ---
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(juce::Font(16.0f, juce::Font::bold));
            // removeFromTop nos devuelve el trozo de arriba y "encoge" el resto del rectángulo
            g.drawText(moduleNames[modIndex], moduleRect.removeFromTop(30), juce::Justification::centred);

            // --- ETIQUETAS DE LOS KNOBS (Abajo) ---
            auto labelArea = moduleRect.removeFromBottom(25);
            int numKnobs = knobNames[modIndex].size();
            int labelWidth = labelArea.getWidth() / numKnobs;

            g.setFont(12.0f);
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            for (int i = 0; i < numKnobs; ++i)
            {
                g.drawText(knobNames[modIndex][i], labelArea.removeFromLeft(labelWidth), juce::Justification::centred);
            }

            modIndex++;
        }
    }
}

void Granular_SynthAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto bottomModulesArea = bounds.removeFromBottom(300); // La zona de los módulos

    int moduleWidth = bottomModulesArea.getWidth() / 4;
    int moduleHeight = bottomModulesArea.getHeight() / 2;

    // Calculamos el rectángulo de la Casilla 1 (Fila 0, Columna 0)
    juce::Rectangle<int> module1Rect(bottomModulesArea.getX(), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    engineModule.setBounds(module1Rect);

    // Calculamos el cuadrado exacto del Módulo 2 (Fila 0, Columna 1)
    juce::Rectangle<int> module2Rect(bottomModulesArea.getX() + moduleWidth,
        bottomModulesArea.getY(),
        moduleWidth,
        moduleHeight);

    // Colocamos nuestra rueda de Posición en el centro de ese módulo, con un poco de margen (reduced)
    //positionKnob.setBounds(module2Rect.reduced(20));
    scanModule.setBounds(module2Rect);
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

