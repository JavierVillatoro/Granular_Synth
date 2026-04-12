/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GranularVoice.h"
#include "EnvelopeModule.h"

Granular_SynthAudioProcessorEditor::Granular_SynthAudioProcessorEditor(Granular_SynthAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    masterModule(p.apvts, p),
    bpmModule(p.apvts),
    mixerModule1(p.apvts, "L1_"),
    layer1Controls(p.apvts, "L1_"),
    layer2Controls(p.apvts, "L2_"),
    layer3Controls(p.apvts, "L3_"),
    layer4Controls(p.apvts, "L4_"),
    thumbnailCache(5),
    thumbnail(512, p.getFormatManager(), thumbnailCache),
    thumbnailL2(512, p.getFormatManager(), thumbnailCache),
    thumbnailL3(512, p.getFormatManager(), thumbnailCache),
    thumbnailL4(512, p.getFormatManager(), thumbnailCache)
{
    thumbnail.addChangeListener(this);
    thumbnailL2.addChangeListener(this);
    thumbnailL3.addChangeListener(this);
    thumbnailL4.addChangeListener(this);
    setSize(1200, 750);

    addAndMakeVisible(scanModule);
    addAndMakeVisible(engineModule);
    addAndMakeVisible(sprayModule);
    addAndMakeVisible(pitchModule);
    addAndMakeVisible(filterModule);
    addAndMakeVisible(envelopeModule);
    addAndMakeVisible(spaceModule);
    addAndMakeVisible(choirModule);
    addAndMakeVisible(lfoModule);
    addAndMakeVisible(masterModule);
    addAndMakeVisible(distModule);
    addAndMakeVisible(bpmModule);
    addAndMakeVisible(layer1Controls);
    addAndMakeVisible(layer2Controls);
    addAndMakeVisible(layer3Controls);
    addAndMakeVisible(layer4Controls);
    addAndMakeVisible(mixerModule1);
    addAndMakeVisible(monk1);
    addAndMakeVisible(monk2);
    addAndMakeVisible(monk3);
    addAndMakeVisible(monk4);

    audioProcessor.apvts.addParameterListener("L1_POSITION", this);
    audioProcessor.apvts.addParameterListener("L1_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L1_SHAPE", this);

    audioProcessor.apvts.addParameterListener("L2_POSITION", this);
    audioProcessor.apvts.addParameterListener("L2_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L2_SHAPE", this);

    audioProcessor.apvts.addParameterListener("L3_POSITION", this);
    audioProcessor.apvts.addParameterListener("L3_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L3_SHAPE", this);

    audioProcessor.apvts.addParameterListener("L4_POSITION", this);
    audioProcessor.apvts.addParameterListener("L4_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L4_SHAPE", this);

    
    for (int i = 1; i <= 4; ++i) {
        audioProcessor.apvts.addParameterListener("L" + juce::String(i) + "_MUTE", this);
        audioProcessor.apvts.addParameterListener("L" + juce::String(i) + "_SOLO", this);
    }

    startTimerHz(30);

    if (audioProcessor.isAudioLoadedL1 && audioProcessor.lastLoadedFilePathL1.isNotEmpty())
        thumbnail.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL1)));
    if (audioProcessor.isAudioLoadedL2 && audioProcessor.lastLoadedFilePathL2.isNotEmpty())
        thumbnailL2.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL2)));
    if (audioProcessor.isAudioLoadedL3 && audioProcessor.lastLoadedFilePathL3.isNotEmpty())
        thumbnailL3.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL3)));
    if (audioProcessor.isAudioLoadedL4 && audioProcessor.lastLoadedFilePathL4.isNotEmpty())
        thumbnailL4.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL4)));
}

Granular_SynthAudioProcessorEditor::~Granular_SynthAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener("L1_POSITION", this);
    audioProcessor.apvts.removeParameterListener("L1_GRAIN_SIZE", this);
    audioProcessor.apvts.removeParameterListener("L1_SHAPE", this);
    thumbnail.removeChangeListener(this);

    audioProcessor.apvts.removeParameterListener("L2_POSITION", this);
    audioProcessor.apvts.removeParameterListener("L2_GRAIN_SIZE", this);
    audioProcessor.apvts.removeParameterListener("L2_SHAPE", this);
    thumbnailL2.removeChangeListener(this);

    audioProcessor.apvts.removeParameterListener("L3_POSITION", this);
    audioProcessor.apvts.removeParameterListener("L3_GRAIN_SIZE", this);
    audioProcessor.apvts.removeParameterListener("L3_SHAPE", this);
    thumbnailL3.removeChangeListener(this);

    audioProcessor.apvts.removeParameterListener("L4_POSITION", this);
    audioProcessor.apvts.removeParameterListener("L4_GRAIN_SIZE", this);
    audioProcessor.apvts.removeParameterListener("L4_SHAPE", this);
    thumbnailL4.removeChangeListener(this);

    for (int i = 1; i <= 4; ++i) {
        audioProcessor.apvts.removeParameterListener("L" + juce::String(i) + "_MUTE", this);
        audioProcessor.apvts.removeParameterListener("L" + juce::String(i) + "_SOLO", this);
    }
}

void Granular_SynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ==========================================================
    // --- 1. MAGIA DE COLOR: TEMA DINÁMICO SEGÚN LA CAPA ---
    // ==========================================================
    juce::Colour themeColor = juce::Colours::cyan; // Capa 1 por defecto
    if (activeLayer == 2) themeColor = juce::Colours::magenta;
    else if (activeLayer == 3) themeColor = juce::Colours::orange;
    else if (activeLayer == 4) themeColor = juce::Colours::lime;

    g.fillAll(juce::Colour(0xff121212)); // Fondo oscuro global

    auto bounds = getLocalBounds();
    auto bottomModulesArea = bounds.removeFromBottom(300);
    auto rightMixerArea = bounds.removeFromRight(350);
    auto wavesArea = bounds;

    // Borde general de las ondas usando el color dinámico
    g.setColour(themeColor.withAlpha(0.3f));
    g.drawRect(wavesArea, 1);

    int layerHeight = wavesArea.getHeight() / 4;

    // --- LEER ESTADOS PARA EL DIMMING VISUAL ---
    bool m1 = audioProcessor.apvts.getRawParameterValue("L1_MUTE")->load() > 0.5f;
    bool m2 = audioProcessor.apvts.getRawParameterValue("L2_MUTE")->load() > 0.5f;
    bool m3 = audioProcessor.apvts.getRawParameterValue("L3_MUTE")->load() > 0.5f;
    bool m4 = audioProcessor.apvts.getRawParameterValue("L4_MUTE")->load() > 0.5f;

    bool s1 = audioProcessor.apvts.getRawParameterValue("L1_SOLO")->load() > 0.5f;
    bool s2 = audioProcessor.apvts.getRawParameterValue("L2_SOLO")->load() > 0.5f;
    bool s3 = audioProcessor.apvts.getRawParameterValue("L3_SOLO")->load() > 0.5f;
    bool s4 = audioProcessor.apvts.getRawParameterValue("L4_SOLO")->load() > 0.5f;

    bool anySolo = s1 || s2 || s3 || s4;

    // Si la capa está muteada, o hay otro solo activo, la opacidad cae a 0.2 (Fantasma)
    float a1 = (!m1 && (!anySolo || s1)) ? 1.0f : 0.2f;
    float a2 = (!m2 && (!anySolo || s2)) ? 1.0f : 0.2f;
    float a3 = (!m3 && (!anySolo || s3)) ? 1.0f : 0.2f;
    float a4 = (!m4 && (!anySolo || s4)) ? 1.0f : 0.2f;

    // Función Helper para pintar los botones de cada capa en la esquina inferior derecha
    auto drawLayerButtons = [&](juce::Rectangle<int> area, juce::Colour color, int num, bool isSolo, bool isMute, float layerAlpha) {
        auto btnArea = area.removeFromBottom(25).removeFromRight(55).withTrimmedRight(5);
        juce::Rectangle<int> btnNum = btnArea.removeFromRight(20);
        btnArea.removeFromRight(5);
        juce::Rectangle<int> btnSolo = btnArea.removeFromRight(20);

        // MUTE/NUM BUTTON (Invertido: brilla si la capa está ON/no muteada)
        g.setColour(!isMute ? color.withAlpha(juce::jmax(0.5f, layerAlpha)) : juce::Colours::grey.withAlpha(0.2f));
        g.fillRect(btnNum);
        g.setColour(juce::Colours::white.withAlpha(!isMute ? 1.0f : 0.3f));
        g.drawText(juce::String(num), btnNum, juce::Justification::centred);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRect(btnNum, 1);

        // SOLO BUTTON
        g.setColour(isSolo ? juce::Colours::yellow.withAlpha(0.8f) : juce::Colours::grey.withAlpha(0.2f));
        g.fillRect(btnSolo);
        g.setColour(juce::Colours::white.withAlpha(isSolo ? 1.0f : 0.3f));
        g.drawText("S", btnSolo, juce::Justification::centred);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRect(btnSolo, 1);
        };

    // ==========================================================
    // --- LÓGICA DE DIBUJO DE CAPAS (Con Dimming Dinámico) ---
    // ==========================================================
    // ==========================================================
    // --- LÓGICA DE DIBUJO DE CAPAS (Con Dimming Dinámico y Shape) ---
    // ==========================================================
    auto drawLayer = [&](juce::Rectangle<int> area, juce::Colour color, float alpha, juce::AudioThumbnail& thumb, juce::String prefix, double zF, double vsR, int num, bool isSolo, bool isMute) {
        g.setColour(color.withAlpha(0.6f * alpha));
        g.drawRect(area, 2);

        if (thumb.getNumChannels() > 0) {
            double totSecs = thumb.getTotalLength();
            double visSecs = totSecs / zF;
            double startT = vsR * totSecs;

            g.setColour(color.withAlpha(alpha));
            thumb.drawChannel(g, area.reduced(2), startT, startT + visSecs, 0, 1.0f);

            auto posParam = audioProcessor.apvts.getRawParameterValue(prefix + "POSITION");
            auto sizeParam = audioProcessor.apvts.getRawParameterValue(prefix + "GRAIN_SIZE");
            auto shapeParam = audioProcessor.apvts.getRawParameterValue(prefix + "SHAPE");

            if (posParam && sizeParam && shapeParam) {
                float winStart = posParam->load();
                if (prefix == "L1_") winStart = audioProcessor.windowStartRatioL1.load();
                else if (prefix == "L2_") winStart = audioProcessor.windowStartRatioL2.load();
                else if (prefix == "L3_") winStart = audioProcessor.windowStartRatioL3.load();
                else if (prefix == "L4_") winStart = audioProcessor.windowStartRatioL4.load();

                float winLen = 1.0f / zF;
                float currentPosition = posParam->load();
                float absolutePos = winStart + (currentPosition * winLen);

                double cursorTimeSeconds = absolutePos * totSecs;
                float cursorX = area.getX() + (((cursorTimeSeconds)-startT) / visSecs) * area.getWidth();

                // --- RECUPERADO: DIBUJO DE LA FORMA DEL GRANO (SHAPE / SIZE) ---
                float sizeRatio = sizeParam->load();
                float shapeValue = shapeParam->load();
                float activeAudioSeconds = (float)totSecs * winLen;
                float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * activeAudioSeconds);
                float grainWidthPixels = (grainSizeSeconds / visSecs) * area.getWidth();
                grainWidthPixels = juce::jmax(3.0f, grainWidthPixels);

                juce::Rectangle<float> grainWindow(cursorX - (grainWidthPixels / 2.0f), area.getY(), grainWidthPixels, area.getHeight());
                juce::Path grainPath;
                grainPath.startNewSubPath(grainWindow.getX(), grainWindow.getBottom());

                for (float x = 0; x <= grainWindow.getWidth(); x += 1.0f) {
                    float progress = x / grainWindow.getWidth();
                    float hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * progress));
                    float square = (progress < 0.005f) ? progress / 0.005f : (progress > 0.995f ? (1.0f - progress) / 0.005f : 1.0f);
                    float amplitude = (hann * (1.0f - shapeValue)) + (square * shapeValue);
                    float yPos = grainWindow.getBottom() - (amplitude * grainWindow.getHeight());
                    grainPath.lineTo(grainWindow.getX() + x, yPos);
                }
                grainPath.lineTo(grainWindow.getRight(), grainWindow.getBottom());
                grainPath.closeSubPath();

                g.setColour(color.withAlpha(0.3f * alpha));
                g.fillPath(grainPath);
                g.setColour(color.withAlpha(0.8f * alpha));
                g.strokePath(grainPath, juce::PathStrokeType(1.5f));
                // ---------------------------------------------------------------

                g.setColour(juce::Colours::white.withAlpha(0.9f * alpha));
                g.drawLine(cursorX, area.getY(), cursorX, area.getBottom(), 2.0f);
            }
        }
        drawLayerButtons(area, color, num, isSolo, isMute, alpha);
        };

    auto layer1Area = wavesArea.removeFromTop(layerHeight);
    drawLayer(layer1Area, juce::Colours::cyan, a1, thumbnail, "L1_", zoomFactor, viewStartRatio, 1, s1, m1);

    auto layer2Area = wavesArea.removeFromTop(layerHeight);
    drawLayer(layer2Area, juce::Colours::magenta, a2, thumbnailL2, "L2_", zoomFactorL2, viewStartRatioL2, 2, s2, m2);

    auto layer3Area = wavesArea.removeFromTop(layerHeight);
    drawLayer(layer3Area, juce::Colours::orange, a3, thumbnailL3, "L3_", zoomFactorL3, viewStartRatioL3, 3, s3, m3);

    auto layer4Area = wavesArea.removeFromTop(layerHeight);
    drawLayer(layer4Area, juce::Colours::lime, a4, thumbnailL4, "L4_", zoomFactorL4, viewStartRatioL4, 4, s4, m4);

    // --- RESALTAR LA CAPA ACTIVA ---
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    if (activeLayer == 1) g.drawRect(layer1Area, 2);
    else if (activeLayer == 2) g.drawRect(layer2Area, 2);
    else if (activeLayer == 3) g.drawRect(layer3Area, 2);
    else if (activeLayer == 4) g.drawRect(layer4Area, 2);

    // ==========================================================
    // --- 4. DIBUJAMOS LA ZONA DEL MIXER / ENVELOPES / FX ---
    // ==========================================================
    g.setColour(juce::Colour(0xff121212));
    g.fillRect(rightMixerArea);

    g.setColour(themeColor.withAlpha(0.3f));
    g.drawRect(matrixArea, 1);
    g.drawRect(mixerArea, 1);
    g.drawRect(masterArea, 1);
    g.drawRect(distArea, 1);
    g.drawRect(bpmArea, 1);

    g.setColour(themeColor.withAlpha(0.2f));
    g.drawRect(fxArea, 1);

    int fxW = fxArea.getWidth() / 2;
    int fxH = fxArea.getHeight() / 2;

    juce::Rectangle<int> vowelAreaRect(fxArea.getX(), fxArea.getY(), fxW, fxH);
    juce::Rectangle<int> resAreaRect(fxArea.getX() + fxW, fxArea.getY(), fxW, fxH);
    juce::Rectangle<int> tapeAreaRect(fxArea.getX(), fxArea.getY() + fxH, fxW, fxH);
    juce::Rectangle<int> stutterAreaRect(fxArea.getX() + fxW, fxArea.getY() + fxH, fxW, fxH);

    g.drawRect(vowelAreaRect, 1);
    g.drawRect(resAreaRect, 1);
    g.drawRect(tapeAreaRect, 1);
    g.drawRect(stutterAreaRect, 1);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(14.0f);
    g.drawText("MATRIX", matrixArea, juce::Justification::centred);
    g.drawText("MIX", mixerArea, juce::Justification::centred);

    g.setFont(11.0f);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText("VOICE 1", vowelAreaRect.withTrimmedTop(5), juce::Justification::centredTop);
    g.drawText("VOICE 2", resAreaRect.withTrimmedTop(5), juce::Justification::centredTop);
    g.drawText("VOICE 3", tapeAreaRect.withTrimmedTop(5), juce::Justification::centredTop);
    g.drawText("VOICE 4", stutterAreaRect.withTrimmedTop(5), juce::Justification::centredTop);

    // Ajustamos las listas de nombres debajo de los knobs
    std::vector<juce::StringArray> knobNames = {
        juce::StringArray {"Size", "Density", "Shape"}, juce::StringArray {"Pos", "Speed", "Dir"},
        juce::StringArray {"Pos", "Pitch", "Pan"}, juce::StringArray {"Trans", "Fine", "Scale"},
        juce::StringArray {"", "", ""}, juce::StringArray {"", "", "", ""},
        juce::StringArray {"", "", ""}
    };

    juce::StringArray moduleNames = { "ENGINE", "SCAN", "SPRAY", "PITCH", "", "", "" };

    int numColumns = 4, numRows = 2;
    int moduleWidth = bottomModulesArea.getWidth() / numColumns;
    int moduleHeight = bottomModulesArea.getHeight() / numRows;

    int modIndex = 0;
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numColumns; ++col) {
            if (row == 1 && col == 3) continue; // Nos saltamos el último hueco

            juce::Rectangle<int> moduleRect(bottomModulesArea.getX() + (col * moduleWidth), bottomModulesArea.getY() + (row * moduleHeight), moduleWidth, moduleHeight);

            g.setColour(themeColor.withAlpha(0.2f));
            g.drawRect(moduleRect, 1);

            if (moduleNames[modIndex].isNotEmpty()) {
                g.setColour(juce::Colours::white.withAlpha(0.7f));
                g.setFont(juce::Font(16.0f, juce::Font::bold));
                g.drawText(moduleNames[modIndex], moduleRect.removeFromTop(30), juce::Justification::centred);
            }

            auto labelArea = moduleRect.removeFromBottom(25);
            int numKnobs = knobNames[modIndex].size();
            if (numKnobs > 0) {
                int labelWidth = labelArea.getWidth() / numKnobs;
                g.setFont(12.0f);
                g.setColour(juce::Colours::white.withAlpha(0.4f));
                for (int i = 0; i < numKnobs; ++i) {
                    g.drawText(knobNames[modIndex][i], labelArea.removeFromLeft(labelWidth), juce::Justification::centred);
                }
            }
            modIndex++;
        }
    }

    // --- DIBUJAMOS EL 8º HUECO (CHOIR y SPACE) ---
    juce::Rectangle<int> lastBlock(bottomModulesArea.getX() + (moduleWidth * 3), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    int halfW = lastBlock.getWidth() / 2;

    juce::Rectangle<int> choirRect = lastBlock.removeFromLeft(halfW);
    juce::Rectangle<int> spaceRect = lastBlock;

    g.setColour(themeColor.withAlpha(0.2f));
    g.drawRect(choirRect, 1);
    g.drawRect(spaceRect, 1);
}

void Granular_SynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto bottomModulesArea = bounds.removeFromBottom(300);
    auto rightMixerArea = bounds.removeFromRight(350);
    auto wavesArea = bounds;
    int layerHeight = wavesArea.getHeight() / 4;

    // Capas
    auto layer1Area = wavesArea.removeFromTop(layerHeight);
    layer1Controls.setBounds(layer1Area.reduced(10).withHeight(20));

    auto layer2Area = wavesArea.removeFromTop(layerHeight);
    layer2Controls.setBounds(layer2Area.reduced(10).withHeight(20));

    auto layer3Area = wavesArea.removeFromTop(layerHeight);
    layer3Controls.setBounds(layer3Area.reduced(10).withHeight(20));

    auto layer4Area = wavesArea.removeFromTop(layerHeight);
    layer4Controls.setBounds(layer4Area.reduced(10).withHeight(20));

    // Mixer / Master
    auto area = rightMixerArea;
    auto rightColumn = area.removeFromRight(area.getWidth() * 0.35f);
    auto leftColumn = area;

    matrixArea = leftColumn.removeFromTop(leftColumn.getHeight() * 0.5f);

    auto bottomHalf = leftColumn;
    mixerArea = bottomHalf.removeFromLeft(bottomHalf.getWidth() * 0.45f);
    fxArea = bottomHalf;

    mixerModule1.setBounds(mixerArea);

    int fxW = fxArea.getWidth() / 2;
    int fxH = fxArea.getHeight() / 2;

    // Pad XY Monjes (M1 a M4)
    juce::Rectangle<int> areaM1(fxArea.getX(), fxArea.getY(), fxW, fxH);
    juce::Rectangle<int> areaM2(fxArea.getX() + fxW, fxArea.getY(), fxW, fxH);
    juce::Rectangle<int> areaM3(fxArea.getX(), fxArea.getY() + fxH, fxW, fxH);
    juce::Rectangle<int> areaM4(fxArea.getX() + fxW, fxArea.getY() + fxH, fxW, fxH);

    monk1.setBounds(areaM1);
    monk2.setBounds(areaM2);
    monk3.setBounds(areaM3);
    monk4.setBounds(areaM4);

    // Módulos verticales derechos (Master, Dist, BPM)
    masterArea = rightColumn.removeFromTop(rightColumn.getHeight() * 0.5f);
    distArea = rightColumn.removeFromTop(rightColumn.getHeight() * 0.5f);
    distModule.setBounds(distArea);
    masterModule.setBounds(masterArea);
    bpmArea = rightColumn;
    bpmModule.setBounds(bpmArea);

    // Módulos inferiores (Fila 1 y 2)
    int moduleWidth = bottomModulesArea.getWidth() / 4;
    int moduleHeight = bottomModulesArea.getHeight() / 2;

    juce::Rectangle<int> module1Rect(bottomModulesArea.getX(), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    engineModule.setBounds(module1Rect);
    juce::Rectangle<int> module2Rect(bottomModulesArea.getX() + moduleWidth, bottomModulesArea.getY(), moduleWidth, moduleHeight);
    scanModule.setBounds(module2Rect);
    juce::Rectangle<int> module3Rect(bottomModulesArea.getX() + (moduleWidth * 2), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    sprayModule.setBounds(module3Rect);
    juce::Rectangle<int> module4Rect(bottomModulesArea.getX() + (moduleWidth * 3), bottomModulesArea.getY(), moduleWidth, moduleHeight);
    pitchModule.setBounds(module4Rect);

    juce::Rectangle<int> lfoRect(bottomModulesArea.getX(), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    lfoModule.setBounds(lfoRect);
    juce::Rectangle<int> envRect(bottomModulesArea.getX() + moduleWidth, bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    envelopeModule.setBounds(envRect);
    juce::Rectangle<int> filterRect(bottomModulesArea.getX() + (moduleWidth * 2), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    filterModule.setBounds(filterRect);

    
    juce::Rectangle<int> lastBlock(bottomModulesArea.getX() + (moduleWidth * 3), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    int halfW = lastBlock.getWidth() / 2;

    juce::Rectangle<int> choirRect = lastBlock.removeFromLeft(halfW);
    juce::Rectangle<int> spaceRect = lastBlock; // Space se queda con la mitad derecha

    // Ahora sí, le pasamos el rectángulo correcto al módulo
    spaceModule.setBounds(spaceRect.reduced(2));
    choirModule.setBounds(choirRect.reduced(2));
}

bool Granular_SynthAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto file : files) {
        if (file.containsIgnoreCase(".wav") || file.containsIgnoreCase(".mp3") ||
            file.containsIgnoreCase(".aif") || file.containsIgnoreCase(".flac") ||
            file.containsIgnoreCase(".aiff")) return true;
    }
    return false;
}

void Granular_SynthAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    if (files.isEmpty()) return;
    juce::String filePath = files[0];

    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);
    int layerHeight = bounds.getHeight() / 4;

    if (y < layerHeight) {
        audioProcessor.loadFile(filePath, 1);
        thumbnail.setSource(new juce::FileInputSource(juce::File(filePath)));
    }
    else if (y >= layerHeight && y < layerHeight * 2) {
        audioProcessor.loadFile(filePath, 2);
        thumbnailL2.setSource(new juce::FileInputSource(juce::File(filePath)));
    }
    else if (y >= layerHeight * 2 && y < layerHeight * 3) {
        audioProcessor.loadFile(filePath, 3);
        thumbnailL3.setSource(new juce::FileInputSource(juce::File(filePath)));
    }
    else if (y >= layerHeight * 3) {
        audioProcessor.loadFile(filePath, 4);
        thumbnailL4.setSource(new juce::FileInputSource(juce::File(filePath)));
    }
}

void Granular_SynthAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source) { repaint(); }

void Granular_SynthAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);

    int layerHeight = bounds.getHeight() / 4;
    auto layer1Area = bounds.removeFromTop(layerHeight);
    auto layer2Area = bounds.removeFromTop(layerHeight);
    auto layer3Area = bounds.removeFromTop(layerHeight);
    auto layer4Area = bounds.removeFromTop(layerHeight);

    if (layer1Area.contains(event.getPosition()) && thumbnail.getTotalLength() > 0.0) {
        double mouseX = event.getPosition().x - layer1Area.getX();
        double mouseRatioInView = mouseX / (double)layer1Area.getWidth();
        double timeUnderMouse = viewStartRatio + (mouseRatioInView / zoomFactor);
        double zoomMultiplier = (wheel.deltaY > 0) ? 1.2 : 1.0 / 1.2;
        double newZoomFactor = juce::jlimit(1.0, 50.0, zoomFactor * zoomMultiplier);
        viewStartRatio = timeUnderMouse - (mouseRatioInView / newZoomFactor);
        viewStartRatio = juce::jlimit(0.0, 1.0 - (1.0 / newZoomFactor), viewStartRatio);
        zoomFactor = newZoomFactor;
        audioProcessor.windowStartRatioL1.store((float)viewStartRatio);
        audioProcessor.windowLengthRatioL1.store(1.0f / (float)zoomFactor);
        repaint();
    }
    else if (layer2Area.contains(event.getPosition()) && thumbnailL2.getTotalLength() > 0.0) {
        double mouseX = event.getPosition().x - layer2Area.getX();
        double mouseRatioInView = mouseX / (double)layer2Area.getWidth();
        double timeUnderMouse = viewStartRatioL2 + (mouseRatioInView / zoomFactorL2);
        double zoomMultiplier = (wheel.deltaY > 0) ? 1.2 : 1.0 / 1.2;
        double newZoomFactorL2 = juce::jlimit(1.0, 50.0, zoomFactorL2 * zoomMultiplier);
        viewStartRatioL2 = timeUnderMouse - (mouseRatioInView / newZoomFactorL2);
        viewStartRatioL2 = juce::jlimit(0.0, 1.0 - (1.0 / newZoomFactorL2), viewStartRatioL2);
        zoomFactorL2 = newZoomFactorL2;
        audioProcessor.windowStartRatioL2.store((float)viewStartRatioL2);
        audioProcessor.windowLengthRatioL2.store(1.0f / (float)zoomFactorL2);
        repaint();
    }
    else if (layer3Area.contains(event.getPosition()) && thumbnailL3.getTotalLength() > 0.0) {
        double mouseX = event.getPosition().x - layer3Area.getX();
        double mouseRatioInView = mouseX / (double)layer3Area.getWidth();
        double timeUnderMouse = viewStartRatioL3 + (mouseRatioInView / zoomFactorL3);
        double zoomMultiplier = (wheel.deltaY > 0) ? 1.2 : 1.0 / 1.2;
        double newZoomFactorL3 = juce::jlimit(1.0, 50.0, zoomFactorL3 * zoomMultiplier);
        viewStartRatioL3 = timeUnderMouse - (mouseRatioInView / newZoomFactorL3);
        viewStartRatioL3 = juce::jlimit(0.0, 1.0 - (1.0 / newZoomFactorL3), viewStartRatioL3);
        zoomFactorL3 = newZoomFactorL3;
        audioProcessor.windowStartRatioL3.store((float)viewStartRatioL3);
        audioProcessor.windowLengthRatioL3.store(1.0f / (float)zoomFactorL3);
        repaint();
    }
    else if (layer4Area.contains(event.getPosition()) && thumbnailL4.getTotalLength() > 0.0) {
        double mouseX = event.getPosition().x - layer4Area.getX();
        double mouseRatioInView = mouseX / (double)layer4Area.getWidth();
        double timeUnderMouse = viewStartRatioL4 + (mouseRatioInView / zoomFactorL4);
        double zoomMultiplier = (wheel.deltaY > 0) ? 1.2 : 1.0 / 1.2;
        double newZoomFactorL4 = juce::jlimit(1.0, 50.0, zoomFactorL4 * zoomMultiplier);
        viewStartRatioL4 = timeUnderMouse - (mouseRatioInView / newZoomFactorL4);
        viewStartRatioL4 = juce::jlimit(0.0, 1.0 - (1.0 / newZoomFactorL4), viewStartRatioL4);
        zoomFactorL4 = newZoomFactorL4;
        audioProcessor.windowStartRatioL4.store((float)viewStartRatioL4);
        audioProcessor.windowLengthRatioL4.store(1.0f / (float)zoomFactorL4);
        repaint();
    }
}

void Granular_SynthAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    lastDragX = event.getPosition().x;
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300); bounds.removeFromRight(350);

    int layerHeight = bounds.getHeight() / 4;
    auto layer1Area = bounds.removeFromTop(layerHeight);
    auto layer2Area = bounds.removeFromTop(layerHeight);
    auto layer3Area = bounds.removeFromTop(layerHeight);
    auto layer4Area = bounds.removeFromTop(layerHeight);

    // Función auxiliar para obtener las coordenadas exactas de los botones
    auto getButtonsArea = [&](juce::Rectangle<int> area) {
        auto btnArea = area.removeFromBottom(25).removeFromRight(55).withTrimmedRight(5);
        juce::Rectangle<int> btnNum = btnArea.removeFromRight(20);
        btnArea.removeFromRight(5);
        juce::Rectangle<int> btnSolo = btnArea.removeFromRight(20);
        return std::make_pair(btnSolo, btnNum);
        };

    // --- 1. COMPROBAR CLICS EN BOTONES (Antes que la selección de pista) ---
    auto checkButtons = [&](juce::Rectangle<int> area, juce::String prefix) {
        auto btns = getButtonsArea(area);
        if (btns.first.contains(event.getPosition())) {
            auto* p = audioProcessor.apvts.getParameter(prefix + "SOLO");
            p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f);
            return true;
        }
        if (btns.second.contains(event.getPosition())) {
            auto* p = audioProcessor.apvts.getParameter(prefix + "MUTE");
            p->setValueNotifyingHost(p->getValue() > 0.5f ? 0.0f : 1.0f);
            return true;
        }
        return false;
        };

    if (checkButtons(layer1Area, "L1_")) return;
    if (checkButtons(layer2Area, "L2_")) return;
    if (checkButtons(layer3Area, "L3_")) return;
    if (checkButtons(layer4Area, "L4_")) return;


    // --- 2. LÓGICA DE SELECCIÓN DE PISTA (Si no hiciste clic en un botón) ---
    auto updateModules = [&](int layer) {
        activeLayer = layer; engineModule.setLayer(layer); scanModule.setLayer(layer);
        sprayModule.setLayer(layer); pitchModule.setLayer(layer); filterModule.setLayer(layer);
        spaceModule.setLayer(layer); choirModule.setLayer(layer); distModule.setLayer(layer);
        envelopeModule.setLayer(layer); mixerModule1.setLayer(layer);
        monk1.setLayer(layer); monk2.setLayer(layer); monk3.setLayer(layer); monk4.setLayer(layer);
        };

    if (layer1Area.contains(event.getPosition())) {
        if (activeLayer != 1) updateModules(1); repaint();
    }
    else if (layer2Area.contains(event.getPosition())) {
        if (activeLayer != 2) updateModules(2); repaint();
    }
    else if (layer3Area.contains(event.getPosition())) {
        if (activeLayer != 3) updateModules(3); repaint();
    }
    else if (layer4Area.contains(event.getPosition())) {
        if (activeLayer != 4) updateModules(4); repaint();
    }
}

void Granular_SynthAudioProcessorEditor::mouseDrag(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);

    int layerHeight = bounds.getHeight() / 4;
    auto layer1Area = bounds.removeFromTop(layerHeight);
    auto layer2Area = bounds.removeFromTop(layerHeight);
    auto layer3Area = bounds.removeFromTop(layerHeight);
    auto layer4Area = bounds.removeFromTop(layerHeight);

    // Calculamos cuánto se ha movido el ratón desde el último fotograma
    int deltaX = event.getPosition().x - lastDragX;
    lastDragX = event.getPosition().x;

    bool isPanMode = event.mods.isAltDown() || event.mods.isRightButtonDown();

    if (activeLayer == 1) {
        if (isPanMode) {
            // Lógica de Panning: Movemos la cámara proporcionalmente al zoom
            double panShift = ((double)deltaX / (double)layer1Area.getWidth()) / zoomFactor;
            viewStartRatio -= panShift; // Restamos para que la onda acompañe al ratón
            viewStartRatio = juce::jlimit(0.0, 1.0 - (1.0 / zoomFactor), viewStartRatio);
            audioProcessor.windowStartRatioL1.store((float)viewStartRatio);
        }
        else {
            // Lógica de Scrubbing normal
            float clickX = event.getPosition().x - layer1Area.getX();
            float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer1Area.getWidth());
            if (auto* p = audioProcessor.apvts.getParameter("L1_POSITION")) p->setValueNotifyingHost(ratioInView);
        }
        repaint();
    }
    else if (activeLayer == 2) {
        if (isPanMode) {
            double panShift = ((double)deltaX / (double)layer2Area.getWidth()) / zoomFactorL2;
            viewStartRatioL2 -= panShift;
            viewStartRatioL2 = juce::jlimit(0.0, 1.0 - (1.0 / zoomFactorL2), viewStartRatioL2);
            audioProcessor.windowStartRatioL2.store((float)viewStartRatioL2);
        }
        else {
            float clickX = event.getPosition().x - layer2Area.getX();
            float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer2Area.getWidth());
            if (auto* p = audioProcessor.apvts.getParameter("L2_POSITION")) p->setValueNotifyingHost(ratioInView);
        }
        repaint();
    }
    else if (activeLayer == 3) {
        if (isPanMode) {
            double panShift = ((double)deltaX / (double)layer3Area.getWidth()) / zoomFactorL3;
            viewStartRatioL3 -= panShift;
            viewStartRatioL3 = juce::jlimit(0.0, 1.0 - (1.0 / zoomFactorL3), viewStartRatioL3);
            audioProcessor.windowStartRatioL3.store((float)viewStartRatioL3);
        }
        else {
            float clickX = event.getPosition().x - layer3Area.getX();
            float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer3Area.getWidth());
            if (auto* p = audioProcessor.apvts.getParameter("L3_POSITION")) p->setValueNotifyingHost(ratioInView);
        }
        repaint();
    }
    else if (activeLayer == 4) {
        if (isPanMode) {
            double panShift = ((double)deltaX / (double)layer4Area.getWidth()) / zoomFactorL4;
            viewStartRatioL4 -= panShift;
            viewStartRatioL4 = juce::jlimit(0.0, 1.0 - (1.0 / zoomFactorL4), viewStartRatioL4);
            audioProcessor.windowStartRatioL4.store((float)viewStartRatioL4);
        }
        else {
            float clickX = event.getPosition().x - layer4Area.getX();
            float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer4Area.getWidth());
            if (auto* p = audioProcessor.apvts.getParameter("L4_POSITION")) p->setValueNotifyingHost(ratioInView);
        }
        repaint();
    }
}

void Granular_SynthAudioProcessorEditor::timerCallback() { repaint(); }

