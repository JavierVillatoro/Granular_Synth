/*
  ==============================================================================

    EnvelopeModule.cpp
    Created: 18 Mar 2026 10:57:07pm
    Author:  franc

  ==============================================================================
*/

#include "EnvelopeModule.h"

EnvelopeModule::EnvelopeModule(juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef(apvts)
{
    // Hacemos que la pantalla se actualice a 30fps para que los gráficos sean fluidos
    startTimerHz(30);
}

EnvelopeModule::~EnvelopeModule() {}

void EnvelopeModule::timerCallback()
{
    repaint(); // Redibuja los gráficos si movemos algo
}

void EnvelopeModule::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();

    // Dividimos el espacio en dos mitades perfectas (Arriba y Abajo)
    auto ampArea = area.removeFromTop(area.getHeight() / 2);
    auto env2Area = area; // Lo que sobra es la mitad de abajo

    // Leemos los valores actuales del Cerebro (APVTS)
    float aA = apvtsRef.getRawParameterValue("AMP_A")->load();
    float aD = apvtsRef.getRawParameterValue("AMP_D")->load();
    float aS = apvtsRef.getRawParameterValue("AMP_S")->load();
    float aR = apvtsRef.getRawParameterValue("AMP_R")->load();

    float e2A = apvtsRef.getRawParameterValue("ENV2_A")->load();
    float e2D = apvtsRef.getRawParameterValue("ENV2_D")->load();
    float e2S = apvtsRef.getRawParameterValue("ENV2_S")->load();
    float e2R = apvtsRef.getRawParameterValue("ENV2_R")->load();

    // Dibujamos las dos envolventes separadas visualmente por una línea sutil
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(area.getX(), ampArea.getBottom(), area.getRight(), ampArea.getBottom(), 1.0f);

    // Pintamos los gráficos
    drawEnvelope(g, ampArea.reduced(5), "AMP", aA, aD, aS, aR);
    drawEnvelope(g, env2Area.reduced(5), "ENV 2", e2A, e2D, e2S, e2R);
}

void EnvelopeModule::drawEnvelope(juce::Graphics& g, juce::Rectangle<int> bounds, juce::String name, float a, float d, float s, float r)
{
    // 1. Textos Minimalistas (Arriba a la izquierda)
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(name, bounds.withHeight(15), juce::Justification::topLeft, false);

    // 2. MATEMÁTICAS DEL DIBUJO
    // Para no salirnos de la pantalla, sumamos el tiempo total máximo posible (5+5+2+5 = 17 seg aprox)
    // Pero lo haremos proporcional al valor real para que se vea bien estético
    float totalTime = a + d + 2.0f + r; // Asumimos un sustain visual de "2 segundos"

    float startX = bounds.getX();
    float bottomY = bounds.getBottom();
    float width = bounds.getWidth();
    float height = bounds.getHeight() - 15; // Dejamos hueco para el texto
    float topY = bottomY - height;

    // Calculamos las posiciones X de cada punto
    float attackX = startX + (width * (a / totalTime));
    float decayX = attackX + (width * (d / totalTime));
    float sustainX = decayX + (width * (2.0f / totalTime));
    float releaseX = bounds.getRight(); // Termina al final de la caja

    // Calculamos la posición Y del Sustain (invertido porque Y=0 está arriba)
    float sustainY = bottomY - (height * s);

    // 3. CREAMOS LA FORMA VECTORIAL (PATH)
    juce::Path envPath;
    envPath.startNewSubPath(startX, bottomY);          // Punto 0: Inicio
    envPath.lineTo(attackX, topY);                     // Punto 1: Pico del Attack
    envPath.lineTo(decayX, sustainY);                  // Punto 2: Caída al Sustain
    envPath.lineTo(sustainX, sustainY);                // Punto 3: Mantiene el Sustain
    envPath.lineTo(releaseX, bottomY);                 // Punto 4: Final del Release

    // 4. EL TOQUE SERUM (Relleno Cyan semitransparente y Borde Brillante)
    g.setColour(juce::Colours::cyan.withAlpha(0.2f)); // Relleno suave
    g.fillPath(envPath);

    g.setColour(juce::Colours::cyan.withAlpha(0.9f)); // Borde Neón
    g.strokePath(envPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    // Dibujamos los circulitos en los nodos clave (Attack, Decay, Sustain, End)
    float dotSize = 4.0f;
    g.setColour(juce::Colours::white);
    g.fillEllipse(attackX - dotSize / 2, topY - dotSize / 2, dotSize, dotSize);
    g.fillEllipse(decayX - dotSize / 2, sustainY - dotSize / 2, dotSize, dotSize);
    g.fillEllipse(sustainX - dotSize / 2, sustainY - dotSize / 2, dotSize, dotSize);
}

void EnvelopeModule::resized()
{
    // No necesitamos componentes hijo, todo se dibuja en el paint!
}
