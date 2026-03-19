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

    // 2. MATEMÁTICAS DEL DIBUJO (ESCALA FIJA PARA EL RATÓN)
    float totalVisualTime = 15.0f;

    float startX = bounds.getX();
    float bottomY = bounds.getBottom();
    float width = bounds.getWidth();
    float height = bounds.getHeight() - 15; // Dejamos hueco para el texto
    float topY = bottomY - height;

    // Calculamos X moviéndose proporcionalmente a los 15 segundos reales
    float attackX = startX + (width * (a / totalVisualTime));
    float decayX = attackX + (width * (d / totalVisualTime));
    float sustainX = decayX + (width * (2.0f / totalVisualTime)); // El Sustain visual ocupa 2 segundos
    float releaseX = sustainX + (width * (r / totalVisualTime));  // ¡El release ahora es dinámico y se mueve!

    // Evitamos que si ponemos todos los parámetros al máximo (5+5+2+5=17s) se salga de la caja
    releaseX = juce::jlimit(startX, (float)bounds.getRight(), releaseX);

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

    // Dibujamos los circulitos en los nodos clave (Más grandes para agarrarlos con el ratón)
    float dotSize = 8.0f; // Los subo a 8.0f para que el "hitbox" sea fácil de tocar
    g.setColour(juce::Colours::white);

    // Solo dibujamos los 3 nodos interactivos (Attack, Decay, Release)
    g.fillEllipse(attackX - dotSize / 2, topY - dotSize / 2, dotSize, dotSize);
    g.fillEllipse(decayX - dotSize / 2, sustainY - dotSize / 2, dotSize, dotSize);
    //g.fillEllipse(sustainX - dotSize / 2, sustainY - dotSize / 2, dotSize, dotSize);
    g.fillEllipse(releaseX - dotSize / 2, bottomY - dotSize / 2, dotSize, dotSize);
}

void EnvelopeModule::resized()
{
    // No necesitamos componentes hijo, todo se dibuja en el paint!
}

// =================================================================================
// --- EVENTOS DE RATÓN (ESTILO SERUM) ---
// =================================================================================

void EnvelopeModule::mouseDown(const juce::MouseEvent& event)
{
    activeNode = -1; // Reseteamos
    auto area = getLocalBounds();
    auto ampArea = area.removeFromTop(area.getHeight() / 2);
    auto env2Area = area;

    // Función lambda para comprobar si hemos cazado algún punto blanco
    auto checkHits = [&](juce::Rectangle<int> bounds, juce::String prefix, int baseIndex)
        {
            float a = apvtsRef.getRawParameterValue(prefix + "_A")->load();
            float d = apvtsRef.getRawParameterValue(prefix + "_D")->load();
            float s = apvtsRef.getRawParameterValue(prefix + "_S")->load();
            float r = apvtsRef.getRawParameterValue(prefix + "_R")->load();

            float totalVisualTime = 15.0f;
            float startX = bounds.getX();
            float bottomY = bounds.getBottom();
            float width = bounds.getWidth();
            float height = bounds.getHeight() - 15;
            float topY = bottomY - height;

            float attackX = startX + (width * (a / totalVisualTime));
            float decayX = attackX + (width * (d / totalVisualTime));
            float sustainX = decayX + (width * (2.0f / totalVisualTime));
            float releaseX = sustainX + (width * (r / totalVisualTime));
            float sustainY = bottomY - (height * s);

            juce::Point<float> mousePos = event.position;

            // Si hacemos clic a menos de 15 píxeles del punto, lo cazamos
            if (mousePos.getDistanceFrom({ attackX, topY }) < 15.0f) { activeNode = baseIndex; startParamX = a; }
            else if (mousePos.getDistanceFrom({ decayX, sustainY }) < 15.0f) { activeNode = baseIndex + 1; startParamX = d; startParamY = s; }
            else if (mousePos.getDistanceFrom({ releaseX, bottomY }) < 15.0f) { activeNode = baseIndex + 2; startParamX = r; }
        };

    checkHits(ampArea.reduced(5), "AMP", 0);   // Comprueba los 3 puntos del AMP
    checkHits(env2Area.reduced(5), "ENV2", 3); // Comprueba los 3 puntos del ENV 2
}

void EnvelopeModule::mouseDrag(const juce::MouseEvent& event)
{
    if (activeNode == -1) return; // Si no cazamos nada, salimos

    // Calculamos cuánto se ha movido el ratón desde el clic inicial
    float deltaX = event.getDistanceFromDragStartX();
    float deltaY = event.getDistanceFromDragStartY();

    // Sensibilidad del ratón: 50 píxeles movidos = 1 segundo de tiempo
    float timeChange = deltaX * 0.02f;
    // Sensibilidad Y: 100 píxeles = Todo el rango de Sustain (0 a 1)
    float sustainChange = -deltaY * 0.01f;

    // Función rápida para enviar el valor actualizado al motor de audio
    auto updateParam = [&](juce::String id, float startVal, float change, float min, float max) {
        if (auto* p = apvtsRef.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(juce::jlimit(min, max, startVal + change)));
        };

    // Aplicamos los cambios dependiendo de qué punto agarramos
    if (activeNode == 0)      updateParam("AMP_A", startParamX, timeChange, 0.01f, 5.0f);
    else if (activeNode == 1) {
        updateParam("AMP_D", startParamX, timeChange, 0.01f, 5.0f); // Mover X cambia Decay
        updateParam("AMP_S", startParamY, sustainChange, 0.0f, 1.0f); // Mover Y cambia Sustain
    }
    else if (activeNode == 2) updateParam("AMP_R", startParamX, timeChange, 0.01f, 5.0f);

    // Lo mismo para el Envelope 2
    else if (activeNode == 3) updateParam("ENV2_A", startParamX, timeChange, 0.01f, 5.0f);
    else if (activeNode == 4) {
        updateParam("ENV2_D", startParamX, timeChange, 0.01f, 5.0f);
        updateParam("ENV2_S", startParamY, sustainChange, 0.0f, 1.0f);
    }
    else if (activeNode == 5) updateParam("ENV2_R", startParamX, timeChange, 0.01f, 5.0f);
}

void EnvelopeModule::mouseUp(const juce::MouseEvent& event)
{
    activeNode = -1; // Soltamos el punto
}
