/*
  ==============================================================================

    MasterModule.cpp
    Created: 23 Mar 2026 10:44:40pm
    Author:  franc

  ==============================================================================
*/

#include "MasterModule.h"

MasterModule::MasterModule(juce::AudioProcessorValueTreeState& apvts, Granular_SynthAudioProcessor& p)
    : apvtsRef(apvts), audioProcessor(p)
{
    // Función lambda para configurar los knobs rápido
    auto setupKnob = [this](juce::Slider& slider, const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramID, slider);
        };

    setupKnob(volKnob, "MASTER_VOL", volAttach);
    setupKnob(limitKnob, "LIMITER_THRESH", limitAttach);

    // --- NUEVO: COLOR GLOBAL (GRIS/BLANCO) PARA ESTOS KNOBS ---
    // Cambiamos el color de la barra de relleno del slider a un gris claro
    volKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightgrey);
    limitKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightgrey);

    volKnob.setColour(juce::Slider::thumbColourId, juce::Colours::white.withAlpha(0.9f));
    limitKnob.setColour(juce::Slider::thumbColourId, juce::Colours::white.withAlpha(0.9f));

    // Encendemos el reloj a 30 fotogramas por segundo para que las barras sean fluidas
    startTimerHz(30);
}

MasterModule::~MasterModule() {}

void MasterModule::timerCallback()
{
    // Leemos el volumen actual directamente desde el motor de audio
    float newLevelL = audioProcessor.visualMeterL.load();
    float newLevelR = audioProcessor.visualMeterR.load();

    // SUAVIZADO DE CAÍDA (Para que las barras no parpadeen de forma histérica)
    // Si el volumen sube, salta de golpe. Si baja, cae suavemente.
    if (newLevelL > currentLevelL) currentLevelL = newLevelL;
    else currentLevelL -= 2.0f; // Cae 2 decibelios por fotograma

    if (newLevelR > currentLevelR) currentLevelR = newLevelR;
    else currentLevelR -= 2.0f;

    // Limitamos a nuestro mínimo visual
    currentLevelL = juce::jmax(currentLevelL, -60.0f);
    currentLevelR = juce::jmax(currentLevelR, -60.0f);

    repaint(); // Repintar a 30 FPS
}

void MasterModule::resized()
{
    auto area = getLocalBounds().reduced(5);

    // Separamos la parte derecha para el Vúmetro (30 píxeles de ancho)
    meterArea = area.removeFromRight(30).reduced(2);

    // La parte izquierda se la quedan los Knobs
    auto knobsArea = area;

    // El volumen principal ocupa un poco más de espacio arriba
    volKnob.setBounds(knobsArea.removeFromTop(knobsArea.getHeight() * 0.6f).reduced(2).withTrimmedTop(15));

    // El techo del limitador abajo
    limitKnob.setBounds(knobsArea.reduced(2).withTrimmedTop(15));
}

void MasterModule::paint(juce::Graphics& g)
{
    // --- TEXTOS ---
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));

    g.drawText("VOL", volKnob.getX(), volKnob.getY() - 15, volKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("LIMIT", limitKnob.getX(), limitKnob.getY() - 15, limitKnob.getWidth(), 15, juce::Justification::centred);

    // ==========================================================
    // --- DIBUJAR VÚMETRO ESTÉREO ---
    // ==========================================================
    // Fondo oscuro para las barras
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(meterArea.toFloat(), 3.0f);

    // Borde sutil
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRoundedRectangle(meterArea.toFloat(), 3.0f, 1.0f);

    // Calculamos el espacio para las dos barras separadas
    float barWidth = (meterArea.getWidth() - 4) / 2.0f;
    juce::Rectangle<float> barL(meterArea.getX() + 1.5f, meterArea.getY() + 1.0f, barWidth, meterArea.getHeight() - 2.0f);
    juce::Rectangle<float> barR(meterArea.getX() + barWidth + 2.5f, meterArea.getY() + 1.0f, barWidth, meterArea.getHeight() - 2.0f);

    // Función para mapear Decibelios (-60 a 0) a altura en píxeles
    auto mapDbToHeight = [](float db, float maxHeight) {
        float normalized = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
        return normalized * maxHeight;
        };

    float hL = mapDbToHeight(currentLevelL, barL.getHeight());
    float hR = mapDbToHeight(currentLevelR, barR.getHeight());

    // Color Gris/Blanco global (¡como pediste!)
    g.setColour(juce::Colours::white.withAlpha(0.8f));

    // Dibujamos las barras rellenas desde abajo hacia arriba
    if (hL > 0) g.fillRoundedRectangle(barL.removeFromBottom(hL), 2.0f);
    if (hR > 0) g.fillRoundedRectangle(barR.removeFromBottom(hR), 2.0f);
}
