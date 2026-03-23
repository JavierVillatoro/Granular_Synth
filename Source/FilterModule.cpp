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

    startTimerHz(30);
}

void FilterModule::timerCallback()
{
    repaint(); // Le dice a JUCE: "¡Borra la pantalla y vuelve a pintarla ya!"
}

FilterModule::~FilterModule() {}

void FilterModule::resized()
{
    auto area = getLocalBounds().reduced(2);

    // Columnas laterales más anchas (60px) para que quepan los knobs y las letras
    auto leftCol = area.removeFromLeft(60);
    auto rightCol = area.removeFromRight(60);

    // ==========================================
    // IZQUIERDA: HPF y H.RES
    // ==========================================
    // Dividimos la columna en dos mitades (arriba y abajo)
    auto hpfArea = leftCol.removeFromTop(leftCol.getHeight() / 2);
    // Le quitamos 15px por arriba al knob para dejarle hueco a la etiqueta de texto
    hpfKnob.setBounds(hpfArea.withTrimmedTop(15).reduced(2));

    auto resHpfArea = leftCol;
    resHpfKnob.setBounds(resHpfArea.withTrimmedTop(15).reduced(2));

    // ==========================================
    // DERECHA: LPF y L.RES
    // ==========================================
    auto lpfArea = rightCol.removeFromTop(rightCol.getHeight() / 2);
    lpfKnob.setBounds(lpfArea.withTrimmedTop(15).reduced(2));

    auto resLpfArea = rightCol;
    resLpfKnob.setBounds(resLpfArea.withTrimmedTop(15).reduced(2));

    // ==========================================
    // CENTRO: LA GRÁFICA
    // ==========================================
    graphArea = area.reduced(5);
}

// ... (deja tu función paint tal y como la tengas por ahora, la cambiaremos en el próximo paso)
// ... (añade las funciones mouseDown y mouseDrag vacías al final si no las tenías)
void FilterModule::mouseDown(const juce::MouseEvent& event) {}
void FilterModule::mouseDrag(const juce::MouseEvent& event) {}

// Pégalo al final de FilterModule.cpp

void FilterModule::paint(juce::Graphics& g)
{
    // 1. FONDO DE LA PANTALLA
    g.setColour(juce::Colours::cyan.withAlpha(0.05f));
    g.fillRoundedRectangle(graphArea.toFloat(), 5.0f);
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRoundedRectangle(graphArea.toFloat(), 5.0f, 1.5f);

    // 2. TEXTOS DE LOS KNOBS
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("HPF", hpfKnob.getX(), hpfKnob.getY() - 15, hpfKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("H.RES", resHpfKnob.getX(), resHpfKnob.getY() - 15, resHpfKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("LPF", lpfKnob.getX(), lpfKnob.getY() - 15, lpfKnob.getWidth(), 15, juce::Justification::centred);
    g.drawText("L.RES", resLpfKnob.getX(), resLpfKnob.getY() - 15, resLpfKnob.getWidth(), 15, juce::Justification::centred);

    // ==========================================================
    // 3. MATEMÁTICA ANALÓGICA DEL FILTRO (Curvas suaves)
    // ==========================================================
    float hpfVal = apvtsRef.getRawParameterValue("FILTER_HPF")->load();
    float hResVal = apvtsRef.getRawParameterValue("FILTER_RES_HPF")->load();
    float lpfVal = apvtsRef.getRawParameterValue("FILTER_LPF")->load();
    float lResVal = apvtsRef.getRawParameterValue("FILTER_RES_LPF")->load();

    juce::Path filterCurve;
    filterCurve.startNewSubPath(graphArea.getX(), graphArea.getBottom());

    for (int x = 0; x <= graphArea.getWidth(); x += 2)
    {
        float proportion = (float)x / (float)graphArea.getWidth();
        float currentFreq = 20.0f * std::pow(1000.0f, proportion);

        float gTotal = 1.0f;

        // --- CÁLCULO HPF (High Pass Filter - Corta Graves) ---
        // Fórmula de magnitud: H(s) = s^2 / (s^2 + s/Q + 1) -> Magnitud = f^2 / sqrt((f^2 - fc^2)^2 + (f*fc/Q)^2)
        // Reorganizando para evitar divisiones por cero con frecuencias muy bajas
        float hQVal = juce::jmap(hResVal, 0.707f, 2.5f, 0.707f, 8.0f);
        float hRatio = currentFreq / hpfVal;
        float hRatioSq = hRatio * hRatio;

        // Evitamos división por cero si currentFreq es muy, muy pequeño
        float hDenom = std::sqrt(std::pow(1.0f - hRatioSq, 2.0f) + (hRatioSq / (hQVal * hQVal)));

        // La ganancia del HPF es 0 en DC y tiende a 1 en altas frecuencias.
        // Si el denominador es muy pequeño, evitamos que la ganancia explote.
        float hGainMagnitude = (hDenom > 1e-6f) ? (hRatioSq / hDenom) : 0.0f;

        gTotal *= hGainMagnitude;

        // --- CÁLCULO LPF (Low Pass Filter - Corta Agudos) ---
        // Fórmula de magnitud: H(s) = 1 / (s^2 + s/Q + 1) -> Magnitud = fc^2 / sqrt((fc^2 - f^2)^2 + (f*fc/Q)^2)
        float lQVal = juce::jmap(lResVal, 0.707f, 2.5f, 0.707f, 8.0f);
        float lRatio = currentFreq / lpfVal;
        float lRatioSq = lRatio * lRatio;

        float lDenom = std::sqrt(std::pow(1.0f - lRatioSq, 2.0f) + (lRatioSq / (lQVal * lQVal)));

        float lGainMagnitude = (lDenom > 1e-6f) ? (1.0f / lDenom) : 1.0f; // Si f -> 0, ganancia -> 1

        gTotal *= lGainMagnitude;

        // Convertimos a decibelios y limitamos para no salir de la pantalla
        // Un piso de ruido de -60dB suele quedar mejor visualmente que -40dB
        float totalDb = 20.0f * std::log10(juce::jmax(gTotal, 1e-6f));
        totalDb = juce::jlimit(-60.0f, 20.0f, totalDb);

        // Mapeamos a píxeles en el eje Y
        float yNorm = juce::jmap(totalDb, -60.0f, 20.0f, 1.0f, 0.0f);
        float yPixel = graphArea.getY() + (yNorm * graphArea.getHeight());
        yPixel = juce::jlimit((float)graphArea.getY(), (float)graphArea.getBottom(), yPixel);

        filterCurve.lineTo(graphArea.getX() + x, yPixel);
    }

    // Cerramos el dibujo
    filterCurve.lineTo(graphArea.getRight(), graphArea.getBottom());
    filterCurve.closeSubPath();

    // 4. PINTAMOS LA CURVA CYAN
    g.setColour(juce::Colours::cyan.withAlpha(0.4f));
    g.fillPath(filterCurve);

    g.setColour(juce::Colours::cyan.brighter());
    g.strokePath(filterCurve, juce::PathStrokeType(2.0f));
}
