/*
  ==============================================================================

    MatrixModule.h
    Created: 21 Apr 2026 1:04:29am
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MatrixModule : public juce::Component, public juce::ChangeListener
{
public:
    MatrixModule(Granular_SynthAudioProcessor& p);
    ~MatrixModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    Granular_SynthAudioProcessor& audioProcessor;

    juce::Label sourceLabels[6];
    juce::TextButton targetButtons[6]; // <--- AHORA SON 6
    juce::Slider gridSquares[6][6];    // <--- AHORA ES 6x6

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MatrixModule)
};
