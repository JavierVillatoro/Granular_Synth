/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class Granular_SynthAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                            public juce::FileDragAndDropTarget,
                                            public juce::ChangeListener
{
public:
    Granular_SynthAudioProcessorEditor (Granular_SynthAudioProcessor&);
    ~Granular_SynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void changeListenerCallback(juce::ChangeBroadcaster* source) override; // Cuando audio listo para dibujarse

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Granular_SynthAudioProcessor& audioProcessor;

    juce::AudioThumbnailCache thumbnailCache; 
    juce::AudioThumbnail thumbnail;           

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Granular_SynthAudioProcessorEditor)
};
