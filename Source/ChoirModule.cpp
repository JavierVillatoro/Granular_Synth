/*
  ==============================================================================

    ChoirModule.cpp
    Created: 11 Apr 2026 10:10:19pm
    Author:  franc

  ==============================================================================
*/

#include "ChoirModule.h"

ChoirModule::ChoirModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    auto setupBar = [this](juce::Slider& s, juce::String name) {
        s.setSliderStyle(juce::Slider::LinearBar);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        // Usamos el textValueSuffix temporalmente como nombre a mostrar
        s.setTextValueSuffix(" " + name);
        addAndMakeVisible(s);
        };

    setupBar(haloPitch, "Pitch");
    setupBar(haloShimmer, "Shimmer");
    setupBar(haloColor, "Color");
    setupBar(haloMix, "Mix");

    setupBar(ensRate, "Rate");
    setupBar(ensDepth, "Depth");
    setupBar(ensWidth, "Width");
    setupBar(ensMix, "Mix");

    setLayer(1);
}

ChoirModule::~ChoirModule() {}

void ChoirModule::paint(juce::Graphics& g)
{
    // Títulos de las secciones dibujados sutilmente al fondo
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));

    auto area = getLocalBounds();
    auto haloArea = area.removeFromTop(area.getHeight() / 2);
    auto ensArea = area;

    g.drawText("HALO", haloArea.withTrimmedTop(2), juce::Justification::centredTop);
    g.drawText("ENSEMBLE", ensArea.withTrimmedTop(2), juce::Justification::centredTop);
}

void ChoirModule::setLayer(int layerIndex)
{
    if (layerIndex == 1) {
        layerPrefix = "L1_";
        currentLayerColor = juce::Colours::cyan;
    }
    else if (layerIndex == 2) {
        layerPrefix = "L2_";
        currentLayerColor = juce::Colours::magenta;
    }
    else if (layerIndex == 3) {
        layerPrefix = "L3_";
        currentLayerColor = juce::Colours::orange;
    }
    else if (layerIndex == 4) {
        layerPrefix = "L4_";
        currentLayerColor = juce::Colours::lime;
    }

    auto colorizeBar = [this](juce::Slider& s) {
        s.setColour(juce::Slider::trackColourId, currentLayerColor.withAlpha(0.6f)); // Barra de llenado
        s.setColour(juce::Slider::backgroundColourId, juce::Colours::black.withAlpha(0.3f)); // Fondo vacío
        };

    colorizeBar(haloPitch); colorizeBar(haloShimmer); colorizeBar(haloColor); colorizeBar(haloMix);
    colorizeBar(ensRate); colorizeBar(ensDepth); colorizeBar(ensWidth); colorizeBar(ensMix);

    // Reconectar cables
    attachHPitch.reset(); attachHShimmer.reset(); attachHColor.reset(); attachHMix.reset();
    attachERate.reset(); attachEDepth.reset(); attachEWidth.reset(); attachEMix.reset();

    attachHPitch = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "HALO_PITCH", haloPitch);
    attachHShimmer = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "HALO_SHIMMER", haloShimmer);
    //attachHColor = std::make_unique<juce::AudioParameterFloat::SliderAttachment>(apvtsRef, layerPrefix + "HALO_COLOR", haloColor);
    attachHColor = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "HALO_COLOR", haloColor);
    attachHMix = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "HALO_MIX", haloMix);

    attachERate = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "ENS_RATE", ensRate);
    attachEDepth = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "ENS_DEPTH", ensDepth);
    attachEWidth = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "ENS_WIDTH", ensWidth);
    attachEMix = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, layerPrefix + "ENS_MIX", ensMix);

    repaint();
}

void ChoirModule::resized()
{
    auto area = getLocalBounds().reduced(4);

    // Dividimos por la mitad
    auto haloArea = area.removeFromTop(area.getHeight() / 2).withTrimmedTop(18); // Dejamos espacio para el título HALO
    auto ensArea = area.withTrimmedTop(18); // Espacio para el título ENSEMBLE

    // Altura de cada barra
    int barHeight = haloArea.getHeight() / 4;

    haloPitch.setBounds(haloArea.removeFromTop(barHeight).reduced(0, 2));
    haloShimmer.setBounds(haloArea.removeFromTop(barHeight).reduced(0, 2));
    haloColor.setBounds(haloArea.removeFromTop(barHeight).reduced(0, 2));
    haloMix.setBounds(haloArea.removeFromTop(barHeight).reduced(0, 2));

    ensRate.setBounds(ensArea.removeFromTop(barHeight).reduced(0, 2));
    ensDepth.setBounds(ensArea.removeFromTop(barHeight).reduced(0, 2));
    ensWidth.setBounds(ensArea.removeFromTop(barHeight).reduced(0, 2));
    ensMix.setBounds(ensArea.removeFromTop(barHeight).reduced(0, 2));
}
