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

    addAndMakeVisible(beatSelector2);
    beatSelector2.addItemList({ "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 1);
    beatAttach2 = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, "LFO2_BEAT", beatSelector2);

    // --- Conexiones al APVTS ---
    depthAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, "LFO1_DEPTH", depthKnob);
    jitterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, "LFO1_JITTER", jitterKnob);
    waveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, "LFO1_WAVE", waveSelector);
    beatAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, "LFO1_BEAT", beatSelector);

    startTimerHz(30);

    // ==============================================================================
    // NUEVO: Inicializa LFO 2 Vectorial con un triángulo básico y manecillas planas
    // ==============================================================================
    LfoNode nodeStart, nodeMid, nodeEnd;

    nodeStart.pos = { 0.0f, 0.5f };
    nodeStart.handleOut = { 0.1f, 0.0f }; // Sale recta a la derecha
    lfoNodes.push_back(nodeStart);

    nodeMid.pos = { 0.5f, 1.0f };
    nodeMid.handleIn = { -0.1f, 0.0f };  // Entra recta desde la izquierda
    nodeMid.handleOut = { 0.1f, 0.0f };  // Sale recta a la derecha
    lfoNodes.push_back(nodeMid);

    nodeEnd.pos = { 1.0f, 0.5f };
    nodeEnd.handleIn = { -0.1f, 0.0f }; // Entra recta desde la izquierda
    lfoNodes.push_back(nodeEnd);
}

LfoModule::~LfoModule() {}

void LfoModule::timerCallback() { repaint(); }

void LfoModule::resized()
{
    auto area = getLocalBounds();
    auto lfo1Area = area.removeFromTop(area.getHeight() / 2);

    lfo2Area = area;

    auto lfo2Header = lfo2Area; // Copiamos el área
    lfo2Header.removeFromTop(2); // Un poco de margen superior
    lfo2Header.removeFromLeft(100); // Dejamos 100 píxeles a la izquierda para el texto "LFO 2 (Vector)"
    beatSelector2.setBounds(lfo2Header.removeFromLeft(70).removeFromTop(16)); // Lo hacemos pequeñito y elegante

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
    drawLfo2(g, lfo2Area);
}

void LfoModule::drawLfo1(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0);

    g.setColour(titaniumColor.withAlpha(0.8f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("LFO 1", bounds.reduced(5).withHeight(15), juce::Justification::topLeft, false);

    g.setFont(10.0f);
    g.drawText("Amp", depthKnob.getBounds().translated(0, 25), juce::Justification::centred, false);
    g.drawText("Jitter", jitterKnob.getBounds().translated(0, 25), juce::Justification::centred, false);

    auto setKnobPlatino = [&](juce::Slider& k) {
        k.setColour(juce::Slider::rotarySliderOutlineColourId, titaniumColor.withAlpha(0.2f));
        k.setColour(juce::Slider::rotarySliderFillColourId, titaniumColor.withAlpha(0.8f));
        k.setColour(juce::Slider::thumbColourId, titaniumColor);
        };

    setKnobPlatino(depthKnob);
    setKnobPlatino(jitterKnob);

    auto screenRect = bounds;
    screenRect.removeFromTop(20);
    screenRect = screenRect.removeFromLeft(130).reduced(5);

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(screenRect);
    g.setColour(titaniumColor.withAlpha(0.3f));
    g.drawRect(screenRect, 1);

    float amp = apvtsRef.getRawParameterValue("LFO1_DEPTH")->load();
    float jitter = apvtsRef.getRawParameterValue("LFO1_JITTER")->load();
    int waveType = (int)apvtsRef.getRawParameterValue("LFO1_WAVE")->load();
    int beatIndex = (int)apvtsRef.getRawParameterValue("LFO1_BEAT")->load();

    auto getNumCyclesForBeat = [](int beatIdx) -> float {
        switch (beatIdx) {
        case 0: return 0.125f;
        case 1: return 0.25f;
        case 2: return 0.5f;
        case 3: return 1.0f;
        case 4: return 2.0f;
        case 5: return 4.0f;
        case 6: return 8.0f;
        case 7: return 16.0f;
        case 8: return 32.0f;
        default: return 4.0f;
        }
        };

    float numCycles = getNumCyclesForBeat(beatIndex);
    float actualAmplitude = (screenRect.getHeight() / 2.0f) * 0.9f * amp;

    float startX = screenRect.getX();
    float width = screenRect.getWidth();
    float centerY = screenRect.getCentreY();
    juce::Path wavePath;

    for (float x = 0; x <= width; x += 1.0f)
    {
        float phase = (x / width) * numCycles;
        float phaseInCycle = std::fmod(phase, 1.0f);
        float yVal = 0.0f;

        if (waveType == 0)      yVal = std::sin(phaseInCycle * juce::MathConstants<float>::twoPi);
        else if (waveType == 1) yVal = 2.0f * std::abs(2.0f * phaseInCycle - 1.0f) - 1.0f;
        else if (waveType == 2) yVal = 1.0f - 2.0f * phaseInCycle;
        else if (waveType == 3) yVal = (phaseInCycle < 0.5f) ? 1.0f : -1.0f;
        else if (waveType == 4) {
            int currentStep = (int)std::floor(phase);
            yVal = std::sin((float)currentStep * 8373.123f);
        }

        if (jitter > 0.0f) {
            float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f);
            float jitterStrength = (waveType == 4) ? 0.5f : 0.3f;
            yVal += noise * jitter * jitterStrength;
            yVal = juce::jlimit(-1.0f, 1.0f, yVal);
        }

        float finalY = centerY - (yVal * actualAmplitude);

        if (x == 0) wavePath.startNewSubPath(startX, finalY);
        else wavePath.lineTo(startX + x, finalY);
    }

    g.setColour(titaniumColor.withAlpha(0.9f));
    g.strokePath(wavePath, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
}

void LfoModule::drawLfo2(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    juce::Colour titaniumColor = juce::Colour(0xffd0d0d0);

    g.setColour(titaniumColor.withAlpha(0.8f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("LFO 2 (Vector)", bounds.reduced(5).withHeight(15), juce::Justification::topLeft, false);

    auto canvasArea = bounds;
    canvasArea.removeFromTop(20);
    canvasArea = canvasArea.reduced(10);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRect(canvasArea);

    g.setColour(juce::Colours::white.withAlpha(0.05f));
    float stepX = canvasArea.getWidth() / 4.0f;
    float stepY = canvasArea.getHeight() / 4.0f;
    for (int i = 1; i < 4; ++i) {
        g.drawLine(canvasArea.getX() + (stepX * i), canvasArea.getY(), canvasArea.getX() + (stepX * i), canvasArea.getBottom());
        g.drawLine(canvasArea.getX(), canvasArea.getY() + (stepY * i), canvasArea.getRight(), canvasArea.getY() + (stepY * i));
    }
    g.setColour(titaniumColor.withAlpha(0.3f));
    g.drawRect(canvasArea, 1);

    if (lfoNodes.size() < 2) return;

    juce::Path lfoPath;
    float startX = canvasArea.getX();
    float bottomY = canvasArea.getBottom();
    float width = canvasArea.getWidth();
    float height = canvasArea.getHeight();

    auto getPixelPos = [&](juce::Point<float> normPos) -> juce::Point<float> {
        return { startX + (normPos.x * width), bottomY - (normPos.y * height) };
        };

    for (size_t i = 0; i < lfoNodes.size(); ++i)
    {
        auto posA = getPixelPos(lfoNodes[i].pos);

        if (i == 0) {
            lfoPath.startNewSubPath(posA);
        }
        else {
            auto posB = posA;
            auto nodePrevious = lfoNodes[i - 1];
            auto nodeCurrent = lfoNodes[i];

            auto handleOutA = getPixelPos(nodePrevious.pos + nodePrevious.handleOut);
            auto handleInB = getPixelPos(nodeCurrent.pos + nodeCurrent.handleIn);

            lfoPath.cubicTo(handleOutA, handleInB, posB);
        }
    }

    juce::Path filledPath = lfoPath;
    filledPath.lineTo(getPixelPos({ lfoNodes.back().pos.x, 0.0f }).x, bottomY);
    filledPath.lineTo(getPixelPos({ lfoNodes.front().pos.x, 0.0f }).x, bottomY);
    filledPath.closeSubPath();
    g.setColour(titaniumColor.withAlpha(0.15f));
    g.fillPath(filledPath);
    g.setColour(titaniumColor.withAlpha(0.9f));
    g.strokePath(lfoPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    float dotSize = 8.0f;
    float handleSize = 6.0f;

    g.setColour(juce::Colours::white);
    for (int i = 0; i < lfoNodes.size(); ++i) {
        auto node = lfoNodes[i];
        auto pPos = getPixelPos(node.pos);

        g.setColour(juce::Colours::grey);

        if (i > 0) {
            auto hIn = getPixelPos(node.pos + node.handleIn);
            g.drawLine(pPos.x, pPos.y, hIn.x, hIn.y, 1.0f); // CORREGIDO
            g.fillEllipse(hIn.x - handleSize / 2, hIn.y - handleSize / 2, handleSize, handleSize);
        }

        if (i < lfoNodes.size() - 1) {
            auto hOut = getPixelPos(node.pos + node.handleOut);
            g.drawLine(pPos.x, pPos.y, hOut.x, hOut.y, 1.0f); // CORREGIDO
            g.fillEllipse(hOut.x - handleSize / 2, hOut.y - handleSize / 2, handleSize, handleSize);
        }

        g.setColour(juce::Colours::white);
        g.fillEllipse(pPos.x - dotSize / 2, pPos.y - dotSize / 2, dotSize, dotSize);
    }
}

// ==============================================================================
// --- CONTROL VECTORIAL (RATÓN) ---
// ==============================================================================
void LfoModule::mouseDown(const juce::MouseEvent& event)
{
    auto areaInt = lfo2Area;
    areaInt.removeFromTop(20);
    auto canvasArea = areaInt.reduced(10).toFloat();

    draggedNode = -1;
    draggedCurve = -1; // <-- Reseteamos
    draggingHandleIn = false;
    draggingHandleOut = false;

    auto getPixelPos = [&](juce::Point<float> normPos) -> juce::Point<float> {
        return { canvasArea.getX() + (normPos.x * canvasArea.getWidth()),
                 canvasArea.getBottom() - (normPos.y * canvasArea.getHeight()) };
        };

    // 1. ¿Hemos clicado en una manecilla?
    for (int i = 0; i < lfoNodes.size(); ++i) {
        if (i > 0) {
            auto hInPix = getPixelPos(lfoNodes[i].pos + lfoNodes[i].handleIn);
            if (event.position.getDistanceFrom(hInPix) < 10.0f) {
                draggedNode = i;
                draggingHandleIn = true;
                return;
            }
        }
        if (i < lfoNodes.size() - 1) {
            auto hOutPix = getPixelPos(lfoNodes[i].pos + lfoNodes[i].handleOut);
            if (event.position.getDistanceFrom(hOutPix) < 10.0f) {
                draggedNode = i;
                draggingHandleOut = true;
                return;
            }
        }
    }

    // 2. ¿Hemos clicado en un nodo principal?
    for (int i = 0; i < lfoNodes.size(); ++i) {
        auto nodePix = getPixelPos(lfoNodes[i].pos);
        if (event.position.getDistanceFrom(nodePix) < 15.0f) {
            draggedNode = i;
            return;
        }
    }

    // 3. NUEVO: ¿Hemos clicado en medio de la curva para doblarla?
    if (canvasArea.contains(event.position)) {
        float normX = (event.position.x - canvasArea.getX()) / canvasArea.getWidth();
        for (int i = 0; i < lfoNodes.size() - 1; ++i) {
            if (normX >= lfoNodes[i].pos.x && normX <= lfoNodes[i + 1].pos.x) {
                draggedCurve = i;
                // Guardamos la posición inicial de las manecillas antes de empezar a tirar
                initialHandleOutY = lfoNodes[i].handleOut.y;
                initialHandleInY = lfoNodes[i + 1].handleIn.y;
                return;
            }
        }
    }
}

void LfoModule::mouseDrag(const juce::MouseEvent& event)
{
    // Protección anti-crashes
    if (draggedNode == -1 && draggedCurve == -1) return;

    // 1. OBTENEMOS EL ÁREA EXACTA DEL LIENZO
    auto areaInt = lfo2Area;
    areaInt.removeFromTop(20);
    auto canvasArea = areaInt.reduced(10).toFloat();

    // 2. CALCULAMOS LA POSICIÓN NORMALIZADA DEL RATÓN
    juce::Point<float> mouseNorm = {
        (event.position.x - canvasArea.getX()) / canvasArea.getWidth(),
        (canvasArea.getBottom() - event.position.y) / canvasArea.getHeight()
    };

    // Blindamos el ratón para que no se salga nunca de los bordes matemáticos
    mouseNorm.x = juce::jlimit(0.0f, 1.0f, mouseNorm.x);
    mouseNorm.y = juce::jlimit(0.0f, 1.0f, mouseNorm.y);

    // ==============================================================================
    // --- 3. LOGICA DE ARRASTRE SEGURO Y LOCAL ---
    // ==============================================================================

    // CASO A: Estamos moviendo un Nodo Principal (Punto Blanco)
    if (draggedNode != -1 && !draggingHandleIn && !draggingHandleOut)
    {
        auto& node = lfoNodes[draggedNode];
        float normX = mouseNorm.x;
        float normY = mouseNorm.y;

        // Límite horizontal estricto para que los nodos no se crucen
        if (draggedNode == 0) normX = 0.0f; // El primero está clavado a la izquierda
        else if (draggedNode == lfoNodes.size() - 1) normX = 1.0f; // El último clavado a la derecha
        else {
            normX = juce::jlimit(lfoNodes[draggedNode - 1].pos.x + 0.02f, lfoNodes[draggedNode + 1].pos.x - 0.02f, normX);
        }

        // Movemos el nodo y sus manecillas se mueven con él
        node.pos = { normX, normY };
    }

    // CASO B: Estamos moviendo una Manecilla Gris (Izquierda o Derecha)
    else if (draggedNode != -1 && (draggingHandleIn || draggingHandleOut))
    {
        auto& node = lfoNodes[draggedNode];

        if (draggingHandleIn) {
            node.handleIn = mouseNorm - node.pos;
            if (node.isSmooth && draggedNode < lfoNodes.size() - 1) node.handleOut = -node.handleIn;
        }
        else if (draggingHandleOut) {
            node.handleOut = mouseNorm - node.pos;
            if (node.isSmooth && draggedNode > 0) node.handleIn = -node.handleOut;
        }
    }

    // CASO C: Estamos "doblando" la línea desde el centro (MÁGICO Y LOCAL)
    else if (draggedCurve != -1)
    {
        // Calculamos el desplazamiento vertical desde que hicimos clic
        float dy = (event.mouseDownPosition.y - event.y) / canvasArea.getHeight();

        // Sensibilidad ajustada para un control suave
        float sensitivity = 1.3f;

        auto& nodeA = lfoNodes[draggedCurve];
        auto& nodeB = lfoNodes[draggedCurve + 1];

        // EL TRUCO MÁGICO: Cuando manipulas el segmento central, forzamos que 
        // los nodos NO sean "smooth" (simétricos) para que el movimiento sea local.
        nodeA.isSmooth = false;
        nodeB.isSmooth = false;

        // Doblamos las manecillas hacia arriba/abajo (Eje Y)
        // Usamos initialHandle...Y para que el movimiento sea acumulativo y suave
        nodeA.handleOut.y = initialHandleOutY + (dy * sensitivity);
        nodeB.handleIn.y = initialHandleInY + (dy * sensitivity);

        // Ajustamos automáticamente el ancho de la curva (Eje X) para que quede bonita
        float distNodesX = nodeB.pos.x - nodeA.pos.x;
        nodeA.handleOut.x = distNodesX * 0.45f;
        nodeB.handleIn.x = -(distNodesX * 0.45f);
    }

    // ==============================================================================
    // --- 4. ESCUDO DE SEGURIDAD ABSOLUTO (POST-PROCESADO) ---
    // ==============================================================================
    // Este bloque de seguridad se ejecuta SIEMPRE para blindar la interfaz y el audio.

    for (int i = 0; i < lfoNodes.size(); ++i) {
        auto& n = lfoNodes[i];

        // Filtro A: Blindaje del Techo y Suelo (Eje Y)
        // Por mucho que estiremos, las manecillas nunca saldrán de 0.0 - 1.0
        float handleInY = n.pos.y + n.handleIn.y;
        if (handleInY > 1.0f || handleInY < 0.0f) {
            float targetY = juce::jlimit(0.0f, 1.0f, handleInY) - n.pos.y;
            n.handleIn.y = targetY;
            // Si es smooth, su espejo también se ajusta en Y
            if (n.isSmooth) n.handleOut.y = -n.handleIn.y;
        }

        float handleOutY = n.pos.y + n.handleOut.y;
        if (handleOutY > 1.0f || handleOutY < 0.0f) {
            float targetY = juce::jlimit(0.0f, 1.0f, handleOutY) - n.pos.y;
            n.handleOut.y = targetY;
            if (n.isSmooth) n.handleIn.y = -n.handleOut.y;
        }

        // Filtro B: Blindaje Anti-Viajes-en-el-Tiempo (Eje X)
        // Las manecillas no pueden cruzar la mitad de la distancia hacia el nodo vecino.

        // Para el nodo actual, manecilla Out vs nodo siguiente
        if (i < lfoNodes.size() - 1) {
            float limitX = (lfoNodes[i + 1].pos.x - n.pos.x) * 0.5f;
            if (n.handleOut.x > limitX) n.handleOut.x = limitX;
            if (n.handleOut.x < 0.0f) n.handleOut.x = 0.0f; // Nunca hacia atrás
            // Si es smooth, su espejo se ajusta en X
            if (n.isSmooth) n.handleIn.x = -n.handleOut.x;
        }

        // Para el nodo actual, manecilla In vs nodo anterior
        if (i > 0) {
            float limitX = (n.pos.x - lfoNodes[i - 1].pos.x) * 0.5f;
            if (n.handleIn.x < -limitX) n.handleIn.x = -limitX;
            if (n.handleIn.x > 0.0f) n.handleIn.x = 0.0f; // Nunca hacia adelante
            if (n.isSmooth) n.handleOut.x = -n.handleIn.x;
        }
    }

    repaint();
}

void LfoModule::mouseDoubleClick(const juce::MouseEvent& event)
{
    auto areaInt = lfo2Area;
    areaInt.removeFromTop(20);
    auto canvasArea = areaInt.reduced(10).toFloat();

    auto getPixelPos = [&](juce::Point<float> normPos) -> juce::Point<float> {
        return { canvasArea.getX() + (normPos.x * canvasArea.getWidth()),
                 canvasArea.getBottom() - (normPos.y * canvasArea.getHeight()) };
        };

    // 1. Borrar nodo central si hacemos doble clic en él
    if (draggedNode != -1 && draggedNode != 0 && draggedNode != lfoNodes.size() - 1) {
        auto nodePix = getPixelPos(lfoNodes[draggedNode].pos);
        if (event.position.getDistanceFrom(nodePix) < 15.0f) {
            lfoNodes.erase(lfoNodes.begin() + draggedNode);
            draggedNode = -1;
            repaint();
            return;
        }
    }

    // 2. Si hacemos doble clic en el nodo (no manecilla), alternamos "Smooth" vs "Roto"
    if (draggedNode != -1 && !draggingHandleIn && !draggingHandleOut) {
        lfoNodes[draggedNode].isSmooth = !lfoNodes[draggedNode].isSmooth;
        return;
    }

    // 3. Crear nuevo nodo
    juce::Point<float> newPos = {
        juce::jlimit(0.0f, 1.0f, (event.position.x - canvasArea.getX()) / canvasArea.getWidth()),
        juce::jlimit(0.0f, 1.0f, (canvasArea.getBottom() - event.position.y) / canvasArea.getHeight())
    };

    LfoNode newNode;
    newNode.pos = newPos;
    newNode.handleIn = { -0.05f, 0.0f }; // Manecillas pequeñas por defecto
    newNode.handleOut = { 0.05f, 0.0f };

    for (auto it = lfoNodes.begin(); it != lfoNodes.end(); ++it) {
        if (it->pos.x > newNode.pos.x) {
            lfoNodes.insert(it, newNode);
            break;
        }
    }
    repaint();
}
