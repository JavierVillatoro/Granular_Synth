/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GranularVoice.h"
#include "EnvelopeModule.h"

//==============================================================================
Granular_SynthAudioProcessorEditor::Granular_SynthAudioProcessorEditor(Granular_SynthAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    masterModule(p.apvts, p),
    //distModule(p.apvts),
    bpmModule(p.apvts), // <-- 1. Inicializamos el nuevo módulo aquí (CON coma)
    mixerModule1(p.apvts, "L1_"),
    layer1Controls(p.apvts, "L1_"),
    layer2Controls(p.apvts, "L2_"),
    thumbnailCache(5),
    thumbnail(512, p.getFormatManager(), thumbnailCache),
    thumbnailL2(512, p.getFormatManager(), thumbnailCache)
{
    thumbnail.addChangeListener(this);
    thumbnailL2.addChangeListener(this);
    setSize(1200, 750);

    // --- NUESTRA NUEVA ARQUITECTURA MODULAR ---

    // 1. Hacemos visible nuestro nuevo módulo independiente
    addAndMakeVisible(scanModule);
    addAndMakeVisible(engineModule);
    addAndMakeVisible(sprayModule);
    addAndMakeVisible(pitchModule);
    addAndMakeVisible(filterModule);
    addAndMakeVisible(envelopeModule);
    addAndMakeVisible(spaceModule);
    addAndMakeVisible(lfoModule);
    addAndMakeVisible(masterModule);
    addAndMakeVisible(distModule);
    addAndMakeVisible(bpmModule);
    addAndMakeVisible(layer1Controls);
    addAndMakeVisible(layer2Controls);
    addAndMakeVisible(mixerModule1);

    // 2. Le decimos a esta pantalla principal que "escuche" si el parámetro POSITION cambia
    // (para que sepa cuándo tiene que mover la línea blanca)
    audioProcessor.apvts.addParameterListener("L1_POSITION", this);
    audioProcessor.apvts.addParameterListener("L1_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L1_SHAPE", this);

    // Escuchas Capa 2 (NUEVO)
    audioProcessor.apvts.addParameterListener("L2_POSITION", this);
    audioProcessor.apvts.addParameterListener("L2_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L2_SHAPE", this);

    startTimerHz(30);

    // ==========================================================
    // --- CURA PARA LA AMNESIA DE LA ONDA DE AUDIO ---
    // ==========================================================
    // Le preguntamos al Motor si ya tenía un audio cargado de antes
    if (audioProcessor.isAudioLoadedL1 && audioProcessor.lastLoadedFilePathL1.isNotEmpty())
    {
        thumbnail.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL1)));
    }
    if (audioProcessor.isAudioLoadedL2 && audioProcessor.lastLoadedFilePathL2.isNotEmpty()) {
        thumbnailL2.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL2)));
    }
}

Granular_SynthAudioProcessorEditor::~Granular_SynthAudioProcessorEditor()
{
    // --- Limpieza Capa 1 ---
    audioProcessor.apvts.removeParameterListener("L1_POSITION", this);
    audioProcessor.apvts.removeParameterListener("L1_GRAIN_SIZE", this);
    audioProcessor.apvts.removeParameterListener("L1_SHAPE", this);
    thumbnail.removeChangeListener(this);

    // --- Limpieza Capa 2 (NUEVO) ---
    audioProcessor.apvts.removeParameterListener("L2_POSITION", this);
    audioProcessor.apvts.removeParameterListener("L2_GRAIN_SIZE", this);
    audioProcessor.apvts.removeParameterListener("L2_SHAPE", this);
    thumbnailL2.removeChangeListener(this);
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
        auto positionParam = audioProcessor.apvts.getRawParameterValue("L1_POSITION");
        auto grainSizeParam = audioProcessor.apvts.getRawParameterValue("L1_GRAIN_SIZE");
        auto shapeParamVal = audioProcessor.apvts.getRawParameterValue("L1_SHAPE");

        if (positionParam != nullptr && grainSizeParam != nullptr && shapeParamVal != nullptr)
        {
            float currentPosition = positionParam->load();
            float sizeRatio = grainSizeParam->load();
            float shapeValue = shapeParamVal->load(); // Leemos el Shape actual

            float winStart = audioProcessor.windowStartRatio.load();
            float winLen = audioProcessor.windowLengthRatio.load();

            // 1. Posición real de la línea en el archivo entero
            float absolutePos = winStart + (currentPosition * winLen);
            double cursorTimeSeconds = absolutePos * totalAudioSeconds;

            // 2. El tamaño ahora es un % del trozo visible, no del archivo entero
            float activeAudioSeconds = (float)totalAudioSeconds * winLen;
            float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * activeAudioSeconds);
            // ==========================================================
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
                //float square = (progress < 0.05f) ? progress / 0.05f : (progress > 0.95f ? (1.0f - progress) / 0.05f : 1.0f);
                float square = (progress < 0.005f) ? progress / 0.005f : (progress > 0.995f ? (1.0f - progress) / 0.005f : 1.0f);
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
        auto& synthL1 = audioProcessor.getSynthesiserL1();

        // Recorremos todas las voces (por si tocas acordes)
        for (int i = 0; i < synthL1.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<GranularVoice*>(synthL1.getVoice(i)))
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
        //g.drawText("Capa 1: Arrastra Audio", layer1Area, juce::Justification::centred, false);
    }
    // ==========================================================
    // --- DIBUJO DE LA CAPA 2 (MAGENTA) ---
    // ==========================================================
    auto layer2Area = wavesArea.removeFromTop(layerHeight); // Cogemos el 2º cuarto de pantalla
    g.setColour(juce::Colours::magenta.withAlpha(0.6f));
    g.drawRect(layer2Area, 2);

    if (thumbnailL2.getNumChannels() > 0)
    {
        double totalAudioSecondsL2 = thumbnailL2.getTotalLength();
        double visibleSecondsL2 = totalAudioSecondsL2 / zoomFactorL2;
        double startTimeL2 = viewStartRatioL2 * totalAudioSecondsL2;
        double endTimeL2 = startTimeL2 + visibleSecondsL2;

        g.setColour(juce::Colours::magenta);
        thumbnailL2.drawChannel(g, layer2Area.reduced(2), startTimeL2, endTimeL2, 0, 1.0f);

        // --- DIBUJAR CURSOR Y FORMA DEL GRANO L2 ---
        auto posParamL2 = audioProcessor.apvts.getRawParameterValue("L2_POSITION");
        auto sizeParamL2 = audioProcessor.apvts.getRawParameterValue("L2_GRAIN_SIZE");
        auto shapeParamL2 = audioProcessor.apvts.getRawParameterValue("L2_SHAPE");

        if (posParamL2 != nullptr && sizeParamL2 != nullptr && shapeParamL2 != nullptr)
        {
            float currentPositionL2 = posParamL2->load();
            float sizeRatioL2 = sizeParamL2->load();
            float shapeValueL2 = shapeParamL2->load();

            float winStartL2 = audioProcessor.windowStartRatio.load(); // Por ahora usamos el zoom global
            float winLenL2 = audioProcessor.windowLengthRatio.load();

            float absolutePosL2 = winStartL2 + (currentPositionL2 * winLenL2);
            double cursorTimeSecondsL2 = absolutePosL2 * totalAudioSecondsL2;

            float activeAudioSecondsL2 = (float)totalAudioSecondsL2 * winLenL2;
            float grainSizeSecondsL2 = juce::jmax(0.01f, sizeRatioL2 * activeAudioSecondsL2);

            float cursorXL2 = layer2Area.getX() + ((cursorTimeSecondsL2 - startTimeL2) / visibleSecondsL2) * layer2Area.getWidth();
            float grainWidthPixelsL2 = (grainSizeSecondsL2 / visibleSecondsL2) * layer2Area.getWidth();
            grainWidthPixelsL2 = juce::jmax(3.0f, grainWidthPixelsL2);

            juce::Rectangle<float> grainWindowL2(cursorXL2 - (grainWidthPixelsL2 / 2.0f),
                layer2Area.getY(), grainWidthPixelsL2, layer2Area.getHeight());

            juce::Path grainPathL2;
            grainPathL2.startNewSubPath(grainWindowL2.getX(), grainWindowL2.getBottom());

            for (float x = 0; x <= grainWindowL2.getWidth(); x += 1.0f) {
                float progress = x / grainWindowL2.getWidth();
                float hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * progress));
                float square = (progress < 0.005f) ? progress / 0.005f : (progress > 0.995f ? (1.0f - progress) / 0.005f : 1.0f);
                float amplitude = (hann * (1.0f - shapeValueL2)) + (square * shapeValueL2);
                float yPos = grainWindowL2.getBottom() - (amplitude * grainWindowL2.getHeight());
                grainPathL2.lineTo(grainWindowL2.getX() + x, yPos);
            }
            grainPathL2.lineTo(grainWindowL2.getRight(), grainWindowL2.getBottom());
            grainPathL2.closeSubPath();

            g.setColour(juce::Colours::magenta.withAlpha(0.3f));
            g.fillPath(grainPathL2);
            g.setColour(juce::Colours::magenta.withAlpha(0.8f));
            g.strokePath(grainPathL2, juce::PathStrokeType(1.5f));

            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawLine(cursorXL2, layer2Area.getY(), cursorXL2, layer2Area.getBottom(), 2.0f);
        }

        // --- MAGIA VISUAL DE LOS GRANOS ANIMADOS L2 ---
        auto& synthL2 = audioProcessor.getSynthesiserL2(); // ¡Llamamos al Jefe 2!
        for (int i = 0; i < synthL2.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<GranularVoice*>(synthL2.getVoice(i)))
            {
                for (int g_idx = 0; g_idx < 128; ++g_idx)
                {
                    float env = voice->visualGrainEnv[g_idx].load();
                    if (env > 0.001f)
                    {
                        float pos = voice->visualGrainPos[g_idx].load();
                        double grainTimeSeconds = pos * totalAudioSecondsL2;
                        float xPixel = layer2Area.getX() + ((grainTimeSeconds - startTimeL2) / visibleSecondsL2) * layer2Area.getWidth();

                        if (xPixel >= layer2Area.getX() && xPixel <= layer2Area.getRight())
                        {
                            float maxLineHeight = layer2Area.getHeight() * 0.8f;
                            float currentHeight = maxLineHeight * env;
                            float yCenter = layer2Area.getCentreY();
                            float yStart = yCenter - (currentHeight / 2.0f);

                            g.setColour(juce::Colours::white.withAlpha(env * 0.8f));
                            g.drawLine(xPixel, yStart, xPixel, yStart + currentHeight, 1.5f + (env * 1.5f));
                        }
                    }
                }
            }
        }
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(20.0f);
        //g.drawText("Capa 2: Arrastra Audio", layer2Area, juce::Justification::centred, false);
    }

    // Dibujamos los dos huecos vacíos que quedan abajo
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (int i = 0; i < 2; ++i) // AHORA SON 2, NO 3
    {
        g.drawRect(wavesArea.removeFromTop(layerHeight), 1);
    }

    // --- RESALTAR LA CAPA ACTIVA ---
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    if (activeLayer == 1) {
        g.drawRect(layer1Area, 2);
    }
    else if (activeLayer == 2) {
        g.drawRect(layer2Area, 2);
    }

    // 4. DIBUJAMOS LA ZONA DEL MIXER / ENVELOPES (Derecha)
    g.setColour(juce::Colour(0xff121212));
    g.fillRect(rightMixerArea); // Mantenemos el fondo oscuro

    // ==========================================================
    // --- DIBUJAMOS EL ESQUELETO FASE 3 Y 4 ---
    // ==========================================================
    // Dibujamos los bordes en color Cyan con un poco de transparencia
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRect(matrixArea, 1);
    g.drawRect(mixerArea, 1);
    g.drawRect(masterArea, 1);
    g.drawRect(distArea, 1);
    g.drawRect(bpmArea, 1);

    // Escribimos los títulos de cada sección para orientarnos
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(14.0f);

    g.drawText("MATRIX", matrixArea, juce::Justification::centred);
    g.drawText("MIXER", mixerArea, juce::Justification::centred);
    //g.drawText("MASTER / LIMIT", masterArea, juce::Justification::centred);
    //g.drawText("DIST", distArea, juce::Justification::centred);
    //g.drawText("BPM", bpmArea, juce::Justification::centred);

    // ==============================================================================
    // 5. NUEVO: DIBUJAMOS LOS 8 MÓDULOS CON NOMBRES Y ETIQUETAS
    // ==============================================================================
    std::vector<juce::StringArray> knobNames = {
        juce::StringArray {"Size", "Density", "Shape"},    // Engine
        juce::StringArray {"Pos", "Speed", "Dir"},         // Scan
        juce::StringArray {"Pos", "Pitch", "Pan"},         // Spray
        juce::StringArray {"Trans", "Fine", "Scale"},      // Pitch
        juce::StringArray {"", "", ""},                    // LFO
        juce::StringArray {"", "", "", ""},                // Envelope
        juce::StringArray {"", "", ""},            // Filter
        juce::StringArray {"Size", "Fback", "Mix"}         // Space
    };

    juce::StringArray moduleNames = { "ENGINE", "SCAN", "SPRAY", "PITCH", "", "", "", "SPACE" };

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
    auto bounds = getLocalBounds();

    // 1. PRIMERO quitamos la zona de abajo (así pilla los 1200px de ancho total)
    auto bottomModulesArea = bounds.removeFromBottom(300);

    // 2. DESPUÉS quitamos el bloque de la derecha del hueco que ha quedado arriba
    auto rightMixerArea = bounds.removeFromRight(350);

    // ==========================================================
    // Lo que sobra en 'bounds' es el rectángulo gigante de las ondas.
    auto wavesArea = bounds;

    int layerHeight = wavesArea.getHeight() / 4;

    // Capa 1 (Cyan)
    auto layer1Area = wavesArea.removeFromTop(layerHeight);
    layer1Controls.setBounds(layer1Area.getX() + 10, layer1Area.getY() + 10, 150, 20);

    // Capa 2 (Magenta) - NUEVO
    auto layer2Area = wavesArea.removeFromTop(layerHeight);
    layer2Controls.setBounds(layer2Area.getX() + 10, layer2Area.getY() + 10, 150, 20);
    // ==========================================================

    // ==========================================================
    // --- FASE 3: ESQUELETO MATRIX / MIXER / MASTER ---
    // ==========================================================
    auto area = rightMixerArea; // Usamos el bloque de 350px que acabas de separar

    // 1. CORTE VERTICAL: Columna Derecha delgada (35% del ancho total de este bloque)
    auto rightColumn = area.removeFromRight(area.getWidth() * 0.35f);

    // Lo que sobra en 'area' es la Columna Izquierda (65% del ancho)
    auto leftColumn = area;

    // 2. CORTES IZQUIERDOS (Grandes)
    matrixArea = leftColumn.removeFromTop(leftColumn.getHeight() * 0.5f); // Mitad arriba
    mixerArea = leftColumn; // El resto (mitad abajo)

    mixerModule1.setBounds(mixerArea);

    // 3. CORTES DERECHOS (Delgados)
    masterArea = rightColumn.removeFromTop(rightColumn.getHeight() * 0.5f); // Mitad arriba
    distArea = rightColumn.removeFromTop(rightColumn.getHeight() * 0.5f);   // 50% de lo que queda
    distModule.setBounds(distArea);
    masterModule.setBounds(masterArea);
    bpmArea = rightColumn; // El resto final (abajo del todo)
    bpmModule.setBounds(bpmArea);

    // ==========================================
    // --- CÁLCULOS DE LA ZONA INFERIOR (TU CÓDIGO INTACTO) ---
    // ==========================================
    int moduleWidth = bottomModulesArea.getWidth() / 4;
    int moduleHeight = bottomModulesArea.getHeight() / 2;

    // --- FILA 0 ---
    juce::Rectangle<int> module1Rect(bottomModulesArea.getX(), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    engineModule.setBounds(module1Rect);

    juce::Rectangle<int> module2Rect(bottomModulesArea.getX() + moduleWidth, bottomModulesArea.getY(), moduleWidth, moduleHeight);
    scanModule.setBounds(module2Rect);

    juce::Rectangle<int> module3Rect(bottomModulesArea.getX() + (moduleWidth * 2), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    sprayModule.setBounds(module3Rect);

    juce::Rectangle<int> module4Rect(bottomModulesArea.getX() + (moduleWidth * 3), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    pitchModule.setBounds(module4Rect);

    // --- FILA 1 ---
    juce::Rectangle<int> lfoRect(bottomModulesArea.getX(), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    lfoModule.setBounds(lfoRect);

    juce::Rectangle<int> envRect(bottomModulesArea.getX() + moduleWidth, bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    envelopeModule.setBounds(envRect);

    juce::Rectangle<int> filterRect(bottomModulesArea.getX() + (moduleWidth * 2), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    filterModule.setBounds(filterRect);

    juce::Rectangle<int> spaceRect(bottomModulesArea.getX() + (moduleWidth * 3), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    spaceModule.setBounds(spaceRect);
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
    if (files.isEmpty()) return;
    juce::String filePath = files[0];

    // Calculamos dónde están las capas visualmente
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);
    int layerHeight = bounds.getHeight() / 4;

    // ¿Dónde soltó el ratón el usuario?
    if (y < layerHeight)
    {
        // Cayó en la Capa 1
        audioProcessor.loadFile(filePath, 1);
        thumbnail.setSource(new juce::FileInputSource(juce::File(filePath)));
    }
    else if (y >= layerHeight && y < layerHeight * 2)
    {
        // Cayó en la Capa 2
        audioProcessor.loadFile(filePath, 2);
        thumbnailL2.setSource(new juce::FileInputSource(juce::File(filePath)));
    }
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
            zoomFactor = newZoomFactor;

            audioProcessor.windowStartRatio.store((float)viewStartRatio);
            audioProcessor.windowLengthRatio.store(1.0f / (float)zoomFactor);
            repaint();
        }
    }
}

void Granular_SynthAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);

    int layerHeight = bounds.getHeight() / 4;
    auto layer1Area = bounds.removeFromTop(layerHeight);
    auto layer2Area = bounds.removeFromTop(layerHeight);

    // ==========================================
    // --- LÓGICA DE CAPA 1 ---
    // ==========================================
    if (layer1Area.contains(event.getPosition()))
    {
        // 1. Si no estaba seleccionada, la seleccionamos y NO movemos el cursor
        if (activeLayer != 1)
        {
            activeLayer = 1;
            repaint();
            return; // Cortamos la ejecución aquí
        }

        // 2. Si YA estaba seleccionada, movemos el cursor libremente
        float clickX = event.getPosition().x - layer1Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer1Area.getWidth());

        if (auto* posParam = audioProcessor.apvts.getParameter("L1_POSITION"))
            posParam->setValueNotifyingHost(ratioInView);

        repaint();
    }
    // ==========================================
    // --- LÓGICA DE CAPA 2 ---
    // ==========================================
    else if (layer2Area.contains(event.getPosition()))
    {
        // 1. Si no estaba seleccionada, la seleccionamos y NO movemos el cursor
        if (activeLayer != 2)
        {
            activeLayer = 2;
            repaint();
            return; // Cortamos la ejecución aquí
        }

        // 2. Si YA estaba seleccionada, movemos el cursor libremente
        float clickX = event.getPosition().x - layer2Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer2Area.getWidth());

        if (auto* posParam = audioProcessor.apvts.getParameter("L2_POSITION"))
            posParam->setValueNotifyingHost(ratioInView);

        repaint();
    }
}

void Granular_SynthAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    // Al arrastrar, ya NO llamamos a mouseDown directamente porque si no
    // el primer milímetro de arrastre nos cortaría la ejecución (por el return; de arriba).
    // Queremos que si ya estamos arrastrando, mueva el cursor obligatoriamente.

    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);

    int layerHeight = bounds.getHeight() / 4;
    auto layer1Area = bounds.removeFromTop(layerHeight);
    auto layer2Area = bounds.removeFromTop(layerHeight);

    if (layer1Area.contains(event.getPosition()) && activeLayer == 1)
    {
        float clickX = event.getPosition().x - layer1Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer1Area.getWidth());

        if (auto* posParam = audioProcessor.apvts.getParameter("L1_POSITION"))
            posParam->setValueNotifyingHost(ratioInView);

        repaint();
    }
    else if (layer2Area.contains(event.getPosition()) && activeLayer == 2)
    {
        float clickX = event.getPosition().x - layer2Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer2Area.getWidth());

        if (auto* posParam = audioProcessor.apvts.getParameter("L2_POSITION"))
            posParam->setValueNotifyingHost(ratioInView);

        repaint();
    }
}

void Granular_SynthAudioProcessorEditor::timerCallback()
{
    // Borra la pantalla y vuelve a llamar a paint()"
    repaint();
}

