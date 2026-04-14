/*
  ==============================================================================

    TcpReceiver.h
    Created: 14 Apr 2026 2:14:46am
    Author:  franc

  ==============================================================================
*/


#pragma once
#include <JuceHeader.h>

// Le decimos que esta clase existirá, para no crear dependencias circulares
class Granular_SynthAudioProcessor;

class TcpReceiver : public juce::Thread
{
public:
    TcpReceiver(Granular_SynthAudioProcessor& p);
    ~TcpReceiver() override;

    void startListening(int layerToUpdate);
    void stopListening();
    void run() override;

private:
    Granular_SynthAudioProcessor& processorRef;
    std::unique_ptr<juce::StreamingSocket> listenerSocket;
    int targetLayer = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TcpReceiver)
};
