#include "MatrixModule.h"

MatrixModule::MatrixModule(Granular_SynthAudioProcessor& p) : audioProcessor(p)
{
    audioProcessor.addChangeListener(this);

    juce::String sourceNames[6] = { "Vel", "ModW", "Aft", "Env2", "LFO1", "LFO2" };

    // 1. ETIQUETAS IZQUIERDAS (Más pequeñas y limpias)
    for (int r = 0; r < 6; ++r) {
        sourceLabels[r].setText(sourceNames[r], juce::dontSendNotification);
        sourceLabels[r].setJustificationType(juce::Justification::centredRight);
        sourceLabels[r].setColour(juce::Label::textColourId, juce::Colours::grey.brighter());
        sourceLabels[r].setFont(juce::Font(12.0f)); // Letra más pequeña
        addAndMakeVisible(sourceLabels[r]);
    }

    // 2. CABECERAS "SELECT" (Diseño pro, de 8 a 6)
    for (int c = 0; c < 6; ++c) {
        targetButtons[c].setButtonText("SEL");
        targetButtons[c].setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        targetButtons[c].setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.15f));
        targetButtons[c].setColour(juce::TextButton::textColourOffId, juce::Colours::grey);

        targetButtons[c].onClick = [this, c] {
            audioProcessor.setMappingColumn(c);
            // Mensaje claro en lugar de puntos suspensivos
            targetButtons[c].setButtonText("[ MAP ]");
            targetButtons[c].setColour(juce::ComboBox::outlineColourId, juce::Colours::red.withAlpha(0.6f));
            targetButtons[c].setColour(juce::TextButton::textColourOffId, juce::Colours::red);
            };
        addAndMakeVisible(targetButtons[c]);
    }

    // 3. LA MATRIZ DE NÚMEROS (De -100 a 100, sin barras gráficas)
    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 6; ++c) {
            // EL FIX: Usamos LinearBarVertical (toda la celda es arrastrable)
            gridSquares[r][c].setSliderStyle(juce::Slider::LinearBarVertical);

            // Le pasamos un enum válido a JUCE, aunque en LinearBar el texto siempre se centra
            gridSquares[r][c].setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);

            // LA MAGIA: Hacemos invisible el fondo y la barra del slider. ¡Solo queda el texto!
            gridSquares[r][c].setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
            gridSquares[r][c].setColour(juce::Slider::backgroundColourId, juce::Colours::transparentBlack);

            gridSquares[r][c].setRange(-100.0, 100.0, 1.0); // Enteros de -100 a 100
            gridSquares[r][c].setValue(0.0);

            // Colores del texto de la matriz inicial
            gridSquares[r][c].setColour(juce::Slider::textBoxTextColourId, juce::Colours::grey.withAlpha(0.5f));
            gridSquares[r][c].setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); // Sin borde

            // --- NUEVO ONVALUECHANGE CON COLORES DINÁMICOS ---
            gridSquares[r][c].onValueChange = [this, r, c] {
                float val = (float)gridSquares[r][c].getValue();
                audioProcessor.setGridDepth(r, c, val);

                // Averiguar qué parámetro controla esta columna para saber su color
                juce::String targetID = audioProcessor.targetColumns[c];
                juce::Colour layerColor = juce::Colours::white; // Por defecto (para parámetros globales)

                if (targetID.startsWith("L1")) layerColor = juce::Colours::cyan;
                else if (targetID.startsWith("L2")) layerColor = juce::Colours::magenta;
                else if (targetID.startsWith("L3")) layerColor = juce::Colours::orange;
                else if (targetID.startsWith("L4")) layerColor = juce::Colours::lime;

                // Color dinámico: Gris translúcido si es 0, color de capa si hay valor
                if (val == 0.0f) {
                    gridSquares[r][c].setColour(juce::Slider::textBoxTextColourId, juce::Colours::grey.withAlpha(0.3f));
                }
                else {
                    gridSquares[r][c].setColour(juce::Slider::textBoxTextColourId, layerColor);
                }
                };

            addAndMakeVisible(gridSquares[r][c]);
        }
    }
}

MatrixModule::~MatrixModule() {
    audioProcessor.removeChangeListener(this);
}

void MatrixModule::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().reduced(5);

    int labelWidth = 40;
    int buttonHeight = 18;

    auto gridArea = area.withTrimmedTop(buttonHeight).withTrimmedLeft(labelWidth);

    // Fondo y borde general
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(area.toFloat(), 4.0f);
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(area.toFloat(), 4.0f, 1.0f);

    // --- DIBUJAR LA CUADRÍCULA (Líneas blancas sutiles) ---
    int colWidth = gridArea.getWidth() / 6;
    int rowHeight = gridArea.getHeight() / 6;

    g.setColour(juce::Colours::white.withAlpha(0.08f)); // Líneas muy finas y translúcidas

    for (int i = 1; i < 6; ++i) {
        // Verticales
        g.drawLine(gridArea.getX() + (i * colWidth), gridArea.getY(),
            gridArea.getX() + (i * colWidth), gridArea.getBottom(), 1.0f);
        // Horizontales
        g.drawLine(gridArea.getX(), gridArea.getY() + (i * rowHeight),
            gridArea.getRight(), gridArea.getY() + (i * rowHeight), 1.0f);
    }
}

void MatrixModule::resized()
{
    auto area = getLocalBounds().reduced(5);

    int labelWidth = 40;   // Ajustado para texto más pequeño
    int buttonHeight = 18; // Ajustado para botones más sutiles

    auto topHeaderArea = area.removeFromTop(buttonHeight);
    topHeaderArea.removeFromLeft(labelWidth);

    auto leftLabelArea = area.removeFromLeft(labelWidth);
    auto gridArea = area;

    int colWidth = gridArea.getWidth() / 6;
    int rowHeight = gridArea.getHeight() / 6;

    // Botones Arriba
    for (int c = 0; c < 6; ++c) {
        targetButtons[c].setBounds(topHeaderArea.removeFromLeft(colWidth).reduced(1));
    }

    // Etiquetas y Números (centrados en la cuadrícula)
    for (int r = 0; r < 6; ++r) {
        sourceLabels[r].setBounds(leftLabelArea.removeFromTop(rowHeight));

        auto currentRowArea = gridArea.removeFromTop(rowHeight);
        for (int c = 0; c < 6; ++c) {
            // El reduced asegura que el número se quede en el centro de su celda
            gridSquares[r][c].setBounds(currentRowArea.removeFromLeft(colWidth).reduced(2));
        }
    }
}

void MatrixModule::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    for (int c = 0; c < 6; ++c)
    {
        // 1. OBTENER EL ID Y DECIDIR EL COLOR
        juce::String targetID = audioProcessor.targetColumns[c];
        juce::Colour layerColor = juce::Colours::white.withAlpha(0.8f);

        if (targetID.startsWith("L1")) layerColor = juce::Colours::cyan;
        else if (targetID.startsWith("L2")) layerColor = juce::Colours::magenta;
        else if (targetID.startsWith("L3")) layerColor = juce::Colours::orange;
        else if (targetID.startsWith("L4")) layerColor = juce::Colours::lime;

        // 2. ACTUALIZAR LA CABECERA (Botones SEL / MAP)
        if (audioProcessor.activeMappingColumn.load() != c)
        {
            if (targetID.isEmpty()) {
                targetButtons[c].setButtonText("SEL");
                targetButtons[c].setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
                targetButtons[c].setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.15f));
            }
            else {
                juce::String shortName = targetID.substring(targetID.indexOf("_") + 1).substring(0, 6);
                targetButtons[c].setButtonText(shortName);
                targetButtons[c].setColour(juce::TextButton::textColourOffId, layerColor);
                targetButtons[c].setColour(juce::ComboBox::outlineColourId, layerColor.withAlpha(0.6f));
            }
        }

        // 3. SINCRONIZAR LOS NÚMEROS Y SUS COLORES CON LA MEMORIA REAL
        for (int r = 0; r < 6; ++r)
        {
            // A. Extraemos el valor real de la memoria del procesador
            float realMemoryValue = audioProcessor.modDepths[r][c];

            // B. Forzamos al slider a mostrar ese número (sin disparar un bucle infinito)
            gridSquares[r][c].setValue(realMemoryValue, juce::dontSendNotification);

            // C. Repintamos el color
            if (realMemoryValue == 0.0f) {
                gridSquares[r][c].setColour(juce::Slider::textBoxTextColourId, juce::Colours::grey.withAlpha(0.3f));
            }
            else {
                gridSquares[r][c].setColour(juce::Slider::textBoxTextColourId, layerColor);
            }
        }
    }
}
