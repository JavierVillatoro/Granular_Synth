/*
  ==============================================================================

    LayerMixerModule.cpp
    Created: 28 Mar 2026 5:44:39pm
    Author:  franc

  ==============================================================================
*/

#include <JuceHeader.h>
#include "LayerMixerModule.h"

LayerMixerModule::LayerMixerModule(juce::AudioProcessorValueTreeState& apvts, juce::String layerPrefix)
    : apvtsRef(apvts), prefix(layerPrefix)
{
    auto setupEqKnob = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        };

    setupEqKnob(eqLow);
    setupEqKnob(eqMidLow);
    setupEqKnob(eqMidHigh);
    setupEqKnob(eqHigh);

    volumeFader.setSliderStyle(juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
    volumeFader.setTextValueSuffix(" dB");
    volumeFader.setColour(juce::Slider::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
    volumeFader.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(volumeFader);

    setLayer(1); // Arranca en Capa 1
}

LayerMixerModule::~LayerMixerModule() {}

void LayerMixerModule::setLayer(int layerIndex)
{
    currentLayer = layerIndex;

    juce::Colour layerColor;
    juce::Colour dotColor;

    if (layerIndex == 1) {
        prefix = "L1_";
        layerColor = juce::Colours::cyan;
        dotColor = juce::Colours::white;
    }
    else if (layerIndex == 2) {
        prefix = "L2_";
        layerColor = juce::Colours::magenta;
        dotColor = juce::Colours::pink;
    }
    else if (layerIndex == 3) {
        prefix = "L3_";
        layerColor = juce::Colours::orange;
        dotColor = juce::Colours::whitesmoke; // Whitesmoke para el contraste perfecto
    }

    eqLow.setColour(juce::Slider::rotarySliderFillColourId, layerColor.withAlpha(0.8f));
    eqLow.setColour(juce::Slider::thumbColourId, dotColor);
    eqMidLow.setColour(juce::Slider::rotarySliderFillColourId, layerColor.withAlpha(0.8f));
    eqMidLow.setColour(juce::Slider::thumbColourId, dotColor);
    eqMidHigh.setColour(juce::Slider::rotarySliderFillColourId, layerColor.withAlpha(0.8f));
    eqMidHigh.setColour(juce::Slider::thumbColourId, dotColor);
    eqHigh.setColour(juce::Slider::rotarySliderFillColourId, layerColor.withAlpha(0.8f));
    eqHigh.setColour(juce::Slider::thumbColourId, dotColor);

    volumeFader.setColour(juce::Slider::trackColourId, layerColor.withAlpha(0.6f));
    volumeFader.setColour(juce::Slider::thumbColourId, layerColor);

    attachLow.reset();
    attachMidLow.reset();
    attachMidHigh.reset();
    attachHigh.reset();
    attachVol.reset();

    attachLow = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_LOW", eqLow);
    attachMidLow = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_MID_LOW", eqMidLow);
    attachMidHigh = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_MID_HIGH", eqMidHigh);
    attachHigh = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "EQ_HIGH", eqHigh);
    attachVol = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, prefix + "MIX_VOL", volumeFader);

    repaint();
}

void LayerMixerModule::paint(juce::Graphics& g)
{
    juce::Colour layerColor;
    if (currentLayer == 1) layerColor = juce::Colours::cyan;
    else if (currentLayer == 2) layerColor = juce::Colours::magenta;
    else if (currentLayer == 3) layerColor = juce::Colours::orange;

    g.fillAll(juce::Colour(0xff121212));
    g.setColour(layerColor.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);

    float zeroY = volumeFader.getY() + volumeFader.getPositionOfValue(0.0);

    g.setColour(layerColor.withAlpha(0.9f));
    g.drawLine((float)volumeFader.getX() - 3.0f, zeroY, (float)volumeFader.getRight() + 3.0f, zeroY, 2.0f);

    g.setFont(11.0f);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawText("HIGH", eqHigh.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("MID-H", eqMidHigh.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("MID-L", eqMidLow.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("LOW", eqLow.getBounds().translated(0, 22), juce::Justification::centred, false);

    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(layerColor);
    juce::String volText = juce::String(volumeFader.getValue(), 1) + " dB";
    g.drawText(volText, volumeFader.getX() - 10, volumeFader.getBottom() + 2, volumeFader.getWidth() + 20, 15, juce::Justification::centred);
}

void LayerMixerModule::resized()
{
    auto area = getLocalBounds();
    auto leftSection = area.removeFromLeft(area.getWidth() * 0.4f).reduced(5);

    auto faderArea = leftSection.removeFromRight(leftSection.getWidth() * 0.35f);
    volumeFader.setBounds(faderArea.reduced(2, 12));

    auto eqArea = leftSection;
    int knobHeight = eqArea.getHeight() / 4;

    eqHigh.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqMidHigh.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqMidLow.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqLow.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
}
