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
        presetHasData[i] = audioProcessor.doesPresetExist(i);
        //juce::String presetName = juce::String(i + 1).paddedLeft('0', 2);
        juce::String presetName = juce::String(i + 1);
        presetButtons[i].setButtonText(presetName);

        //presetButtons[i].setBorderSize(juce::BorderSize<int>(0));
       

        presetButtons[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A1A1D));
        presetButtons[i].setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
        presetButtons[i].setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.1f));

        addAndMakeVisible(presetButtons[i]);
        presetButtons[i].getProperties().set("minimumHeight", 10);

        presetButtons[i].addMouseListener(this, false);

        presetButtons[i].onClick = [this, i] {
            currentPresetIndex = i;

            // 1. Detectamos si es un Doble Clic (menos de 300ms entre clics en el mismo botón)
            juce::uint32 now = juce::Time::getMillisecondCounter();
            if (lastClickedIndex == i && (now - lastClickTime) < 300)
            {
                // ¡DOBLE CLIC! Actuamos igual que el botón LOAD
                if (presetHasData[i]) {
                    audioProcessor.loadPreset(i);
                }
                else {
                    audioProcessor.initSynth(); // Si está vacío, carga el Init
                }
                loadedPresetIndex = i;
            }

            // 2. Guardamos los datos para el próximo clic
            lastClickedIndex = i;
            lastClickTime = now;

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
        if (currentPresetIndex != -1) {
            if (presetHasData[currentPresetIndex]) {
                audioProcessor.loadPreset(currentPresetIndex);
            }
            else {
                audioProcessor.initSynth(); // NUEVO: Si está vacío, reset.
            }
            loadedPresetIndex = currentPresetIndex;
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

void PresetModule::mouseDown(const juce::MouseEvent& event)
{
    // 1. ¿Es un clic derecho? (JUCE lo llama "PopupMenu")
    if (event.mods.isPopupMenu())
    {
        // 2. Buscamos a qué botón exacto se le hizo el clic
        for (int i = 0; i < 16; ++i)
        {
            if (event.originalComponent == &presetButtons[i])
            {
                // 3. Solo abrimos el menú si ese botón tiene un preset guardado
                if (presetHasData[i])
                {
                    juce::PopupMenu menu;
                    // Añadimos la opción de borrar (ID 1, Texto "DELETE Preset X")
                    menu.addItem(1, "DELETE Preset " + juce::String(i + 1));

                    // Mostramos el menú anclado al botón
                    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetButtons[i]),
                        [this, i](int result)
                        {
                            if (result == 1) // Si el usuario hizo clic izquierdo en "DELETE"
                            {
                                // A. Borramos el archivo del disco duro
                                audioProcessor.deletePreset(i);

                                // B. Le decimos a la pantalla que ese botón vuelve a estar vacío
                                presetHasData[i] = false;

                                // C. Si el preset que acabamos de borrar era el que estaba sonando...
                                // ¡Vaciamos el sintetizador para que no suene un audio fantasma!
                                if (loadedPresetIndex == i || currentPresetIndex == i) {
                                    audioProcessor.initSynth();
                                    loadedPresetIndex = -1;
                                    currentPresetIndex = -1;
                                }

                                // D. Repintamos las lucecitas de los botones
                                updatePresetButtonColors();
                            }
                        });
                }
                break; // Ya encontramos el botón, dejamos de buscar
            }
        }
    }
}
