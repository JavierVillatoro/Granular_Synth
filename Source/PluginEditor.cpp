/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GranularVoice.h"

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
    addAndMakeVisible(sprayModule);
    addAndMakeVisible(pitchModule);
    addAndMakeVisible(filterModule);

    // 2. Le decimos a esta pantalla principal que "escuche" si el parámetro POSITION cambia
    // (para que sepa cuándo tiene que mover la línea blanca)
    audioProcessor.apvts.addParameterListener("POSITION", this);
    audioProcessor.apvts.addParameterListener("GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("SHAPE", this);

    startTimerHz(30);
}

Granular_SynthAudioProcessorEditor::~Granular_SynthAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener("POSITION", this);
    audioProcessor.apvts.removeParameterListener("GRAIN_SIZE", this);
    audioProcessor.apvts.removeParameterListener("SHAPE", this);
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
        double totalAudioSeconds = thumbnail.getTotalLength();
        double visibleSeconds = totalAudioSeconds / zoomFactor;
        double startTime = viewStartRatio * totalAudioSeconds;
        double endTime = startTime + visibleSeconds;

        g.setColour(juce::Colours::cyan);
        thumbnail.drawChannel(g, layer1Area.reduced(2), startTime, endTime, 0, 1.0f);

        // --- DIBUJAR CURSOR Y FORMA DEL GRANO ---
        auto positionParam = audioProcessor.apvts.getRawParameterValue("POSITION");
        auto grainSizeParam = audioProcessor.apvts.getRawParameterValue("GRAIN_SIZE");
        auto shapeParamVal = audioProcessor.apvts.getRawParameterValue("SHAPE");

        if (positionParam != nullptr && grainSizeParam != nullptr && shapeParamVal != nullptr)
        {
            float currentPosition = positionParam->load();
            float sizeRatio = grainSizeParam->load();
            float shapeValue = shapeParamVal->load(); // Leemos el Shape actual

            double cursorTimeSeconds = currentPosition * totalAudioSeconds;
            float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * (float)totalAudioSeconds);
            float cursorX = layer1Area.getX() + ((cursorTimeSeconds - startTime) / visibleSeconds) * layer1Area.getWidth();

            float grainWidthPixels = (grainSizeSeconds / visibleSeconds) * layer1Area.getWidth();
            grainWidthPixels = juce::jmax(3.0f, grainWidthPixels);

            // Definimos el contenedor del grano
            juce::Rectangle<float> grainWindow(cursorX - (grainWidthPixels / 2.0f),
                layer1Area.getY(),
                grainWidthPixels,
                layer1Area.getHeight());

            // --- AQUÍ EMPIEZA EL DIBUJO DINÁMICO DEL SHAPE ---
            juce::Path grainPath;
            grainPath.startNewSubPath(grainWindow.getX(), grainWindow.getBottom());

            for (float x = 0; x <= grainWindow.getWidth(); x += 1.0f)
            {
                float progress = x / grainWindow.getWidth();

                // Hann (Campana)
                float hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * progress));
                // Square (Trapecio con mini-fade de 5% para que no sea un bloque puro)
                float square = (progress < 0.05f) ? progress / 0.05f : (progress > 0.95f ? (1.0f - progress) / 0.05f : 1.0f);

                float amplitude = (hann * (1.0f - shapeValue)) + (square * shapeValue);
                float yPos = grainWindow.getBottom() - (amplitude * grainWindow.getHeight());

                grainPath.lineTo(grainWindow.getX() + x, yPos);
            }

            grainPath.lineTo(grainWindow.getRight(), grainWindow.getBottom());
            grainPath.closeSubPath();

            g.setColour(juce::Colours::cyan.withAlpha(0.3f));
            g.fillPath(grainPath);
            g.setColour(juce::Colours::cyan.withAlpha(0.8f));
            g.strokePath(grainPath, juce::PathStrokeType(1.5f));
            // --------------------------------------------------

            // Dibujamos la línea central blanca
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawLine(cursorX, layer1Area.getY(), cursorX, layer1Area.getBottom(), 2.0f);
        }

        // ==========================================================
        // --- MAGIA VISUAL DE LOS GRANOS ANIMADOS ---
        // ==========================================================
        auto& synth = audioProcessor.getSynthesiser();

        // Recorremos todas las voces (por si tocas acordes)
        for (int i = 0; i < synth.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<GranularVoice*>(synth.getVoice(i)))
            {
                for (int g_idx = 0; g_idx < 128; ++g_idx)
                {
                    float env = voice->visualGrainEnv[g_idx].load();

                    if (env > 0.001f)
                    {
                        float pos = voice->visualGrainPos[g_idx].load();

                        // Calculamos el tiempo exacto del grano para aplicar el Zoom y Scroll actual
                        double grainTimeSeconds = pos * totalAudioSeconds;
                        float xPixel = layer1Area.getX() + ((grainTimeSeconds - startTime) / visibleSeconds) * layer1Area.getWidth();

                        // Solo lo dibujamos si cae dentro de la pantalla visible
                        if (xPixel >= layer1Area.getX() && xPixel <= layer1Area.getRight())
                        {
                            float maxLineHeight = layer1Area.getHeight() * 0.8f;
                            float currentHeight = maxLineHeight * env;

                            float yCenter = layer1Area.getCentreY();
                            float yStart = yCenter - (currentHeight / 2.0f);

                            g.setColour(juce::Colours::white.withAlpha(env * 0.8f));
                            g.drawLine(xPixel, yStart, xPixel, yStart + currentHeight, 1.5f + (env * 1.5f));
                        }
                    }
                }
            }
        }
        // ==========================================================
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
        juce::StringArray {"LPF", "Res", "HPF"},           // Filter
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
            juce::Rectangle<int> moduleRect(bottomModulesArea.getX() + (col * moduleWidth),
                bottomModulesArea.getY() + (row * moduleHeight),
                moduleWidth, moduleHeight);

            g.setColour(juce::Colours::cyan.withAlpha(0.2f));
            g.drawRect(moduleRect, 1);

            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(juce::Font(16.0f, juce::Font::bold));
            g.drawText(moduleNames[modIndex], moduleRect.removeFromTop(30), juce::Justification::centred);

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

    // Casilla 1 (Fila 0, Columna 0)
    juce::Rectangle<int> module1Rect(bottomModulesArea.getX(), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    engineModule.setBounds(module1Rect);

    // Módulo 2 (Fila 0, Columna 1)
    juce::Rectangle<int> module2Rect(bottomModulesArea.getX() + moduleWidth,
        bottomModulesArea.getY(),
        moduleWidth,
        moduleHeight);
    
    // SPRAY  (Columna 2)
    juce::Rectangle<int> module3Rect(bottomModulesArea.getX() + (moduleWidth * 2), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    sprayModule.setBounds(module3Rect);

    
    //positionKnob.setBounds(module2Rect.reduced(20));
    scanModule.setBounds(module2Rect);
    
    // PITCH (Fila 0, Columna 3) ---
    juce::Rectangle<int> module4Rect(bottomModulesArea.getX() + (moduleWidth * 3), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    pitchModule.setBounds(module4Rect);

    // FILTER (Fila 1, Columna 2)
    juce::Rectangle<int> filterRect(bottomModulesArea.getX() + (moduleWidth * 2),
        bottomModulesArea.getY() + moduleHeight, // + moduleHeight nos baja a la segunda fila
        moduleWidth,
        moduleHeight);
    filterModule.setBounds(filterRect);
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

// ==============================================================================
// --- CONTROL DE RATÓN UNIFICADO: ZOOM Y CURSOR ---

void Granular_SynthAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (thumbnail.getTotalLength() > 0.0)
    {
        // 1. Recreamos el área de la onda para saber si el ratón está dentro
        auto bounds = getLocalBounds();
        bounds.removeFromBottom(300);
        bounds.removeFromRight(350);
        auto layer1Area = bounds.removeFromTop(bounds.getHeight() / 4);

        if (layer1Area.contains(event.getPosition()))
        {
            // 2. ¿En qué píxel relativo de la caja azul está el ratón?
            double mouseX = event.getPosition().x - layer1Area.getX();

            // ¿Qué porcentaje de la pantalla (0.0 a 1.0) es ese píxel?
            double mouseRatioInView = mouseX / (double)layer1Area.getWidth();

            // 3. MAGIA: ¿Qué porcentaje del audio TOTAL está debajo del ratón AHORA MISMO?
            double timeUnderMouse = viewStartRatio + (mouseRatioInView / zoomFactor);

            // 4. Calculamos el nuevo nivel de Zoom
            double zoomMultiplier = (wheel.deltaY > 0) ? 1.2 : 1.0 / 1.2;
            double newZoomFactor = juce::jlimit(1.0, 50.0, zoomFactor * zoomMultiplier);

            // 5. Ajustamos el inicio de la vista para que 'timeUnderMouse' siga bajo el ratón
            viewStartRatio = timeUnderMouse - (mouseRatioInView / newZoomFactor);

            // 6. Limitamos para no salirnos por los bordes izquierdo (0.0) o derecho
            viewStartRatio = juce::jlimit(0.0, 1.0 - (1.0 / newZoomFactor), viewStartRatio);

            // Aplicamos el zoom definitivo y redibujamos
            zoomFactor = newZoomFactor;
            repaint();
        }
    }
}

void Granular_SynthAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);
    auto layer1Area = bounds.removeFromTop(bounds.getHeight() / 4);

    if (layer1Area.contains(event.getPosition()))
    {
        // 1. ¿En qué pixel de la pantalla hemos hecho clic?
        float clickX = event.getPosition().x - layer1Area.getX();

        // 2. ¿Qué porcentaje de la pantalla representa ese pixel? (De 0.0 a 1.0)
        float ratioInView = clickX / (float)layer1Area.getWidth();

        // 3. MAGIA: Convertimos ese clic en pantalla a la posición real del audio,
        // teniendo en cuenta el nivel de Zoom y el desplazamiento actual.
        float visibleRatio = 1.0f / zoomFactor;
        float normalizedPos = viewStartRatio + (ratioInView * visibleRatio);

        normalizedPos = juce::jlimit(0.0f, 1.0f, normalizedPos);

        // 4. Actualizamos el knob y el motor de audio
        if (auto* posParam = audioProcessor.apvts.getParameter("POSITION"))
        {
            posParam->setValueNotifyingHost(normalizedPos);
        }
    }
}

void Granular_SynthAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    // Al arrastrar, llamamos exactamente a la misma lógica que al hacer clic
    mouseDown(event);
}

void Granular_SynthAudioProcessorEditor::timerCallback()
{
    // Esto le dice a JUCE: "Oye, borra la pantalla y vuelve a llamar a paint()"
    repaint();
}

