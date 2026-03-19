/*
  ==============================================================================

    LfoModule.h
    Created: 19 Mar 2026 8:45:08pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// NUEVO: La estructura de datos para cada punto de tu LFO dibujable
struct LfoNode {
    float x;     // Posición horizontal (0.0 a 1.0)
    float y;     // Posición vertical (0.0 a 1.0)
    float curve; // Curva de tensión hacia el siguiente nodo (-1.0 a 1.0) -> ¡La usaremos pronto!
};

class LfoModule : public juce::Component, public juce::Timer
{
public:
    LfoModule(juce::AudioProcessorValueTreeState& apvts);
    ~LfoModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // --- NUEVO: Escuchamos al ratón ---
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    // Componentes visuales del LFO 1
    juce::Slider depthKnob, jitterKnob;
    juce::ComboBox waveSelector, beatSelector;

    // Conexiones al APVTS
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttach, jitterAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttach, beatAttach;

    // --- NUEVO: Variables del LFO 2 Dibujable ---
    std::vector<LfoNode> lfoNodes; // Aquí guardamos tu dibujo
    int draggedNode = -1;          // Saber qué punto estás agarrando con el ratón
    juce::Rectangle<int> lfo2Area; // Guardamos las coordenadas del lienzo para saber si clicas dentro

    // Funciones de dibujo modulares
    void drawLfo1(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawLfo2(juce::Graphics& g, juce::Rectangle<int> bounds); // NUEVO

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoModule)
};
