/*
  ==============================================================================

    FilterModule.cpp
    Created: 18 Mar 2026 9:10:29pm
    Author:  franc

  ==============================================================================
*/

#include "FilterModule.h"

FilterModule::FilterModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupKnob = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        };

    setupKnob(hpfKnob);
    setupKnob(resHpfKnob);
    setupKnob(lpfKnob);
    setupKnob(resLpfKnob);

    setLayer(1);
    startTimerHz(30);
}

FilterModule::~FilterModule() {}

void FilterModule::setLayer(int layerIndex)
{
    currentLayer = layerIndex; // Guardamos para la función paint()

    juce::Colour layerColor;
    juce::Colour dotColor;

    if (layerIndex == 1) {
        layerPrefix = "L1_";
        layerColor = juce::Colours::cyan;
        dotColor = juce::Colours::white;
    }
    else if (layerIndex == 2) {
        layerPrefix = "L2_";
        layerColor = juce::Colours::magenta;
        dotColor = juce::Colours::pink;
    }
    else if (layerIndex == 3) {
        layerPrefix = "L3_";
        layerColor = juce::Colours::orange;
        dotColor = juce::Colours::whitesmoke;
    }

    hpfKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    resHpfKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    lpfKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);
    resLpfKnob.setColour(juce::Slider::rotarySliderFillColourId, layerColor);

    hpfKnob.setColour(juce::Slider::thumbColourId, dotColor);
    resHpfKnob.setColour(juce::Slider::thumbColourId, dotColor);
    lpfKnob.setColour(juce::Slider::thumbColourId, dotColor);
    resLpfKnob.setColour(juce::Slider::thumbColourId, dotColor);

    hpfAttach.reset(); resHpfAttach.reset();
    lpfAttach.reset(); resLpfAttach.reset();

    hpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "FILTER_HPF", hpfKnob);
    resHpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "FILTER_RES_HPF", resHpfKnob);
    lpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "FILTER_LPF", lpfKnob);
    resLpfAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "FILTER_RES_LPF", resLpfKnob);

    repaint(); // Forzamos a dibujar la curva con el nuevo color al instante
}

void FilterModule::timerCallback() { repaint(); }

void FilterModule::resized()
{
    auto area = getLocalBounds().reduced(2);
    auto leftCol = area.removeFromLeft(60);
    auto rightCol = area.removeFromRight(60);

    auto hpfArea = leftCol.removeFromTop(leftCol.getHeight() / 2);
    hpfKnob.setBounds(hpfArea.withTrimmedTop(15).reduced(2));

    auto resHpfArea = leftCol;
    resHpfKnob.setBounds(resHpfArea.withTrimmedTop(15).reduced(2));

    auto lpfArea = rightCol.removeFromTop(rightCol.getHeight() / 2);
    lpfKnob.setBounds(lpfArea.withTrimmedTop(15).reduced(2));

    auto resLpfArea = rightCol;
    resLpfKnob.setBounds(resLpfArea.withTrimmedTop(15).reduced(2));

    graphArea = area.reduced(5);
}

void FilterModule::mouseDown(const juce::MouseEvent& event)
{
    if (!graphArea.contains(event.x, event.y)) return;

    auto getDotX = [&](float freq) {
        float minF = 20.0f, maxF = 20000.0f;
        float proportion = std::log10(freq / minF) / std::log10(maxF / minF);
        return graphArea.getX() + proportion * graphArea.getWidth();
        };

    auto getDotY = [&](float qVal) {
        float qDb = 20.0f * std::log10(juce::jmax(qVal, 0.707f));
        float yNorm = juce::jmap(qDb, -2.0f, 15.0f, 0.8f, 0.0f);
        return graphArea.getY() + (yNorm * graphArea.getHeight());
        };

    float baseHpf = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_HPF")->load();
    float baseHRes = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_HPF")->load();
    float baseLpf = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_LPF")->load();
    float baseLRes = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_LPF")->load();

    juce::Point<float> hpfDot(getDotX(baseHpf), getDotY(baseHRes));
    juce::Point<float> lpfDot(getDotX(baseLpf), getDotY(baseLRes));

    juce::Point<float> mousePos((float)event.x, (float)event.y);
    float hitRadius = 20.0f;

    if (mousePos.getDistanceFrom(hpfDot) < hitRadius) draggedDot = 0;
    else if (mousePos.getDistanceFrom(lpfDot) < hitRadius) draggedDot = 1;
    else draggedDot = -1;
}

void FilterModule::mouseDrag(const juce::MouseEvent& event)
{
    if (draggedDot == -1) return;

    float proportionX = (float)(event.x - graphArea.getX()) / (float)graphArea.getWidth();
    float proportionY = (float)(event.y - graphArea.getY()) / (float)graphArea.getHeight();

    proportionX = juce::jlimit(0.0f, 1.0f, proportionX);
    proportionY = juce::jlimit(0.0f, 1.0f, proportionY);

    float newFreq = 20.0f * std::pow(1000.0f, proportionX);
    newFreq = juce::jlimit(20.0f, 20000.0f, newFreq);

    float newRes = juce::jmap(1.0f - proportionY, 0.0f, 1.0f, 0.707f, 2.5f);
    newRes = juce::jlimit(0.707f, 2.5f, newRes);

    if (draggedDot == 0) {
        hpfKnob.setValue(newFreq);
        resHpfKnob.setValue(newRes);
    }
    else if (draggedDot == 1) {
        lpfKnob.setValue(newFreq);
        resLpfKnob.setValue(newRes);
    }
}

void FilterModule::paint(juce::Graphics& g)
{
    // MAGIA: El color cambia con la variable 'currentLayer'
    juce::Colour layerColor;
    if (currentLayer == 1) layerColor = juce::Colours::cyan;
    else if (currentLayer == 2) layerColor = juce::Colours::magenta;
    else if (currentLayer == 3) layerColor = juce::Colours::orange;

    g.setColour(layerColor.withAlpha(0.05f));
    g.fillRoundedRectangle(graphArea.toFloat(), 5.0f);
    g.setColour(layerColor.withAlpha(0.3f));
    g.drawRoundedRectangle(graphArea.toFloat(), 5.0f, 1.5f);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("HPF", hpfKnob.getX(), hpfKnob.getY() - 15, hpfKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("H.RES", resHpfKnob.getX(), resHpfKnob.getY() - 15, resHpfKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("LPF", lpfKnob.getX(), lpfKnob.getY() - 15, lpfKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("L.RES", resLpfKnob.getX(), resLpfKnob.getY() - 15, resLpfKnob.getWidth(), 15, juce::Justification::centred);

    float hpfVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_HPF")->load();
    float hResVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_HPF")->load();
    float lpfVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_LPF")->load();
    float lResVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_LPF")->load();

    juce::Path filterCurve;
    filterCurve.startNewSubPath(graphArea.getX(), graphArea.getBottom());

    for (int x = 0; x <= graphArea.getWidth(); x += 2)
    {
        float proportion = (float)x / (float)graphArea.getWidth();
        float currentFreq = 20.0f * std::pow(1000.0f, proportion);
        float gTotal = 1.0f;

        float hQVal = juce::jmap(hResVal, 0.707f, 2.5f, 0.707f, 8.0f);
        float hRatio = currentFreq / hpfVal;
        float hRatioSq = hRatio * hRatio;
        float hDenom = std::sqrt(std::pow(1.0f - hRatioSq, 2.0f) + (hRatioSq / (hQVal * hQVal)));
        float hGainMagnitude = (hDenom > 1e-6f) ? (hRatioSq / hDenom) : 0.0f;
        gTotal *= hGainMagnitude;

        float lQVal = juce::jmap(lResVal, 0.707f, 2.5f, 0.707f, 8.0f);
        float lRatio = currentFreq / lpfVal;
        float lRatioSq = lRatio * lRatio;
        float lDenom = std::sqrt(std::pow(1.0f - lRatioSq, 2.0f) + (lRatioSq / (lQVal * lQVal)));
        float lGainMagnitude = (lDenom > 1e-6f) ? (1.0f / lDenom) : 1.0f;
        gTotal *= lGainMagnitude;

        float totalDb = 20.0f * std::log10(juce::jmax(gTotal, 1e-6f));
        totalDb = juce::jlimit(-60.0f, 20.0f, totalDb);

        float yNorm = juce::jmap(totalDb, -60.0f, 20.0f, 1.0f, 0.0f);
        float yPixel = graphArea.getY() + (yNorm * graphArea.getHeight());
        yPixel = juce::jlimit((float)graphArea.getY(), (float)graphArea.getBottom(), yPixel);

        filterCurve.lineTo(graphArea.getX() + x, yPixel);
    }

    filterCurve.lineTo(graphArea.getRight(), graphArea.getBottom());
    filterCurve.closeSubPath();

    g.setColour(layerColor.withAlpha(0.4f));
    g.fillPath(filterCurve);

    g.setColour(layerColor.brighter());
    g.strokePath(filterCurve, juce::PathStrokeType(2.0f));

    auto getDotX = [&](float freq) {
        float minF = 20.0f, maxF = 20000.0f;
        float proportion = std::log10(freq / minF) / std::log10(maxF / minF);
        return graphArea.getX() + proportion * graphArea.getWidth();
        };

    auto getDotY = [&](float qVal) {
        float qDb = 20.0f * std::log10(juce::jmax(qVal, 0.707f));
        float yNorm = juce::jmap(qDb, -2.0f, 15.0f, 0.8f, 0.0f);
        return graphArea.getY() + (yNorm * graphArea.getHeight());
        };

    juce::Point<float> hpfDot(getDotX(hpfVal), getDotY(hResVal));
    juce::Point<float> lpfDot(getDotX(lpfVal), getDotY(lResVal));

    float dotRadius = 5.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(hpfDot.getX() - dotRadius, hpfDot.getY() - dotRadius, dotRadius * 2, dotRadius * 2);
    g.fillEllipse(lpfDot.getX() - dotRadius, lpfDot.getY() - dotRadius, dotRadius * 2, dotRadius * 2);
}
