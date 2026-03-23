/*
  ==============================================================================

    FilterModule.h
    Created: 18 Mar 2026 9:10:29pm
    Author:  franc

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class FilterModule : public juce::Component
{
public:
    FilterModule(juce::AudioProcessorValueTreeState& apvts);
    ~FilterModule() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // --- MAGIA INTERACTIVA (Ratón) ---
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;

    // --- LOS 4 KNOBS ---
    juce::Slider lpfKnob, resLpfKnob;
    juce::Slider hpfKnob, resHpfKnob;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lpfAttach, resLpfAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfAttach, resHpfAttach;

    // --- ZONAS VISUALES ---
    juce::Rectangle<int> graphArea;
    int draggedDot = -1; // -1 = Nada, 0 = Punto HPF, 1 = Punto LPF

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterModule)
};
