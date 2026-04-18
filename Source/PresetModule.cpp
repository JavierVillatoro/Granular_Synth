/*
  ==============================================================================

    PresetModule.cpp
    Created: 18 Apr 2026 1:10:21am
    Author:  franc

  ==============================================================================
*/

#include "PresetModule.h"

PresetModule::PresetModule(Granular_SynthAudioProcessor& p) : audioProcessor(p)
{
    // 1. Configuramos los 16 botones
    for (int i = 0; i < 16; ++i)
    {
        //juce::String presetName = juce::String(i + 1).paddedLeft('0', 2);
        juce::String presetName = juce::String(i + 1);
        presetButtons[i].setButtonText(presetName);

        //presetButtons[i].setBorderSize(juce::BorderSize<int>(0));
       

        presetButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1D));
        presetButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
        presetButtons[i].setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.1f));

        addAndMakeVisible(presetButtons[i]);
        presetButtons[i].getProperties().set("minimumHeight", 10);

        presetButtons[i].onClick = [this, i] {
            currentPresetIndex = i;
            // Aquí en la Fase 2 llamaremos a audioProcessor.loadPreset(i);
            updatePresetButtonColors();
            };
    }

    // 2. Configuramos la Action Bar
    auto setupActionButton = [&](juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1D));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
        addAndMakeVisible(btn);
        };

    setupActionButton(saveButton);
    setupActionButton(loadButton);
    setupActionButton(initButton);

    //initButton.onClick = [this] {
        //currentPresetIndex = -1;
        //updatePresetButtonColors();
        // Aquí en la Fase 2 llamaremos a audioProcessor.initSynth();
        //};

    // --- LÓGICA DEL BOTÓN SAVE ---
    saveButton.onClick = [this] {
        if (currentPresetIndex != -1) {
            audioProcessor.savePreset(currentPresetIndex);
            presetHasData[currentPresetIndex] = true;
            loadedPresetIndex = currentPresetIndex; // NUEVO: Al guardar, se convierte en el cargado
            updatePresetButtonColors();
        }
        };

    // --- LÓGICA DEL BOTÓN LOAD ---
    loadButton.onClick = [this] {
        if (currentPresetIndex != -1 && presetHasData[currentPresetIndex]) {
            audioProcessor.loadPreset(currentPresetIndex);
            loadedPresetIndex = currentPresetIndex; // NUEVO: Al cargar, se convierte en el cargado
            updatePresetButtonColors();
        }
        };

    // --- LÓGICA DEL BOTÓN INIT ---
    initButton.onClick = [this] {
        audioProcessor.initSynth();
        currentPresetIndex = -1;
        loadedPresetIndex = -1; // NUEVO: Al resetear, no hay nada cargado
        updatePresetButtonColors();
        };
}

PresetModule::~PresetModule() {}

void PresetModule::paint(juce::Graphics& g)
{
    // No necesitamos pintar fondo porque el Editor ya pinta el fondo oscuro
    // Pero si quieres pintar un borde general para todo el módulo, lo haríamos aquí.
}

void PresetModule::resized()
{
    auto area = getLocalBounds().reduced(4); // Margen de respiración

    // Fila inferior (25%) para SAVE / LOAD / INIT
    auto actionBar = area.removeFromBottom(area.getHeight() * 0.25f);

    int actionBtnWidth = actionBar.getWidth() / 3;
    saveButton.setBounds(actionBar.removeFromLeft(actionBtnWidth).reduced(2));
    loadButton.setBounds(actionBar.removeFromLeft(actionBtnWidth).reduced(2));
    initButton.setBounds(actionBar.reduced(2));

    area.removeFromBottom(2); // Separación

    // El 75% superior para la cuadrícula 2x8
    int presetBtnWidth = area.getWidth() / 8;
    int presetBtnHeight = area.getHeight() / 2;

    int idx = 0;
    for (int row = 0; row < 2; ++row)
    {
        auto rowArea = area.removeFromTop(presetBtnHeight);
        for (int col = 0; col < 8; ++col)
        {
            presetButtons[idx].setBounds(rowArea.removeFromLeft(presetBtnWidth).reduced(1));
            idx++;
        }
    }
}

void PresetModule::setLayer(int layer)
{
    activeLayer = layer;
    updatePresetButtonColors();
}

void PresetModule::updatePresetButtonColors()
{
    juce::Colour activeColor = juce::Colours::cyan;
    if (activeLayer == 2) activeColor = juce::Colours::magenta;
    if (activeLayer == 3) activeColor = juce::Colours::orange;
    if (activeLayer == 4) activeColor = juce::Colours::lime;

    for (int i = 0; i < 16; ++i)
    {
        if (i == loadedPresetIndex) {
            // 1. EL QUE ESTÁ SONANDO (Fondo tintado, borde brillante del color de la capa)
            presetButtons[i].setColour(juce::TextButton::buttonColourId, activeColor.withAlpha(0.3f));
            presetButtons[i].setColour(juce::ComboBox::outlineColourId, activeColor);
            presetButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
        else if (i == currentPresetIndex) {
            // 2. EL QUE HAS SELECCIONADO (Para guardar/cargar) - Contorno blanco puro, sin fondo
            presetButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1D));
            presetButtons[i].setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.8f));
            presetButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        }
        else if (presetHasData[i]) {
            // 3. TIENE DATOS (Contorno sutil, texto gris claro)
            presetButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1D));
            presetButtons[i].setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.15f));
            presetButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::grey.brighter());
        }
        else {
            // 4. TOTALMENTE VACÍO (Sin contorno, texto muy oscuro)
            presetButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1D));
            presetButtons[i].setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
            presetButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::grey.darker());
        }
    }
    repaint();
}
