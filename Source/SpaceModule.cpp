#include "SpaceModule.h"

SpaceModule::SpaceModule(juce::AudioProcessorValueTreeState& apvts)
{
    // 1. Configuración básica de los knobs
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(sizeKnob);
    setupKnob(fbackKnob);
    setupKnob(mixKnob);

    // ====================================================================
    // 2. FORZAMOS COLOR PLATINO EN LOS PARAMETROS GLOBALES
    // ====================================================================
    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0);

    auto setKnobPlatino = [&](juce::Slider& k) {
        k.setColour(juce::Slider::rotarySliderOutlineColourId, titaniumColor.withAlpha(0.2f));
        k.setColour(juce::Slider::rotarySliderFillColourId, titaniumColor.withAlpha(0.8f));
        k.setColour(juce::Slider::thumbColourId, titaniumColor);
        };

    // Aplicamos gris solo al Size y al Feedback
    setKnobPlatino(sizeKnob);
    setKnobPlatino(fbackKnob);
    // (El mixKnob lo dejamos tal cual para que pille el color cyan de su capa)

    // 3. Conectamos al cerebro (APVTS)
    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_SIZE", sizeKnob);
    fbackAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_FBACK", fbackKnob);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "SPACE_MIX", mixKnob);
}

SpaceModule::~SpaceModule() {}

void SpaceModule::resized()
{
    auto area = getLocalBounds().reduced(10);
    int widthUnit = area.getWidth() / 3;
    sizeKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    fbackKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
    mixKnob.setBounds(area.removeFromLeft(widthUnit).reduced(5));
}
