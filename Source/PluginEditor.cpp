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
    thumbnailCache(5),
    thumbnail(512, p.getFormatManager(), thumbnailCache),
    thumbnailL2(512, p.getFormatManager(), thumbnailCache),
    thumbnailL3(512, p.getFormatManager(), thumbnailCache)
{
    thumbnail.addChangeListener(this);
    thumbnailL2.addChangeListener(this);
    thumbnailL3.addChangeListener(this);
    setSize(1200, 750);

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
    addAndMakeVisible(layer3Controls);
    addAndMakeVisible(mixerModule1);

    audioProcessor.apvts.addParameterListener("L1_POSITION", this);
    audioProcessor.apvts.addParameterListener("L1_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L1_SHAPE", this);

    audioProcessor.apvts.addParameterListener("L2_POSITION", this);
    audioProcessor.apvts.addParameterListener("L2_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L2_SHAPE", this);

    audioProcessor.apvts.addParameterListener("L3_POSITION", this);
    audioProcessor.apvts.addParameterListener("L3_GRAIN_SIZE", this);
    audioProcessor.apvts.addParameterListener("L3_SHAPE", this);

    startTimerHz(30);

    if (audioProcessor.isAudioLoadedL1 && audioProcessor.lastLoadedFilePathL1.isNotEmpty())
        thumbnail.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL1)));
    if (audioProcessor.isAudioLoadedL2 && audioProcessor.lastLoadedFilePathL2.isNotEmpty())
        thumbnailL2.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL2)));
    if (audioProcessor.isAudioLoadedL3 && audioProcessor.lastLoadedFilePathL3.isNotEmpty())
        thumbnailL3.setSource(new juce::FileInputSource(juce::File(audioProcessor.lastLoadedFilePathL3)));
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
}

void Granular_SynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff121212));

    auto bounds = getLocalBounds();
    auto bottomModulesArea = bounds.removeFromBottom(300);
    auto rightMixerArea = bounds.removeFromRight(350);
    auto wavesArea = bounds;

    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRect(wavesArea, 1);

    int layerHeight = wavesArea.getHeight() / 4;

    // ==========================================================
    // --- CAPA 1 (CYAN) ---
    // ==========================================================
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

        auto positionParam = audioProcessor.apvts.getRawParameterValue("L1_POSITION");
        auto grainSizeParam = audioProcessor.apvts.getRawParameterValue("L1_GRAIN_SIZE");
        auto shapeParamVal = audioProcessor.apvts.getRawParameterValue("L1_SHAPE");

        if (positionParam != nullptr && grainSizeParam != nullptr && shapeParamVal != nullptr)
        {
            float currentPosition = positionParam->load();
            float sizeRatio = grainSizeParam->load();
            float shapeValue = shapeParamVal->load();

            float winStart = audioProcessor.windowStartRatioL1.load();
            float winLen = audioProcessor.windowLengthRatioL1.load();

            float absolutePos = winStart + (currentPosition * winLen);
            double cursorTimeSeconds = absolutePos * totalAudioSeconds;

            float activeAudioSeconds = (float)totalAudioSeconds * winLen;
            float grainSizeSeconds = juce::jmax(0.01f, sizeRatio * activeAudioSeconds);

            float cursorX = layer1Area.getX() + ((cursorTimeSeconds - startTime) / visibleSeconds) * layer1Area.getWidth();
            float grainWidthPixels = (grainSizeSeconds / visibleSeconds) * layer1Area.getWidth();
            grainWidthPixels = juce::jmax(3.0f, grainWidthPixels);

            juce::Rectangle<float> grainWindow(cursorX - (grainWidthPixels / 2.0f), layer1Area.getY(), grainWidthPixels, layer1Area.getHeight());
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

            g.setColour(juce::Colours::cyan.withAlpha(0.3f));
            g.fillPath(grainPath);
            g.setColour(juce::Colours::cyan.withAlpha(0.8f));
            g.strokePath(grainPath, juce::PathStrokeType(1.5f));

            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawLine(cursorX, layer1Area.getY(), cursorX, layer1Area.getBottom(), 2.0f);
        }

        auto& synthL1 = audioProcessor.getSynthesiserL1();
        for (int i = 0; i < synthL1.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<GranularVoice*>(synthL1.getVoice(i))) {
                for (int g_idx = 0; g_idx < 128; ++g_idx) {
                    float env = voice->visualGrainEnv[g_idx].load();
                    if (env > 0.001f) {
                        float pos = voice->visualGrainPos[g_idx].load();
                        double grainTimeSeconds = pos * totalAudioSeconds;
                        float xPixel = layer1Area.getX() + ((grainTimeSeconds - startTime) / visibleSeconds) * layer1Area.getWidth();

                        if (xPixel >= layer1Area.getX() && xPixel <= layer1Area.getRight()) {
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
    }

    // ==========================================================
    // --- CAPA 2 (MAGENTA) ---
    // ==========================================================
    auto layer2Area = wavesArea.removeFromTop(layerHeight);
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

        auto posParamL2 = audioProcessor.apvts.getRawParameterValue("L2_POSITION");
        auto sizeParamL2 = audioProcessor.apvts.getRawParameterValue("L2_GRAIN_SIZE");
        auto shapeParamL2 = audioProcessor.apvts.getRawParameterValue("L2_SHAPE");

        if (posParamL2 != nullptr && sizeParamL2 != nullptr && shapeParamL2 != nullptr)
        {
            float currentPositionL2 = posParamL2->load();
            float sizeRatioL2 = sizeParamL2->load();
            float shapeValueL2 = shapeParamL2->load();

            float winStartL2 = audioProcessor.windowStartRatioL2.load();
            float winLenL2 = audioProcessor.windowLengthRatioL2.load();

            float absolutePosL2 = winStartL2 + (currentPositionL2 * winLenL2);
            double cursorTimeSecondsL2 = absolutePosL2 * totalAudioSecondsL2;
            float activeAudioSecondsL2 = (float)totalAudioSecondsL2 * winLenL2;
            float grainSizeSecondsL2 = juce::jmax(0.01f, sizeRatioL2 * activeAudioSecondsL2);

            float cursorXL2 = layer2Area.getX() + ((cursorTimeSecondsL2 - startTimeL2) / visibleSecondsL2) * layer2Area.getWidth();
            float grainWidthPixelsL2 = (grainSizeSecondsL2 / visibleSecondsL2) * layer2Area.getWidth();
            grainWidthPixelsL2 = juce::jmax(3.0f, grainWidthPixelsL2);

            juce::Rectangle<float> grainWindowL2(cursorXL2 - (grainWidthPixelsL2 / 2.0f), layer2Area.getY(), grainWidthPixelsL2, layer2Area.getHeight());
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

        auto& synthL2 = audioProcessor.getSynthesiserL2();
        for (int i = 0; i < synthL2.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<GranularVoice*>(synthL2.getVoice(i))) {
                for (int g_idx = 0; g_idx < 128; ++g_idx) {
                    float env = voice->visualGrainEnv[g_idx].load();
                    if (env > 0.001f) {
                        float pos = voice->visualGrainPos[g_idx].load();
                        double grainTimeSeconds = pos * totalAudioSecondsL2;
                        float xPixel = layer2Area.getX() + ((grainTimeSeconds - startTimeL2) / visibleSecondsL2) * layer2Area.getWidth();

                        if (xPixel >= layer2Area.getX() && xPixel <= layer2Area.getRight()) {
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

    // ==========================================================
    // --- CAPA 3 (NARANJA) ---
    // ==========================================================
    auto layer3Area = wavesArea.removeFromTop(layerHeight);
    g.setColour(juce::Colours::orange.withAlpha(0.6f));
    g.drawRect(layer3Area, 2);

    if (thumbnailL3.getNumChannels() > 0)
    {
        double totalAudioSecondsL3 = thumbnailL3.getTotalLength();
        double visibleSecondsL3 = totalAudioSecondsL3 / zoomFactorL3;
        double startTimeL3 = viewStartRatioL3 * totalAudioSecondsL3;
        double endTimeL3 = startTimeL3 + visibleSecondsL3;

        g.setColour(juce::Colours::orange);
        thumbnailL3.drawChannel(g, layer3Area.reduced(2), startTimeL3, endTimeL3, 0, 1.0f);

        auto posParamL3 = audioProcessor.apvts.getRawParameterValue("L3_POSITION");
        auto sizeParamL3 = audioProcessor.apvts.getRawParameterValue("L3_GRAIN_SIZE");
        auto shapeParamL3 = audioProcessor.apvts.getRawParameterValue("L3_SHAPE");

        if (posParamL3 != nullptr && sizeParamL3 != nullptr && shapeParamL3 != nullptr)
        {
            float currentPositionL3 = posParamL3->load();
            float sizeRatioL3 = sizeParamL3->load();
            float shapeValueL3 = shapeParamL3->load();

            float winStartL3 = audioProcessor.windowStartRatioL3.load();
            float winLenL3 = audioProcessor.windowLengthRatioL3.load();

            float absolutePosL3 = winStartL3 + (currentPositionL3 * winLenL3);
            double cursorTimeSecondsL3 = absolutePosL3 * totalAudioSecondsL3;
            float activeAudioSecondsL3 = (float)totalAudioSecondsL3 * winLenL3;
            float grainSizeSecondsL3 = juce::jmax(0.01f, sizeRatioL3 * activeAudioSecondsL3);

            float cursorXL3 = layer3Area.getX() + ((cursorTimeSecondsL3 - startTimeL3) / visibleSecondsL3) * layer3Area.getWidth();
            float grainWidthPixelsL3 = (grainSizeSecondsL3 / visibleSecondsL3) * layer3Area.getWidth();
            grainWidthPixelsL3 = juce::jmax(3.0f, grainWidthPixelsL3);

            juce::Rectangle<float> grainWindowL3(cursorXL3 - (grainWidthPixelsL3 / 2.0f), layer3Area.getY(), grainWidthPixelsL3, layer3Area.getHeight());
            juce::Path grainPathL3;
            grainPathL3.startNewSubPath(grainWindowL3.getX(), grainWindowL3.getBottom());

            for (float x = 0; x <= grainWindowL3.getWidth(); x += 1.0f) {
                float progress = x / grainWindowL3.getWidth();
                float hann = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * progress));
                float square = (progress < 0.005f) ? progress / 0.005f : (progress > 0.995f ? (1.0f - progress) / 0.005f : 1.0f);
                float amplitude = (hann * (1.0f - shapeValueL3)) + (square * shapeValueL3);
                float yPos = grainWindowL3.getBottom() - (amplitude * grainWindowL3.getHeight());
                grainPathL3.lineTo(grainWindowL3.getX() + x, yPos);
            }
            grainPathL3.lineTo(grainWindowL3.getRight(), grainWindowL3.getBottom());
            grainPathL3.closeSubPath();

            g.setColour(juce::Colours::orange.withAlpha(0.3f));
            g.fillPath(grainPathL3);
            g.setColour(juce::Colours::orange.withAlpha(0.8f));
            g.strokePath(grainPathL3, juce::PathStrokeType(1.5f));

            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawLine(cursorXL3, layer3Area.getY(), cursorXL3, layer3Area.getBottom(), 2.0f);
        }

        auto& synthL3 = audioProcessor.getSynthesiserL3();
        for (int i = 0; i < synthL3.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<GranularVoice*>(synthL3.getVoice(i))) {
                for (int g_idx = 0; g_idx < 128; ++g_idx) {
                    float env = voice->visualGrainEnv[g_idx].load();
                    if (env > 0.001f) {
                        float pos = voice->visualGrainPos[g_idx].load();
                        double grainTimeSeconds = pos * totalAudioSecondsL3;
                        float xPixel = layer3Area.getX() + ((grainTimeSeconds - startTimeL3) / visibleSecondsL3) * layer3Area.getWidth();

                        if (xPixel >= layer3Area.getX() && xPixel <= layer3Area.getRight()) {
                            float maxLineHeight = layer3Area.getHeight() * 0.8f;
                            float currentHeight = maxLineHeight * env;
                            float yCenter = layer3Area.getCentreY();
                            float yStart = yCenter - (currentHeight / 2.0f);
                            g.setColour(juce::Colours::white.withAlpha(env * 0.8f));
                            g.drawLine(xPixel, yStart, xPixel, yStart + currentHeight, 1.5f + (env * 1.5f));
                        }
                    }
                }
            }
        }
    }

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    for (int i = 0; i < 1; ++i) // AHORA SOLO QUEDA 1 HUECO LIBRE
    {
        g.drawRect(wavesArea.removeFromTop(layerHeight), 1);
    }

    // --- RESALTAR LA CAPA ACTIVA ---
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    if (activeLayer == 1) g.drawRect(layer1Area, 2);
    else if (activeLayer == 2) g.drawRect(layer2Area, 2);
    else if (activeLayer == 3) g.drawRect(layer3Area, 2); // Naranja Activo

    // 4. DIBUJAMOS LA ZONA DEL MIXER / ENVELOPES
    g.setColour(juce::Colour(0xff121212));
    g.fillRect(rightMixerArea);

    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRect(matrixArea, 1);
    g.drawRect(mixerArea, 1);
    g.drawRect(masterArea, 1);
    g.drawRect(distArea, 1);
    g.drawRect(bpmArea, 1);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(14.0f);
    g.drawText("MATRIX", matrixArea, juce::Justification::centred);
    g.drawText("MIXER", mixerArea, juce::Justification::centred);

    std::vector<juce::StringArray> knobNames = {
        juce::StringArray {"Size", "Density", "Shape"}, juce::StringArray {"Pos", "Speed", "Dir"},
        juce::StringArray {"Pos", "Pitch", "Pan"}, juce::StringArray {"Trans", "Fine", "Scale"},
        juce::StringArray {"", "", ""}, juce::StringArray {"", "", "", ""},
        juce::StringArray {"", "", ""}, juce::StringArray {"Size", "Fback", "Mix"}
    };
    juce::StringArray moduleNames = { "ENGINE", "SCAN", "SPRAY", "PITCH", "", "", "", "SPACE" };

    int numColumns = 4, numRows = 2;
    int moduleWidth = bottomModulesArea.getWidth() / numColumns;
    int moduleHeight = bottomModulesArea.getHeight() / numRows;

    int modIndex = 0;
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numColumns; ++col) {
            juce::Rectangle<int> moduleRect(bottomModulesArea.getX() + (col * moduleWidth), bottomModulesArea.getY() + (row * moduleHeight), moduleWidth, moduleHeight);
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
            for (int i = 0; i < numKnobs; ++i) g.drawText(knobNames[modIndex][i], labelArea.removeFromLeft(labelWidth), juce::Justification::centred);
            modIndex++;
        }
    }
}

void Granular_SynthAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto bottomModulesArea = bounds.removeFromBottom(300);
    auto rightMixerArea = bounds.removeFromRight(350);
    auto wavesArea = bounds;
    int layerHeight = wavesArea.getHeight() / 4;

    auto layer1Area = wavesArea.removeFromTop(layerHeight);
    layer1Controls.setBounds(layer1Area.getX() + 10, layer1Area.getY() + 10, 150, 20);

    auto layer2Area = wavesArea.removeFromTop(layerHeight);
    layer2Controls.setBounds(layer2Area.getX() + 10, layer2Area.getY() + 10, 150, 20);

    auto layer3Area = wavesArea.removeFromTop(layerHeight);
    layer3Controls.setBounds(layer3Area.getX() + 10, layer3Area.getY() + 10, 150, 20);

    auto area = rightMixerArea;
    auto rightColumn = area.removeFromRight(area.getWidth() * 0.35f);
    auto leftColumn = area;

    matrixArea = leftColumn.removeFromTop(leftColumn.getHeight() * 0.5f);
    mixerArea = leftColumn;
    mixerModule1.setBounds(mixerArea);

    masterArea = rightColumn.removeFromTop(rightColumn.getHeight() * 0.5f);
    distArea = rightColumn.removeFromTop(rightColumn.getHeight() * 0.5f);
    distModule.setBounds(distArea);
    masterModule.setBounds(masterArea);
    bpmArea = rightColumn;
    bpmModule.setBounds(bpmArea);

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
    juce::Rectangle<int> spaceRect(bottomModulesArea.getX() + (moduleWidth * 3), bottomModulesArea.getY() + moduleHeight, moduleWidth, moduleHeight);
    spaceModule.setBounds(spaceRect);
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
}

void Granular_SynthAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(300);
    bounds.removeFromRight(350);

    int layerHeight = bounds.getHeight() / 4;
    auto layer1Area = bounds.removeFromTop(layerHeight);
    auto layer2Area = bounds.removeFromTop(layerHeight);
    auto layer3Area = bounds.removeFromTop(layerHeight);

    auto updateModules = [&](int layer) {
        activeLayer = layer;
        engineModule.setLayer(layer);
        scanModule.setLayer(layer);
        sprayModule.setLayer(layer);
        pitchModule.setLayer(layer);
        filterModule.setLayer(layer);
        spaceModule.setLayer(layer);
        distModule.setLayer(layer);
        envelopeModule.setLayer(layer);
        mixerModule1.setLayer(layer);
        };

    if (layer1Area.contains(event.getPosition())) {
        if (activeLayer != 1) { updateModules(1); repaint(); return; }
        float clickX = event.getPosition().x - layer1Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer1Area.getWidth());
        if (auto* p = audioProcessor.apvts.getParameter("L1_POSITION")) p->setValueNotifyingHost(ratioInView);
        repaint();
    }
    else if (layer2Area.contains(event.getPosition())) {
        if (activeLayer != 2) { updateModules(2); repaint(); return; }
        float clickX = event.getPosition().x - layer2Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer2Area.getWidth());
        if (auto* p = audioProcessor.apvts.getParameter("L2_POSITION")) p->setValueNotifyingHost(ratioInView);
        repaint();
    }
    else if (layer3Area.contains(event.getPosition())) {
        if (activeLayer != 3) { updateModules(3); repaint(); return; }
        float clickX = event.getPosition().x - layer3Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer3Area.getWidth());
        if (auto* p = audioProcessor.apvts.getParameter("L3_POSITION")) p->setValueNotifyingHost(ratioInView);
        repaint();
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

    if (layer1Area.contains(event.getPosition()) && activeLayer == 1) {
        float clickX = event.getPosition().x - layer1Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer1Area.getWidth());
        if (auto* p = audioProcessor.apvts.getParameter("L1_POSITION")) p->setValueNotifyingHost(ratioInView);
        repaint();
    }
    else if (layer2Area.contains(event.getPosition()) && activeLayer == 2) {
        float clickX = event.getPosition().x - layer2Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer2Area.getWidth());
        if (auto* p = audioProcessor.apvts.getParameter("L2_POSITION")) p->setValueNotifyingHost(ratioInView);
        repaint();
    }
    else if (layer3Area.contains(event.getPosition()) && activeLayer == 3) {
        float clickX = event.getPosition().x - layer3Area.getX();
        float ratioInView = juce::jlimit(0.0f, 1.0f, clickX / (float)layer3Area.getWidth());
        if (auto* p = audioProcessor.apvts.getParameter("L3_POSITION")) p->setValueNotifyingHost(ratioInView);
        repaint();
    }
}

void Granular_SynthAudioProcessorEditor::timerCallback() { repaint(); }

