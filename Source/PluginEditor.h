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
#include "MasterModule.h"
#include "DistModule.h"
#include "BpmModule.h"
#include "LayerControlsModule.h"
#include "LayerMixerModule.h"
#include "FxFormantModule.h"
#include "ChoirModule.h"

class Granular_SynthAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::FileDragAndDropTarget,
    public juce::ChangeListener,
    public juce::AudioProcessorValueTreeState::Listener,
    public juce::Timer
{
public:
    Granular_SynthAudioProcessorEditor(Granular_SynthAudioProcessor&);
    ~Granular_SynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        if (parameterID.contains("POSITION") || parameterID.contains("GRAIN_SIZE") ||
            parameterID.contains("SHAPE") || parameterID.contains("MUTE") ||
            parameterID.contains("SOLO") || parameterID.contains("REC")) // <-- ¡AÑADIDO "REC"!
        {
            juce::MessageManager::callAsync([this] { repaint(); });
        }
    }

    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    MasterModule masterModule;
    BpmModule bpmModule;

    LayerControlsModule layer1Controls;
    LayerControlsModule layer2Controls;
    LayerControlsModule layer3Controls;
    LayerControlsModule layer4Controls;

    Granular_SynthAudioProcessor& audioProcessor;

    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    juce::AudioThumbnail thumbnailL2;
    juce::AudioThumbnail thumbnailL3;
    juce::AudioThumbnail thumbnailL4;

    EngineModule engineModule{ audioProcessor.apvts, "L1_" };
    ScanModule scanModule{ audioProcessor.apvts, "L1_" };
    SprayModule sprayModule{ audioProcessor.apvts, "L1_" };
    PitchModule pitchModule{ audioProcessor.apvts, "L1_" };
    FilterModule filterModule{ audioProcessor.apvts, "L1_" };
    EnvelopeModule envelopeModule{ audioProcessor.apvts, "L1_" };
    SpaceModule spaceModule{ audioProcessor.apvts, "L1_" };
    ChoirModule choirModule{ audioProcessor.apvts, "L1_" };
    LfoModule lfoModule{ audioProcessor.apvts };
    DistModule distModule{ audioProcessor.apvts, "L1_" };
    LayerMixerModule mixerModule1{ audioProcessor.apvts, "L1_" };
    FxFormantModule monk1{ audioProcessor.apvts, "L1_", 1 };
    FxFormantModule monk2{ audioProcessor.apvts, "L1_", 2 };
    FxFormantModule monk3{ audioProcessor.apvts, "L1_", 3 };
    FxFormantModule monk4{ audioProcessor.apvts, "L1_", 4 };

    double zoomFactor = 1.0;
    double viewStartRatio = 0.0;

    double zoomFactorL2 = 1.0;
    double viewStartRatioL2 = 0.0;

    double zoomFactorL3 = 1.0;
    double viewStartRatioL3 = 0.0;

    double zoomFactorL4 = 1.0;
    double viewStartRatioL4 = 0.0;

    int lastDragX = 0;
    int activeLayer = 1;
    
    bool ignoreDragForPosition = false;

    juce::Rectangle<int> matrixArea;
    juce::Rectangle<int> mixerArea;
    juce::Rectangle<int> masterArea;
    juce::Rectangle<int> distArea;
    juce::Rectangle<int> bpmArea;
    juce::Rectangle<int> fxArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Granular_SynthAudioProcessorEditor)
};
