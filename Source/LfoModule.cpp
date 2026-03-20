/*
  ==============================================================================

    LfoModule.cpp
    Created: 19 Mar 2026 8:45:08pm
    Author:  franc

  ==============================================================================
*/

#include "LfoModule.h"

LfoModule::LfoModule(juce::AudioProcessorValueTreeState& apvts) : apvtsRef(apvts)
{
    // --- Configuración de los Knobs ---
    auto setupKnob = [this](juce::Slider& k) {
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(k);
        };

    setupKnob(depthKnob);
    setupKnob(jitterKnob);

    // --- Configuración de Menús ---
    addAndMakeVisible(waveSelector);
    addAndMakeVisible(beatSelector);

    waveSelector.addItemList({ "Sine", "Triangle", "Saw", "Square", "S&H" }, 1);
    beatSelector.addItemList({ "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 1);

    // --- Conexiones al APVTS ---
    depthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, "LFO1_DEPTH", depthKnob);
    jitterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, "LFO1_JITTER", jitterKnob);
    waveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, "LFO1_WAVE", waveSelector);
    beatAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, "LFO1_BEAT", beatSelector);

    startTimerHz(30); // Actualiza la pantalla a 30fps para ver el Jitter en vivo

    // Inicializa LFO 2 con una forma de triángulo básica
    lfoNodes.push_back({ 0.0f, 0.5f, 0.0f });  // Punto inicial (Izquierda, Centro)
    lfoNodes.push_back({ 0.5f, 1.0f, 0.0f });  // Punto central (Medio, Arriba del todo)
    lfoNodes.push_back({ 1.0f, 0.5f, 0.0f });  // Punto final (Derecha, Centro)
}

LfoModule::~LfoModule() {}

void LfoModule::timerCallback() { repaint(); }

void LfoModule::resized()
{
    auto area = getLocalBounds();
    auto lfo1Area = area.removeFromTop(area.getHeight() / 2);

    // Guardamos el área exacta del lienzo del LFO 2 para el ratón
    lfo2Area = area;

    // --- Layout del LFO 1 ---
    lfo1Area.removeFromTop(20);
    auto screenArea = lfo1Area.removeFromLeft(130).reduced(5);
    auto menusArea = lfo1Area.removeFromLeft(70).reduced(2, 5);
    waveSelector.setBounds(menusArea.removeFromTop(menusArea.getHeight() / 2).reduced(0, 2));
    beatSelector.setBounds(menusArea.reduced(0, 2));
    auto knobsArea = lfo1Area.reduced(5);
    depthKnob.setBounds(knobsArea.removeFromLeft(knobsArea.getWidth() / 2));
    jitterKnob.setBounds(knobsArea);
}

void LfoModule::paint(juce::Graphics& g)
{
    auto area = getLocalBounds();
    auto lfo1Area = area.removeFromTop(area.getHeight() / 2);

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawLine(area.getX(), lfo1Area.getBottom(), area.getRight(), lfo1Area.getBottom(), 1.0f);

    drawLfo1(g, lfo1Area);

    // Llamamos a la nueva super-función del LFO 2
    drawLfo2(g, lfo2Area);
}

void LfoModule::drawLfo1(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // ========================================================
    // --- 1. AJUSTES VISUALES: COLORES Y TEXTO ---
    // ========================================================
    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0); // Gris platino brillante

    g.setColour(titaniumColor.withAlpha(0.8f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("LFO 1", bounds.reduced(5).withHeight(15), juce::Justification::topLeft, false);

    g.setFont(10.0f);
    g.drawText("Amp", depthKnob.getBounds().translated(0, 25), juce::Justification::centred, false);
    g.drawText("Jitter", jitterKnob.getBounds().translated(0, 25), juce::Justification::centred, false);

    // ==============================================================================
    // --- NUEVO 1.5: FORZAR KNOBS DE LFO A GRIS PLATINO ---
    // ==============================================================================
    auto setKnobPlatino = [&](juce::Slider& k) {
        k.setColour(juce::Slider::rotarySliderOutlineColourId, titaniumColor.withAlpha(0.2f));
        k.setColour(juce::Slider::rotarySliderFillColourId, titaniumColor.withAlpha(0.8f));
        k.setColour(juce::Slider::thumbColourId, titaniumColor); // El "punto" del knob
        };

    setKnobPlatino(depthKnob);
    setKnobPlatino(jitterKnob);

    // ========================================================
    // --- 2. CONFIGURACIÓN DE LA PANTALLA ---
    // ========================================================
    auto screenRect = bounds;
    screenRect.removeFromTop(20);
    screenRect = screenRect.removeFromLeft(130).reduced(5);

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(screenRect);
    g.setColour(titaniumColor.withAlpha(0.3f));
    g.drawRect(screenRect, 1);

    // ==============================================================================
    // --- 2.5: MATEMÁTICAS DE LA ONDA (DINÁMICAS Y REALISTAS) ---
    // ==============================================================================

    // Leemos los parámetros en vivo del cerebro
    float amp = apvtsRef.getRawParameterValue("LFO1_DEPTH")->load();
    float jitter = apvtsRef.getRawParameterValue("LFO1_JITTER")->load();
    int waveType = (int)apvtsRef.getRawParameterValue("LFO1_WAVE")->load();
    int beatIndex = (int)apvtsRef.getRawParameterValue("LFO1_BEAT")->load();

    // TABLA DE MAPEO CORREGIDA: La pantalla representa 1 Redonda (1/1 o un compás de 4/4)
    auto getNumCyclesForBeat = [](int beatIdx) -> float {
        switch (beatIdx) {
        case 0: return 0.125f;// 8/1 - Muy lento
        case 1: return 0.25f; // 4/1
        case 2: return 0.5f;  // 2/1
        case 3: return 1.0f;  // 1/1 (1 ciclo ocupa toda la pantalla)
        case 4: return 2.0f;  // 1/2 (Blancas - 2 ciclos)
        case 5: return 4.0f;  // 1/4 (Negras - 4 ciclos)
        case 6: return 8.0f;  // 1/8 (Corcheas - 8 ciclos)
        case 7: return 16.0f; // 1/16 (Semicorcheas - 16 ciclos)
        case 8: return 32.0f; // 1/32 (Fusas - 32 ciclos, bastante apretado)
        default: return 4.0f;
        }
        };

    // Obtenemos los ciclos exactos
    float numCycles = getNumCyclesForBeat(beatIndex);

    // Altura máxima = 90% de la caja. Lo multiplicamos por el Amp.
    float actualAmplitude = (screenRect.getHeight() / 2.0f) * 0.9f * amp;

    float startX = screenRect.getX();
    float width = screenRect.getWidth();
    float centerY = screenRect.getCentreY();
    juce::Path wavePath;

    // ==============================================================================
    // --- 3: EL MOTOR DE DIBUJO UNIFICADO ---
    // ==============================================================================

    for (float x = 0; x <= width; x += 1.0f)
    {
        // Calculamos la "fase global" (ej. de 0.0 a 4.0 si estamos en 1/4)
        float phase = (x / width) * numCycles;

        // Fase dentro de un solo ciclo (siempre va de 0.0 a 1.0)
        float phaseInCycle = std::fmod(phase, 1.0f);
        float yVal = 0.0f;

        // Fórmulas de las formas de onda
        if (waveType == 0)      yVal = std::sin(phaseInCycle * juce::MathConstants<float>::twoPi);
        else if (waveType == 1) yVal = 2.0f * std::abs(2.0f * phaseInCycle - 1.0f) - 1.0f;
        else if (waveType == 2) yVal = 1.0f - 2.0f * phaseInCycle;
        else if (waveType == 3) yVal = (phaseInCycle < 0.5f) ? 1.0f : -1.0f;
        else if (waveType == 4) {
            // FIX S&H: Función Hash matemática (Caos puro)
            int currentStep = (int)std::floor(phase);
            // Multiplicamos por un número gigante para que el Seno salte a lo loco entre -1 y 1
            yVal = std::sin((float)currentStep * 8373.123f);
        }

        // --- APLICAMOS EL JITTER (RUIDO) ---
        if (jitter > 0.0f) {
            float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
            // El S&H admite más ruido visual que las ondas limpias
            float jitterStrength = (waveType == 4) ? 0.5f : 0.3f;
            yVal += noise * jitter * jitterStrength;
            yVal = juce::jlimit(-1.0f, 1.0f, yVal);
        }

        // Convertimos el valor matemático (-1 a 1) en píxeles de pantalla
        float finalY = centerY - (yVal * actualAmplitude);

        // Trazamos la línea
        if (x == 0) wavePath.startNewSubPath(startX, finalY);
        else wavePath.lineTo(startX + x, finalY);
    }

    // Dibujamos el trazo final en Gris Platino
    g.setColour(titaniumColor.withAlpha(0.9f));
    g.strokePath(wavePath, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
}

void LfoModule::drawLfo2(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0);

    // 1. TEXTO Y FONDO
    g.setColour(titaniumColor.withAlpha(0.8f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("LFO 2", bounds.reduced(5).withHeight(15), juce::Justification::topLeft, false);

    // Creamos la "Pantalla" interactiva (dejamos margen para los textos)
    auto canvasArea = bounds;
    canvasArea.removeFromTop(20);
    canvasArea = canvasArea.reduced(10); // Margen para que los puntos no toquen el borde

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRect(canvasArea);

    // Dibujamos una cuadrícula sutil (Grid)
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    float stepX = canvasArea.getWidth() / 4.0f;
    float stepY = canvasArea.getHeight() / 4.0f;
    for (int i = 1; i < 4; ++i) {
        g.drawLine(canvasArea.getX() + (stepX * i), canvasArea.getY(), canvasArea.getX() + (stepX * i), canvasArea.getBottom());
        g.drawLine(canvasArea.getX(), canvasArea.getY() + (stepY * i), canvasArea.getRight(), canvasArea.getY() + (stepY * i));
    }

    g.setColour(titaniumColor.withAlpha(0.3f));
    g.drawRect(canvasArea, 1);

    // 2. DIBUJAMOS LOS PUNTOS Y LÍNEAS DE NUESTRO VECTOR
    if (lfoNodes.size() < 2) return; // Protección anti-crashes

    juce::Path lfoPath;
    float startX = canvasArea.getX();
    float bottomY = canvasArea.getBottom();
    float width = canvasArea.getWidth();
    float height = canvasArea.getHeight();

    // Función rápida para convertir de coordenadas normalizadas (0.0-1.0) a píxeles de la pantalla
    auto getPixelPos = [&](LfoNode node) -> juce::Point<float> {
        return { startX + (node.x * width), bottomY - (node.y * height) }; // Y está invertida visualmente
        };

    // Trazamos las líneas
    //for (size_t i = 0; i < lfoNodes.size(); ++i)
    //{
        //auto pos = getPixelPos(lfoNodes[i]);
        //if (i == 0) lfoPath.startNewSubPath(pos);
        //else lfoPath.lineTo(pos); // (Más adelante cambiaremos esto por curvas Bézier)
    //}

    // Trazamos las líneas (¡AHORA CON CURVAS!)
    for (size_t i = 0; i < lfoNodes.size() - 1; ++i)
    {
        auto nodeA = lfoNodes[i];
        auto nodeB = lfoNodes[i + 1];

        if (i == 0) lfoPath.startNewSubPath(getPixelPos(nodeA));

        // Si la curva está a 0, trazamos una línea recta directa
        if (std::abs(nodeA.curve) < 0.01f) {
            lfoPath.lineTo(getPixelPos(nodeB));
        }
        else {
            // Magia: Dibujamos la curva dividiéndola en 20 micro-segmentos
            int numSegments = 20;
            float power = std::exp(nodeA.curve * 3.0f); // Tensión Exponencial

            for (int s = 1; s <= numSegments; ++s) {
                float t = (float)s / (float)numSegments;
                float warpedT = std::pow(t, power);
                float interpX = nodeA.x + (nodeB.x - nodeA.x) * t;
                float interpY = nodeA.y + (nodeB.y - nodeA.y) * warpedT;
                lfoPath.lineTo(getPixelPos({ interpX, interpY, 0.0f }));
            }
        }
    }

    // Relleno semitransparente (estilo Serum)
    juce::Path filledPath = lfoPath;
    filledPath.lineTo(getPixelPos(lfoNodes.back()).x, bottomY);
    filledPath.lineTo(getPixelPos(lfoNodes.front()).x, bottomY);
    filledPath.closeSubPath();

    g.setColour(titaniumColor.withAlpha(0.15f));
    g.fillPath(filledPath);

    // Borde de la línea
    g.setColour(titaniumColor.withAlpha(0.9f));
    g.strokePath(lfoPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    // Dibujamos los "Nodos" (Circulitos) para que el usuario sepa de dónde agarrar
    float dotSize = 8.0f;
    g.setColour(juce::Colours::white);
    for (const auto& node : lfoNodes) {
        auto pos = getPixelPos(node);
        g.fillEllipse(pos.x - dotSize / 2, pos.y - dotSize / 2, dotSize, dotSize);
    }
}

// RATON
void LfoModule::mouseDown(const juce::MouseEvent& event)
{
    // CÁLCULO EXACTO DEL LIENZO (Igual que en el Paint)
    auto areaInt = lfo2Area;
    areaInt.removeFromTop(20);
    auto canvasArea = areaInt.reduced(10).toFloat();

    draggedNode = -1;
    draggedCurve = -1;

    // 1. Comprobamos si hemos agarrado un NODO (Punto blanco)
    for (int i = 0; i < lfoNodes.size(); ++i) {
        juce::Point<float> nodePos(canvasArea.getX() + lfoNodes[i].x * canvasArea.getWidth(),
            canvasArea.getBottom() - lfoNodes[i].y * canvasArea.getHeight());

        if (event.position.getDistanceFrom(nodePos) < 15.0f) { // Imán de 15 píxeles
            draggedNode = i;
            return; // ¡Agarrado! Salimos de la función
        }
    }

    // 2. Si no agarramos un nodo, comprobamos si estamos agarrando una CURVA (Línea)
    if (canvasArea.contains(event.position) || lfo2Area.toFloat().contains(event.position)) {
        float normX = (event.position.x - canvasArea.getX()) / canvasArea.getWidth();

        // Buscamos entre qué dos puntos ha caído el ratón
        for (int i = 0; i < lfoNodes.size() - 1; ++i) {
            if (normX >= lfoNodes[i].x && normX <= lfoNodes[i + 1].x) {
                draggedCurve = i;
                initialCurve = lfoNodes[i].curve; // Guardamos cómo estaba la curva
                return;
            }
        }
    }
}

void LfoModule::mouseDrag(const juce::MouseEvent& event)
{
    // CÁLCULO EXACTO DEL LIENZO
    auto areaInt = lfo2Area;
    areaInt.removeFromTop(20);
    auto canvasArea = areaInt.reduced(10).toFloat();

    // CASO A: Estamos moviendo un NODO
    if (draggedNode != -1) {
        float normX = (event.position.x - canvasArea.getX()) / canvasArea.getWidth();
        float normY = (canvasArea.getBottom() - event.position.y) / canvasArea.getHeight();

        normY = juce::jlimit(0.0f, 1.0f, normY); // Límite vertical perfecto

        // Límite horizontal (No puede adelantar a sus vecinos)
        if (draggedNode == 0) normX = 0.0f;
        else if (draggedNode == lfoNodes.size() - 1) normX = 1.0f;
        else {
            normX = juce::jlimit(lfoNodes[draggedNode - 1].x + 0.02f, lfoNodes[draggedNode + 1].x - 0.02f, normX);
        }

        lfoNodes[draggedNode].x = normX;
        lfoNodes[draggedNode].y = normY;
        repaint();
    }
    // CASO B: Estamos doblando una CURVA
    else if (draggedCurve != -1) {
        float dy = (event.mouseDownPosition.y - event.y) / canvasArea.getHeight();
        lfoNodes[draggedCurve].curve = juce::jlimit(-1.0f, 1.0f, initialCurve + (dy * 2.0f));
        repaint();
    }
}

void LfoModule::mouseDoubleClick(const juce::MouseEvent& event)
{
    // CÁLCULO EXACTO DEL LIENZO
    auto areaInt = lfo2Area;
    areaInt.removeFromTop(20);
    auto canvasArea = areaInt.reduced(10).toFloat();

    // 1. BORRAR: ¿Hemos hecho doble clic en un nodo?
    for (int i = 1; i < lfoNodes.size() - 1; ++i) {
        juce::Point<float> nodePos(canvasArea.getX() + lfoNodes[i].x * canvasArea.getWidth(),
            canvasArea.getBottom() - lfoNodes[i].y * canvasArea.getHeight());

        if (event.position.getDistanceFrom(nodePos) < 15.0f) {
            lfoNodes.erase(lfoNodes.begin() + i);
            repaint();
            return;
        }
    }

    // 2. AÑADIR: Si no hay nodo, creamos uno nuevo
    float normX = (event.position.x - canvasArea.getX()) / canvasArea.getWidth();
    float normY = (canvasArea.getBottom() - event.position.y) / canvasArea.getHeight();

    normX = juce::jlimit(0.0f, 1.0f, normX);
    normY = juce::jlimit(0.0f, 1.0f, normY);

    LfoNode newNode{ normX, normY, 0.0f };

    for (auto it = lfoNodes.begin(); it != lfoNodes.end(); ++it) {
        if (it->x > newNode.x) {
            lfoNodes.insert(it, newNode);
            break;
        }
    }
    repaint();
}
