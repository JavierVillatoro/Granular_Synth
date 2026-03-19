/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ScanModule.h"
#include "EngineModule.h"
#include "SprayModule.h"
#include "PitchModule.h"
#include "FilterModule.h"
#include "EnvelopeModule.h"
#include "SpaceModule.h"
#include "LfoModule.h"

//==============================================================================
/**
*/
class Granular_SynthAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                            public juce::FileDragAndDropTarget,
                                            public juce::ChangeListener,
                                            public juce::AudioProcessorValueTreeState::Listener,
                                            public juce::Timer
{
public:
    Granular_SynthAudioProcessorEditor (Granular_SynthAudioProcessor&);
    ~Granular_SynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override; // Cuando audio listo para dibujarse

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        // Añadimos "SHAPE" a la condición
        if (parameterID == "POSITION" || parameterID == "GRAIN_SIZE" || parameterID == "SHAPE")
        {
            juce::MessageManager::callAsync([this] { repaint(); });
        }
    }

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    //void mouseDown(const juce::MouseEvent& event) override;
    //void mouseDrag(const juce::MouseEvent& event) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Granular_SynthAudioProcessor& audioProcessor;

    juce::AudioThumbnailCache thumbnailCache; 
    juce::AudioThumbnail thumbnail;
    //juce::Slider positionKnob;

    //std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> positionAttachment;

    EngineModule engineModule{ audioProcessor.apvts };
    ScanModule scanModule{ audioProcessor.apvts };
    SprayModule sprayModule{ audioProcessor.apvts };
    PitchModule pitchModule{ audioProcessor.apvts };
    FilterModule filterModule{ audioProcessor.apvts };
    EnvelopeModule envelopeModule{ audioProcessor.apvts };
    SpaceModule spaceModule{ audioProcessor.apvts };
    LfoModule lfoModule{ audioProcessor.apvts };

    // --- VARIABLES DE ZOOM Y NAVEGACIÓN ---
    double zoomFactor = 1.0;     // 1.0 = vista completa, 10.0 = zoom x10
    double viewStartRatio = 0.0; // De 0.0 a 1.0, indica qué parte del audio está a la izquierda de la pantalla
    int lastDragX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Granular_SynthAudioProcessorEditor)
};
