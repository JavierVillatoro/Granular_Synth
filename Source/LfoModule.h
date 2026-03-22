#pragma once
#include <JuceHeader.h>

class Granular_SynthAudioProcessor;

// NUEVO PARADIGMA: Cada punto es un NODO VECTORIAL completo (Cubic Bézier)
struct LfoNode {
    juce::Point<float> pos;       // Posición principal del punto blanco (0.0 a 1.0)
    juce::Point<float> handleIn;  // Manecilla IZQUIERDA (Relativa a pos, ej. {-0.1, 0.0})
    juce::Point<float> handleOut; // Manecilla DERECHA (Relativa a pos, ej. {0.1, 0.0})
    bool isSmooth = true;         // ¿Las manecillas están alineadas o rotas?
};

class LfoModule : public juce::Component, public juce::Timer
{
public:
    LfoModule(juce::AudioProcessorValueTreeState& apvts);
    ~LfoModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // --- Escuchamos al ratón ---
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

    // --- NUEVO: Componente visual del LFO 2 ---
    juce::ComboBox beatSelector2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> beatAttach2;

    // --- Variables del LFO 2 Dibujable (Bézier) ---
    std::vector<LfoNode> lfoNodes;

    // Memoria del ratón
    int draggedNode = -1;
    bool draggingHandleIn = false; // ¿Estás tirando de la manecilla izquierda?
    bool draggingHandleOut = false;// ¿Estás tirando de la manecilla derecha?

    //CURVE
    int draggedCurve = -1;
    float initialHandleOutY = 0.0f;
    float initialHandleInY = 0.0f;

    juce::Rectangle<int> lfo2Area;

    // Funciones de dibujo modulares
    void drawLfo1(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawLfo2(juce::Graphics& g, juce::Rectangle<int> bounds);

    void bakeWavetable();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoModule)
};
