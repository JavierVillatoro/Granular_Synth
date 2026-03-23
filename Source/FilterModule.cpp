/*
  ==============================================================================

    FilterModule.cpp
    Created: 18 Mar 2026 9:10:29pm
    Author:  franc

  ==============================================================================
*/

#include "FilterModule.h"

FilterModule::FilterModule(juce::AudioProcessorValueTreeState& apvts) : apvtsRef(apvts)
{
    // Función lambda para no repetir código al configurar los knobs
    auto setupKnob = [this](juce::Slider& slider, const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramID, slider);
        };

    // Configuramos y conectamos los 4 knobs
    setupKnob(hpfKnob, "FILTER_HPF", hpfAttach);
    setupKnob(resHpfKnob, "FILTER_RES_HPF", resHpfAttach);

    setupKnob(lpfKnob, "FILTER_LPF", lpfAttach);
    setupKnob(resLpfKnob, "FILTER_RES_LPF", resLpfAttach);
}

FilterModule::~FilterModule() {}

void FilterModule::resized()
{
    auto area = getLocalBounds();

    // Ya no quitamos los 25px de arriba porque borramos el título "FILTER" en el Editor.
    // Usamos todo el espacio disponible.

    // Margen general muy pequeño para maximizar el tamaño de los knobs
    area = area.reduced(2);

    // --- CORTE DERECHO (LPF / RES LPF) ---
    // Les damos 60px de ancho para que los knobs sean grandes
    auto rightCol = area.removeFromRight(60).reduced(2);

    // Mitad superior el knob de corte, mitad inferior la resonancia
    lpfKnob.setBounds(rightCol.removeFromTop(rightCol.getHeight() / 2).reduced(2));
    resLpfKnob.setBounds(rightCol.reduced(2));

    // --- CORTE IZQUIERDO (HPF / RES HPF) ---
    auto leftCol = area.removeFromLeft(60).reduced(2);
    hpfKnob.setBounds(leftCol.removeFromTop(leftCol.getHeight() / 2).reduced(2));
    resHpfKnob.setBounds(leftCol.reduced(2));

    // --- CENTRAR CUADRO GRÁFICO ---
    // Lo que queda en el centro es nuestra pantalla gigante para dibujar.
    // Usamos jmin para asegurar que sea cuadrado o proporcionado, y lo centramos.
    int graphW = juce::jmin(area.getWidth(), 200); // Máximo 200px de ancho
    int graphH = area.getHeight() - 10;           // Casi toda la altura

    graphArea = juce::Rectangle<int>(0, 0, graphW, graphH).withCentre(area.getCentre());
}

// ... (deja tu función paint tal y como la tengas por ahora, la cambiaremos en el próximo paso)
// ... (añade las funciones mouseDown y mouseDrag vacías al final si no las tenías)
void FilterModule::mouseDown(const juce::MouseEvent& event) {}
void FilterModule::mouseDrag(const juce::MouseEvent& event) {}

// Pégalo al final de FilterModule.cpp

void FilterModule::paint(juce::Graphics& g)
{
    // Dibujamos un rectángulo suave para delimitar la zona de la pantalla interactiva
    g.setColour(juce::Colours::cyan.withAlpha(0.2f));
    g.drawRoundedRectangle(graphArea.toFloat(), 5.0f, 2.0f);

    // Etiqueta temporal para que sepas qué es cada miniknob
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(10.0f);

    // (Opcional) Puedes dibujar textitos al lado de los knobs si quieres, 
    // pero por ahora con ver la caja central nos vale.
}
