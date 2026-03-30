/*
  ==============================================================================

    EnvelopeModule.cpp
    Created: 18 Mar 2026 10:57:07pm
    Author:  franc

  ==============================================================================
*/

#include "EnvelopeModule.h"

EnvelopeModule::EnvelopeModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    startTimerHz(30);
}

EnvelopeModule::~EnvelopeModule() {}

void EnvelopeModule::timerCallback() { repaint(); }

void EnvelopeModule::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    auto ampArea = area.removeFromTop(area.getHeight() / 2);
    auto env2Area = area;

    // LECTURA DINÁMICA CON PREFIJO PARA EL AMP (Capa 1/2/3...)
    float aA = apvtsRef.getRawParameterValue(layerPrefix + "AMP_A")->load();
    float aD = apvtsRef.getRawParameterValue(layerPrefix + "AMP_D")->load();
    float aS = apvtsRef.getRawParameterValue(layerPrefix + "AMP_S")->load();
    float aR = apvtsRef.getRawParameterValue(layerPrefix + "AMP_R")->load();

    // LECTURA GLOBAL PARA EL ENV2 (SIN PREFIJO)
    float e2A = apvtsRef.getRawParameterValue("ENV2_A")->load();
    float e2D = apvtsRef.getRawParameterValue("ENV2_D")->load();
    float e2S = apvtsRef.getRawParameterValue("ENV2_S")->load();
    float e2R = apvtsRef.getRawParameterValue("ENV2_R")->load();

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(area.getX(), ampArea.getBottom(), area.getRight(), ampArea.getBottom(), 1.0f);

    juce::Colour layerColor = juce::Colours::cyan;
    juce::Colour globalColor = juce::Colour(0xffd0d0d0); // Gris Titanio

    drawEnvelope(g, ampArea.reduced(5), "AMP", aA, aD, aS, aR, layerColor);
    drawEnvelope(g, env2Area.reduced(5), "ENV 2", e2A, e2D, e2S, e2R, globalColor);
}

void EnvelopeModule::drawEnvelope(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String name,
    float a, float d, float s, float r, juce::Colour envColor)
{
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(name, bounds.withHeight(15), juce::Justification::topLeft, false);

    float totalVisualTime = 15.0f;
    float startX = bounds.getX();
    float bottomY = bounds.getBottom();
    float width = bounds.getWidth();
    float height = bounds.getHeight() - 15;
    float topY = bottomY - height;

    float attackX = startX + (width * (a / totalVisualTime));
    float decayX = attackX + (width * (d / totalVisualTime));
    float sustainX = decayX + (width * (2.0f / totalVisualTime));
    float releaseX = sustainX + (width * (r / totalVisualTime));

    releaseX = juce::jlimit(startX, (float)bounds.getRight(), releaseX);
    float sustainY = bottomY - (height * s);

    juce::Path envPath;
    envPath.startNewSubPath(startX, bottomY);
    envPath.lineTo(attackX, topY);
    envPath.lineTo(decayX, sustainY);
    envPath.lineTo(sustainX, sustainY);
    envPath.lineTo(releaseX, bottomY);

    g.setColour(envColor.withAlpha(0.2f));
    g.fillPath(envPath);
    g.setColour(envColor.withAlpha(0.9f));
    g.strokePath(envPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    float dotSize = 8.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(attackX - dotSize / 2, topY - dotSize / 2, dotSize, dotSize);
    g.fillEllipse(decayX - dotSize / 2, sustainY - dotSize / 2, dotSize, dotSize);
    g.fillEllipse(releaseX - dotSize / 2, bottomY - dotSize / 2, dotSize, dotSize);
}

void EnvelopeModule::resized() {}

void EnvelopeModule::mouseDown(const juce::MouseEvent& event)
{
    activeNode = -1;
    auto area = getLocalBounds();
    auto ampArea = area.removeFromTop(area.getHeight() / 2);
    auto env2Area = area;

    // AÑADIDO: 'isGlobal' para saber si le ponemos el prefijo de capa o no
    auto checkHits = [&](juce::Rectangle<int> bounds, juce::String baseParamName, int baseIndex, bool isGlobal)
        {
            juce::String prefixToUse = isGlobal ? "" : layerPrefix;

            float a = apvtsRef.getRawParameterValue(prefixToUse + baseParamName + "_A")->load();
            float d = apvtsRef.getRawParameterValue(prefixToUse + baseParamName + "_D")->load();
            float s = apvtsRef.getRawParameterValue(prefixToUse + baseParamName + "_S")->load();
            float r = apvtsRef.getRawParameterValue(prefixToUse + baseParamName + "_R")->load();

            float totalVisualTime = 15.0f;
            float startX = bounds.getX();
            float bottomY = bounds.getBottom();
            float width = bounds.getWidth();
            float height = bounds.getHeight() - 15;
            float topY = bottomY - height;

            float attackX = startX + (width * (a / totalVisualTime));
            float decayX = attackX + (width * (d / totalVisualTime));
            float sustainX = decayX + (width * (2.0f / totalVisualTime));
            float releaseX = sustainX + (width * (r / totalVisualTime));
            float sustainY = bottomY - (height * s);

            juce::Point<float> mousePos = event.position;

            if (mousePos.getDistanceFrom({ attackX, topY }) < 15.0f) { activeNode = baseIndex; startParamX = a; }
            else if (mousePos.getDistanceFrom({ decayX, sustainY }) < 15.0f) { activeNode = baseIndex + 1; startParamX = d; startParamY = s; }
            else if (mousePos.getDistanceFrom({ releaseX, bottomY }) < 15.0f) { activeNode = baseIndex + 2; startParamX = r; }
        };

    // AMP es de Capa (isGlobal = false), ENV2 es Global (isGlobal = true)
    checkHits(ampArea.reduced(5), "AMP", 0, false);
    checkHits(env2Area.reduced(5), "ENV2", 3, true);
}

void EnvelopeModule::mouseDrag(const juce::MouseEvent& event)
{
    if (activeNode == -1) return;

    float deltaX = event.getDistanceFromDragStartX();
    float deltaY = event.getDistanceFromDragStartY();

    float timeChange = deltaX * 0.02f;
    float sustainChange = -deltaY * 0.01f;

    // AÑADIDO: 'isGlobal' al actualizar el parámetro
    auto updateParam = [&](juce::String id, float startVal, float change, float min, float max, bool isGlobal) {
        juce::String prefixToUse = isGlobal ? "" : layerPrefix;
        if (auto* p = apvtsRef.getParameter(prefixToUse + id))
            p->setValueNotifyingHost(p->convertTo0to1(juce::jlimit(min, max, startVal + change)));
        };

    // AMP (isGlobal = false)
    if (activeNode == 0)      updateParam("AMP_A", startParamX, timeChange, 0.01f, 5.0f, false);
    else if (activeNode == 1) {
        updateParam("AMP_D", startParamX, timeChange, 0.01f, 5.0f, false);
        updateParam("AMP_S", startParamY, sustainChange, 0.0f, 1.0f, false);
    }
    else if (activeNode == 2) updateParam("AMP_R", startParamX, timeChange, 0.01f, 5.0f, false);

    // ENV 2 (isGlobal = true)
    else if (activeNode == 3) updateParam("ENV2_A", startParamX, timeChange, 0.01f, 5.0f, true);
    else if (activeNode == 4) {
        updateParam("ENV2_D", startParamX, timeChange, 0.01f, 5.0f, true);
        updateParam("ENV2_S", startParamY, sustainChange, 0.0f, 1.0f, true);
    }
    else if (activeNode == 5) updateParam("ENV2_R", startParamX, timeChange, 0.01f, 5.0f, true);
}

void EnvelopeModule::mouseUp(const juce::MouseEvent& event)
{
    activeNode = -1;
}
