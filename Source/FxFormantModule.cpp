/*
  ==============================================================================

    FxFormantModule.cpp
    Created: 10 Apr 2026 12:18:11pm
    Author:  franc

  ==============================================================================
*/

#include "FxFormantModule.h"

// ==============================================================================
// PAD XY
// ==============================================================================
XYPad::XYPad(juce::AudioProcessorValueTreeState& apvts, juce::String paramX, juce::String paramY)
    : apvtsRef(apvts), currentParamX(paramX), currentParamY(paramY) {
}

XYPad::~XYPad() {}

void XYPad::updateParameters(juce::String newParamX, juce::String newParamY)
{
    currentParamX = newParamX;
    currentParamY = newParamY;
}

void XYPad::setColors(juce::Colour newBaseColor)
{
    baseColor = newBaseColor;
}

void XYPad::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // 1. Pantallita de tono suave (parecido al fondo de tus filtros)
    g.setColour(baseColor.withAlpha(0.15f));
    g.fillRoundedRectangle(area, 4.0f);

    // Borde sutil
    g.setColour(baseColor.withAlpha(0.4f));
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    // Retícula central para guiar la vista
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(area.getWidth() / 2.0f, 0.0f, area.getWidth() / 2.0f, area.getHeight(), 1.0f);
    g.drawLine(0.0f, area.getHeight() / 2.0f, area.getWidth(), area.getHeight() / 2.0f, 1.0f);

    // 2. Bolita del color del layer
    if (apvtsRef.getParameter(currentParamX) != nullptr && apvtsRef.getParameter(currentParamY) != nullptr)
    {
        float xVal = apvtsRef.getRawParameterValue(currentParamX)->load();
        float yVal = apvtsRef.getRawParameterValue(currentParamY)->load();

        float dotX = xVal * area.getWidth();
        float dotY = (1.0f - yVal) * area.getHeight(); // Y invertida (1.0 arriba)
        float radius = 5.0f;

        // Aura brillante
        g.setColour(baseColor.withAlpha(0.3f));
        g.fillEllipse(dotX - (radius * 2.0f), dotY - (radius * 2.0f), radius * 4.0f, radius * 4.0f);

        // Núcleo sólido de la bolita
        g.setColour(baseColor);
        g.fillEllipse(dotX - radius, dotY - radius, radius * 2.0f, radius * 2.0f);
    }
}

void XYPad::mouseDown(const juce::MouseEvent& e) { mouseDrag(e); }

void XYPad::mouseDrag(const juce::MouseEvent& e)
{
    float x = juce::jlimit(0.0f, 1.0f, (float)e.x / (float)getWidth());
    float y = juce::jlimit(0.0f, 1.0f, 1.0f - ((float)e.y / (float)getHeight()));

    if (auto* pX = apvtsRef.getParameter(currentParamX))
        pX->setValueNotifyingHost(x);

    if (auto* pY = apvtsRef.getParameter(currentParamY))
        pY->setValueNotifyingHost(y);
}


// ==============================================================================
// MÓDULO VOWEL PRINCIPAL
// ==============================================================================
FxFormantModule::FxFormantModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix), xyPad(apvts, prefix + "VOWEL_X", prefix + "VOWEL_Y")
{
    addAndMakeVisible(xyPad);

    // Barra que se rellena horizontalmente
    mixSlider.setSliderStyle(juce::Slider::LinearBar);
    mixSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(mixSlider);

    setLayer(1);
    startTimerHz(30);
}

FxFormantModule::~FxFormantModule() {}

void FxFormantModule::setLayer(int layerIndex)
{
    currentLayer = layerIndex;

    if (layerIndex == 1) { layerPrefix = "L1_"; currentBaseColor = juce::Colours::cyan; }
    else if (layerIndex == 2) { layerPrefix = "L2_"; currentBaseColor = juce::Colours::magenta; }
    else if (layerIndex == 3) { layerPrefix = "L3_"; currentBaseColor = juce::Colours::orange; }
    else if (layerIndex == 4) { layerPrefix = "L4_"; currentBaseColor = juce::Colours::lime; }

    xyPad.updateParameters(layerPrefix + "VOWEL_X", layerPrefix + "VOWEL_Y");
    xyPad.setColors(currentBaseColor);

    // 3. La barra de Mix se llena con el color del layer seleccionado
    mixSlider.setColour(juce::Slider::trackColourId, currentBaseColor.withAlpha(0.6f));
    mixSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::black.withAlpha(0.3f));

    mixAttach.reset();
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "VOWEL_MIX", mixSlider);

    repaint();
}

void FxFormantModule::timerCallback()
{
    xyPad.repaint();
}

void FxFormantModule::paint(juce::Graphics& g)
{
    // Opcional: Escribimos "MIX" chiquito encima del fader o lo dejamos super limpio.
    // Como el LinearBar es muy intuitivo, con el diseño actual queda hiper minimalista.
}

void FxFormantModule::resized()
{
    auto area = getLocalBounds().reduced(4); // Pequeño margen general

    // Cortamos 12 píxeles por abajo para la barra de Mix
    auto mixArea = area.removeFromBottom(12);
    mixSlider.setBounds(mixArea);

    // Dejamos un poco de espacio respirable entre el Pad y la barra
    area.removeFromBottom(4);

    // El resto es para la pantalla XY
    xyPad.setBounds(area);
}
