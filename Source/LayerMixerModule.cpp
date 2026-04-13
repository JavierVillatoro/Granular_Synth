/*
  ==============================================================================

    LayerMixerModule.cpp

  ==============================================================================
*/

#include <JuceHeader.h>
#include "LayerMixerModule.h"

// =============================================================================
// --- ESTILO DEL FADER (El Sándwich Visual Corregido) ---
// =============================================================================
class MixerFaderStyle : public juce::LookAndFeel_V4 {
public:
    void drawLinearSliderBackground(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto layerColor = slider.findColour(juce::Slider::thumbColourId);
        auto trackColor = slider.findColour(juce::Slider::trackColourId);

        // 1. Carril oscuro
        float trackWidth = 4.0f; // Más fino y elegante
        float trackX = x + (width - trackWidth) * 0.5f;

        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(trackX, (float)y, trackWidth, (float)height, 2.0f);

        // 2. Relleno inferior del color de la capa
        g.setColour(trackColor);
        g.fillRoundedRectangle(trackX, sliderPos, trackWidth, (y + height) - sliderPos, 2.0f);

        // 3. LA LÍNEA DE 0dB (Asegurada dentro de los márgenes)
        float zeroY = slider.getPositionOfValue(0.0);
        g.setColour(layerColor.withAlpha(0.9f));

        // La dibujamos de lado a lado del fader, dejando 2px de margen para que JUCE no la recorte
        g.drawLine((float)x + 2.0f, zeroY, (float)x + width - 2.0f, zeroY, 2.0f);
    }

    void drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto layerColor = slider.findColour(juce::Slider::thumbColourId);

        // Capuchón ancho (estilo mesa)
        float thumbW = width * 0.6f;
        float thumbH = 12.0f;
        float thumbX = x + (width - thumbW) * 0.5f;
        juce::Rectangle<float> thumb(thumbX, sliderPos - thumbH * 0.5f, thumbW, thumbH);

        // Sombra
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(thumb.translated(0.0f, 2.0f), 2.0f);

        // Botón
        g.setColour(layerColor);
        g.fillRoundedRectangle(thumb, 2.0f);

        // Marca blanca en el centro
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillRect(thumbX + 4.0f, sliderPos - 1.0f, thumbW - 8.0f, 2.0f);
    }
};

// =============================================================================
// --- CLASE PRINCIPAL ---
// =============================================================================

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

    // Le ponemos su traje a medida
    faderStyle = std::make_unique<MixerFaderStyle>();
    volumeFader.setLookAndFeel(faderStyle.get());

    volumeFader.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    volumeFader.setColour(juce::Slider::textBoxTextColourId, juce::Colours::transparentBlack);

    addAndMakeVisible(volumeFader);

    setLayer(1);
}

LayerMixerModule::~LayerMixerModule() {
    volumeFader.setLookAndFeel(nullptr);
}

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
        dotColor = juce::Colours::whitesmoke;
    }
    else if (layerIndex == 4) {
        prefix = "L4_";
        layerColor = juce::Colours::lime;
        dotColor = juce::Colours::lightgrey.withAlpha(0.9f);
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
    else if (currentLayer == 4) layerColor = juce::Colours::lime;

    g.fillAll(juce::Colour(0xff121212));
    g.setColour(layerColor.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);

    // NOTA: La línea de 0dB ahora la dibuja el LookAndFeel del fader.

    g.setFont(11.0f);
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.drawText("HIGH", eqHigh.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("MID-H", eqMidHigh.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("MID-L", eqMidLow.getBounds().translated(0, 22), juce::Justification::centred, false);
    g.drawText("LOW", eqLow.getBounds().translated(0, 22), juce::Justification::centred, false);

    // VOLVEMOS A TU CÓDIGO ORIGINAL PARA EL TEXTO
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(layerColor);
    juce::String volText = juce::String(volumeFader.getValue(), 1) + " dB"; // Mantiene tu decimal
    g.drawText(volText, volumeFader.getX() - 10, volumeFader.getBottom() + 2, volumeFader.getWidth() + 20, 15, juce::Justification::centred);
}

void LayerMixerModule::resized()
{
    // VOLVEMOS A TUS DIMENSIONES ORIGINALES
    auto area = getLocalBounds().reduced(5);

    auto faderArea = area.removeFromRight(area.getWidth() * 0.35f);
    volumeFader.setBounds(faderArea.reduced(2, 12));

    auto eqArea = area;
    int knobHeight = eqArea.getHeight() / 4;

    eqHigh.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqMidHigh.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqMidLow.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
    eqLow.setBounds(eqArea.removeFromTop(knobHeight).reduced(3));
}