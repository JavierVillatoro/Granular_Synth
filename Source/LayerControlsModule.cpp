/*
  ==============================================================================

    LayerControlsModule.cpp

  ==============================================================================
*/

#include "LayerControlsModule.h"

// =============================================================================
// --- ESTILO DEL BOTÓN REC (Forma ovalada + Texto dinámico) ---
// =============================================================================
class RecStyle : public juce::LookAndFeel_V4 {
public:
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();
        float cornerSize = bounds.getHeight() / 2.0f; // Radio perfecto para óvalo

        if (button.getToggleState()) {
            // SI ESTÁ GRABANDO (ON)
            g.setColour(juce::Colours::red.withAlpha(0.9f));
            g.fillRoundedRectangle(bounds, cornerSize);

            // Borde blanco radiante
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize, 2.0f);
        }
        else {
            // SI ESTÁ APAGADO (OFF)
            g.setColour(juce::Colours::transparentBlack);
            g.fillRoundedRectangle(bounds, cornerSize);

            g.setColour(juce::Colours::white.withAlpha(0.2f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
        }
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool) override {
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        // Texto blanco si graba, rojo apagado si no
        g.setColour(button.getToggleState() ? juce::Colours::white : juce::Colours::red.withAlpha(0.6f));
        g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }
};

// =============================================================================
// --- ESTILO DEL MENÚ DESPLEGABLE  ---
// =============================================================================
class ComboStyle : public juce::LookAndFeel_V4 {
public:
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
        int buttonX, int buttonY, int buttonW, int buttonH,
        juce::ComboBox& box) override
    {
        // Fondo y Borde sutil
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(0, 0, width, height, 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, 4.0f, 1.0f);

        // --- Lógica del texto corto ---
        juce::String fullText = box.getText();
        juce::String shortText = "DAW"; // Por defecto

        if (fullText.contains("WIFI")) shortText = "WIFI";
        else if (fullText.contains("USB")) shortText = "USB";

        // Dibujamos nuestro texto personalizado
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::Font(12.0f, juce::Font::bold).withExtraKerningFactor(0.05f));
        g.drawText(shortText, 0, 0, width - buttonW, height, juce::Justification::centred, false);

        // Dibujar la flechita
        juce::Path path;
        float arrowX = width - 14.0f;
        float arrowY = height / 2.0f - 2.0f;
        path.addTriangle(arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f, arrowY + 5.0f);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.fillPath(path);
    }
};


// =============================================================================
// --- CLASE PRINCIPAL ---
// =============================================================================

LayerControlsModule::LayerControlsModule(juce::AudioProcessorValueTreeState& apvts, juce::String prefix)
    : apvtsRef(apvts), paramPrefix(prefix)
{
    setInterceptsMouseClicks(false, true);

    juce::Colour mainColor = juce::Colours::cyan;
    if (paramPrefix == "L2_") mainColor = juce::Colours::magenta;
    if (paramPrefix == "L3_") mainColor = juce::Colours::orange;
    if (paramPrefix == "L4_") mainColor = juce::Colours::lime;

    auto setupButton = [this, mainColor](juce::TextButton& btn, juce::String text, juce::String paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach, juce::Colour onColor) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
        btn.setColour(juce::TextButton::buttonOnColourId, onColor.withAlpha(0.3f));
        btn.setColour(juce::TextButton::textColourOnId, onColor);
        addAndMakeVisible(btn);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvtsRef, paramId, btn);
        };

    // 1. Botones Estándar
    setupButton(playButton, "PLAY", paramPrefix + "PLAY", playAttach, mainColor);
    setupButton(midiButton, "MIDI", paramPrefix + "MIDI", midiAttach, mainColor);
    setupButton(holdButton, "HOLD", paramPrefix + "HOLD", holdAttach, mainColor);
    setupButton(muteButton, "MUTE", paramPrefix + "MUTE", muteAttach, mainColor);

    // 2. Botón REC (Le ponemos su nuevo traje)
    setupButton(recButton, "REC", paramPrefix + "REC", recAttach, juce::Colours::red);
    recStyle = std::make_unique<RecStyle>();
    recButton.setLookAndFeel(recStyle.get());

    // 3. Menú Desplegable
    recModeBox.addItem("DAW / MIC IN", 1);
    recModeBox.addItem("WIFI FILE (TCP)", 2);
    // ¡BORRAMOS LA LÍNEA DEL USB AQUÍ!

    recModeBox.setColour(juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    recModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    recModeBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);

    comboStyle = std::make_unique<ComboStyle>();
    recModeBox.setLookAndFeel(comboStyle.get());
    addAndMakeVisible(recModeBox);

    // Cambiamos REC_MODE por R_MODE para que se enlace bien
    recModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvtsRef, paramPrefix + "R_MODE", recModeBox);

    // ==========================================================
    // --- ESTILO DE LOS BOTONES (GRN, PLY, CUE) ---
    // ==========================================================
    auto setupModeButton = [this, mainColor](juce::TextButton& btn, juce::String text, bool isRadio) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);
        if (isRadio) btn.setRadioGroupId(199); // Magia: Los botones con ID 199 se apagan entre sí

        btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
        btn.setColour(juce::TextButton::buttonOnColourId, mainColor.withAlpha(0.3f));
        btn.setColour(juce::TextButton::textColourOnId, mainColor);
        addAndMakeVisible(btn);
        };

    // Los configuramos llamando a la función
    setupModeButton(grnButton, "GRN", true);  // true = pertenece al grupo de radio
    setupModeButton(plyButton, "PLY", true);  // true = pertenece al grupo de radio
    setupModeButton(cueButton, "CUE", false); // false = es un interruptor libre

    // Colores especiales para CUE (Amarillo clásico de DJ Mixer)
    cueButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow.withAlpha(0.4f));
    cueButton.setColour(juce::TextButton::textColourOnId, juce::Colours::yellow);

    // Encendemos el modo Granular por defecto sin enviar mensajes (solo visual)
    grnButton.setToggleState(true, juce::dontSendNotification);

    // 4. Slider de Paneo (¡DEVUELVO LOS COLORES A LOS DOTS!)
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    // AQUÍ ESTÁ LA LÍNEA QUE FALTABA:
    panSlider.setColour(juce::Slider::thumbColourId, mainColor);
    panSlider.setColour(juce::Slider::trackColourId, juce::Colours::white.withAlpha(0.3f));
    panSlider.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(panSlider);
    panAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvtsRef, paramPrefix + "PAN", panSlider);
}

LayerControlsModule::~LayerControlsModule() {
    recButton.setLookAndFeel(nullptr);
    recModeBox.setLookAndFeel(nullptr);
}

void LayerControlsModule::paint(juce::Graphics& g) {}

void LayerControlsModule::resized()
{
    auto area = getLocalBounds();

    int stdBtnW = 40;
    int gap = 2;
    int topMargin = 8;
    int sideMargin = 10;

    
    playButton.setBounds(sideMargin, topMargin, stdBtnW, 20);
    midiButton.setBounds(sideMargin + stdBtnW + gap, topMargin, stdBtnW, 20);
    holdButton.setBounds(sideMargin + (stdBtnW * 2) + (gap * 2), topMargin, stdBtnW, 20);

    int transportWidth = (stdBtnW * 3) + (gap * 2);
    panSlider.setBounds(sideMargin, area.getHeight() - 25, transportWidth, 20);

    
    int rightEdge = area.getWidth() - sideMargin;

    muteButton.setBounds(rightEdge - stdBtnW, topMargin, stdBtnW, 20);
    recButton.setBounds(rightEdge - (stdBtnW * 2) - gap, topMargin, stdBtnW, 20);

    int comboW = 75; // Más corto, ya que ahora solo pone DAW, UDP o TCP
    recModeBox.setBounds(rightEdge - (stdBtnW * 2) - gap - comboW - gap, topMargin, comboW, 20);

    int bottomY = area.getHeight() - 25;

    int comboX = rightEdge - (stdBtnW * 2) - gap - comboW - gap;

    int newBtnW = 37;

    grnButton.setBounds(comboX, bottomY, newBtnW, 20);
    plyButton.setBounds(comboX + newBtnW + gap, bottomY, newBtnW, 20);
    cueButton.setBounds(comboX + (newBtnW * 2) + (gap * 2), bottomY, newBtnW, 20);
}
