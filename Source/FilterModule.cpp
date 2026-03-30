/*
  ==============================================================================

    FilterModule.cpp
    Created: 18 Mar 2026 9:10:29pm
    Author:  franc

  ==============================================================================
*/

#include "FilterModule.h"

// 1. CONSTRUCTOR ACTUALIZADO: Recibe y guarda el layerPrefix
FilterModule::FilterModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), layerPrefix(prefix)
{
    // Función lambda para no repetir código al configurar los knobs
    auto setupKnob = [this](juce::Slider& slider, const juce::String& paramID, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramID, slider);
        };

    // Configuramos y conectamos los 4 knobs usando el prefijo dinámico
    setupKnob(hpfKnob, layerPrefix + "FILTER_HPF", hpfAttach);
    setupKnob(resHpfKnob, layerPrefix + "FILTER_RES_HPF", resHpfAttach);

    setupKnob(lpfKnob, layerPrefix + "FILTER_LPF", lpfAttach);
    setupKnob(resLpfKnob, layerPrefix + "FILTER_RES_LPF", resLpfAttach);

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

void FilterModule::mouseDown(const juce::MouseEvent& event)
{
    // Solo actuamos si el clic está dentro del rectángulo de la gráfica
    if (!graphArea.contains(event.x, event.y)) return;

    // Obtenemos las posiciones actuales de los puntos para calcular la distancia
    auto getDotX = [&](float freq) {
        float minF = 20.0f, maxF = 20000.0f;
        float proportion = std::log10(freq / minF) / std::log10(maxF / minF);
        return graphArea.getX() + proportion * graphArea.getWidth();
        };

    auto getDotY = [&](float qVal) {
        float qDb = 20.0f * std::log10(juce::jmax(qVal, 0.707f));
        float yNorm = juce::jmap(qDb, -2.0f, 15.0f, 0.8f, 0.0f);
        return graphArea.getY() + (yNorm * graphArea.getHeight());
        };

    // LEEMOS CON PREFIJO DINÁMICO
    float baseHpf = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_HPF")->load();
    float baseHRes = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_HPF")->load();
    float baseLpf = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_LPF")->load();
    float baseLRes = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_LPF")->load();

    juce::Point<float> hpfDot(getDotX(baseHpf), getDotY(baseHRes));
    juce::Point<float> lpfDot(getDotX(baseLpf), getDotY(baseLRes));

    juce::Point<float> mousePos((float)event.x, (float)event.y);

    // Si pulsamos cerca (radio de 20px para que sea fácil acertar) de un punto
    float hitRadius = 20.0f;

    if (mousePos.getDistanceFrom(hpfDot) < hitRadius) {
        draggedDot = 0; // Enganchamos HPF
    }
    else if (mousePos.getDistanceFrom(lpfDot) < hitRadius) {
        draggedDot = 1; // Enganchamos LPF
    }
    else {
        draggedDot = -1; // Clic en el aire
    }
}

void FilterModule::mouseDrag(const juce::MouseEvent& event)
{
    // Si no tenemos ningún punto enganchado, salimos
    if (draggedDot == -1) return;

    // Convertimos la posición X e Y del ratón en proporciones de 0.0 a 1.0 dentro de la gráfica
    float proportionX = (float)(event.x - graphArea.getX()) / (float)graphArea.getWidth();
    float proportionY = (float)(event.y - graphArea.getY()) / (float)graphArea.getHeight();

    // Limitamos para no salirnos de la caja
    proportionX = juce::jlimit(0.0f, 1.0f, proportionX);
    proportionY = juce::jlimit(0.0f, 1.0f, proportionY);

    // Convertimos proporción X a Hercios (Logarítmico)
    float newFreq = 20.0f * std::pow(1000.0f, proportionX);
    newFreq = juce::jlimit(20.0f, 20000.0f, newFreq);

    // Convertimos proporción Y a Resonancia (Y arriba es 0, así que invertimos con 1.0-)
    float newRes = juce::jmap(1.0f - proportionY, 0.0f, 1.0f, 0.707f, 2.5f);
    newRes = juce::jlimit(0.707f, 2.5f, newRes);

    // Actualizamos los Knobs (y los Attachments harán la magia de actualizar el Procesador)
    if (draggedDot == 0) { // HPF
        hpfKnob.setValue(newFreq);
        resHpfKnob.setValue(newRes);
    }
    else if (draggedDot == 1) { // LPF
        lpfKnob.setValue(newFreq);
        resLpfKnob.setValue(newRes);
    }
}

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
    // LEEMOS CON PREFIJO DINÁMICO
    float hpfVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_HPF")->load();
    float hResVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_HPF")->load();
    float lpfVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_LPF")->load();
    float lResVal = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_LPF")->load();

    juce::Path filterCurve;
    filterCurve.startNewSubPath(graphArea.getX(), graphArea.getBottom());

    for (int x = 0; x <= graphArea.getWidth(); x += 2)
    {
        float proportion = (float)x / (float)graphArea.getWidth();
        float currentFreq = 20.0f * std::pow(1000.0f, proportion);

        float gTotal = 1.0f;

        // --- CÁLCULO HPF (High Pass Filter - Corta Graves) ---
        float hQVal = juce::jmap(hResVal, 0.707f, 2.5f, 0.707f, 8.0f);
        float hRatio = currentFreq / hpfVal;
        float hRatioSq = hRatio * hRatio;
        float hDenom = std::sqrt(std::pow(1.0f - hRatioSq, 2.0f) + (hRatioSq / (hQVal * hQVal)));
        float hGainMagnitude = (hDenom > 1e-6f) ? (hRatioSq / hDenom) : 0.0f;
        gTotal *= hGainMagnitude;

        // --- CÁLCULO LPF (Low Pass Filter - Corta Agudos) ---
        float lQVal = juce::jmap(lResVal, 0.707f, 2.5f, 0.707f, 8.0f);
        float lRatio = currentFreq / lpfVal;
        float lRatioSq = lRatio * lRatio;
        float lDenom = std::sqrt(std::pow(1.0f - lRatioSq, 2.0f) + (lRatioSq / (lQVal * lQVal)));
        float lGainMagnitude = (lDenom > 1e-6f) ? (1.0f / lDenom) : 1.0f;
        gTotal *= lGainMagnitude;

        // Convertimos a decibelios y limitamos
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

    // PUNTOS BLANCOS
    auto getDotX = [&](float freq) {
        float minF = 20.0f, maxF = 20000.0f;
        float proportion = std::log10(freq / minF) / std::log10(maxF / minF);
        return graphArea.getX() + proportion * graphArea.getWidth();
        };

    auto getDotY = [&](float qVal) {
        float qDb = 20.0f * std::log10(juce::jmax(qVal, 0.707f));
        float yNorm = juce::jmap(qDb, -2.0f, 15.0f, 0.8f, 0.0f);
        return graphArea.getY() + (yNorm * graphArea.getHeight());
        };

    // LEEMOS CON PREFIJO DINÁMICO
    float baseHpf = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_HPF")->load();
    float baseHRes = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_HPF")->load();
    float baseLpf = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_LPF")->load();
    float baseLRes = apvtsRef.getRawParameterValue(layerPrefix + "FILTER_RES_LPF")->load();

    juce::Point<float> hpfDot(getDotX(baseHpf), getDotY(baseHRes));
    juce::Point<float> lpfDot(getDotX(baseLpf), getDotY(baseLRes));

    float dotRadius = 5.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(hpfDot.getX() - dotRadius, hpfDot.getY() - dotRadius, dotRadius * 2, dotRadius * 2);
    g.fillEllipse(lpfDot.getX() - dotRadius, lpfDot.getY() - dotRadius, dotRadius * 2, dotRadius * 2);
}
