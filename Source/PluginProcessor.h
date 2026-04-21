/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GranularVoice.h"
#include "LfoModule.h"
#include "TcpReceiver.h"

class Granular_SynthAudioProcessor : public juce::AudioProcessor,
                                     public juce::AudioProcessorValueTreeState::Listener,
                                     public juce::ChangeBroadcaster,
                                     public juce::OSCReceiver, 
                                     public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>,
                                     public juce::Timer
{
public:
    Granular_SynthAudioProcessor();
    ~Granular_SynthAudioProcessor() override;

    void oscMessageReceived(const juce::OSCMessage& message) override;

    std::atomic<int> uiLayerRequested{ 0 };

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void loadFile(const juce::String& path, int layerIndex);

    void clearFile(int layerIndex);

    juce::AudioFormatManager& getFormatManager() { return formatManager; }
    void releaseResources() override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void savePreset(int presetIndex);
    void loadPreset(int presetIndex);
    void deletePreset(int presetIndex);
    bool doesPresetExist(int presetIndex);
    void initSynth();

    // --- SISTEMA DE MODULACIÓN (MATRIX GRID) ---
    // 6 Fuentes fijas: 0=Vel, 1=ModWheel, 2=Aftertouch, 3=Env2, 4=LFO1, 5=LFO2
    // 8 Destinos dinámicos (Columnas)
    juce::String targetColumns[6] = { "", "", "", "", "", "" };

    // Matriz 2D: modDepths[fuente][destino]
    float modDepths[6][8] = { {0.0f} };

    std::atomic<int> activeMappingColumn{ -1 }; // -1 = No estamos mapeando

    void setMappingColumn(int colIndex);
    void setGridDepth(int sourceRow, int targetCol, float depth);

    // Funciones que usará la interfaz
    //void setMappingMode(int slotIndex);
    //void setModSource(int slotIndex, int sourceID);
    //void setModDepth(int slotIndex, float depth);

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    
    juce::String getLocalIPAddress()
    {
        auto allIPs = juce::IPAddress::getAllAddresses();
        juce::String bestIP = "Desconocida (Abre el WiFi/USB)";

        for (auto& ip : allIPs) {
            juce::String ipStr = ip.toString();

            // Filtramos localhost (127), raras (169), Multicast (224) 
            // Y AHORA: Filtramos 192.168.56.x (Adaptadores fantasma de VirtualBox/Docker)
            if (ip != juce::IPAddress::local() && !ipStr.startsWith("169.") &&
                !ipStr.startsWith("224.") && !ipStr.startsWith("192.168.56."))
            {
                // Prioridad a redes locales típicas
                if (ipStr.startsWith("192.168.") || ipStr.startsWith("10.") || ipStr.startsWith("172.")) {
                    return ipStr; // Devuelve la primera BUENA de verdad
                }
                bestIP = ipStr;
            }
        }
        return bestIP;
    }

    juce::AudioProcessorValueTreeState apvts;

    // --- LOS 4 JEFES ---
    juce::Synthesiser& getSynthesiserL1() { return synthL1; }
    juce::Synthesiser& getSynthesiserL2() { return synthL2; }
    juce::Synthesiser& getSynthesiserL3() { return synthL3; }
    juce::Synthesiser& getSynthesiserL4() { return synthL4; }

    // --- DISCOS DUROS PARA LAS CAPAS ---
    juce::AudioBuffer<float> audioBufferL1;
    juce::AudioBuffer<float> audioBufferL2;
    juce::AudioBuffer<float> audioBufferL3;
    juce::AudioBuffer<float> audioBufferL4;

    juce::AudioBuffer<float>& getAudioBufferL1() { return audioBufferL1; }
    juce::AudioBuffer<float>& getAudioBufferL2() { return audioBufferL2; }
    juce::AudioBuffer<float>& getAudioBufferL3() { return audioBufferL3; }
    juce::AudioBuffer<float>& getAudioBufferL4() { return audioBufferL4; }

    bool isAudioLoadedL1 = false;
    bool isAudioLoadedL2 = false;
    bool isAudioLoadedL3 = false;
    bool isAudioLoadedL4 = false;

    // Anti clicks (Lock)
    std::atomic<bool> isUpdatingBufferL1{ false };
    std::atomic<bool> isUpdatingBufferL2{ false };
    std::atomic<bool> isUpdatingBufferL3{ false };
    std::atomic<bool> isUpdatingBufferL4{ false };

    juce::String lastLoadedFilePathL1 = "";
    juce::String lastLoadedFilePathL2 = "";
    juce::String lastLoadedFilePathL3 = "";
    juce::String lastLoadedFilePathL4 = "";

    // --- CÁMARAS DE ZOOM INDEPENDIENTES ---
    std::atomic<float> windowStartRatioL1{ 0.0f };
    std::atomic<float> windowLengthRatioL1{ 1.0f };

    std::atomic<float> windowStartRatioL2{ 0.0f };
    std::atomic<float> windowLengthRatioL2{ 1.0f };

    std::atomic<float> windowStartRatioL3{ 0.0f };
    std::atomic<float> windowLengthRatioL3{ 1.0f };

    std::atomic<float> windowStartRatioL4{ 0.0f };
    std::atomic<float> windowLengthRatioL4{ 1.0f };

    std::vector<LfoNode> savedLfoNodes;
    bool isLfoSaved = false;

    float globalLfo1Value = 0.0f;
    float globalLfo2Value = 0.0f;

    static constexpr int LFO_TABLE_SIZE = 2048;
    std::array<float, LFO_TABLE_SIZE> lfo2Table = { 0.0f };

    std::atomic<float> visualLpfCutoff{ 20000.0f };
    std::atomic<float> visualHpfCutoff{ 20.0f };
    std::atomic<float> visualMeterL{ -60.0f };
    std::atomic<float> visualMeterR{ -60.0f };

    // --- HERRAMIENTAS ANTI-CLICK ---
    void timerCallback() override;
    int pendingAction = 0; // 1 = Hacer Init, 2 = Borrar Capa
    int pendingLayerToClear = 0;

    // --- ESTADO MIDI PARA LA MATRIZ ---
    std::atomic<float> globalModWheel{ 0.0f };
    std::atomic<float> globalAftertouch{ 0.0f };

    // Función puente que calculará la modulación exacta para cualquier parámetro
    float getMatrixModulation(const juce::String& targetParamID, float voiceVelocity, float voiceEnv2, float voiceLFO1, float voiceLFO2);


private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::AudioFormatManager formatManager;

    juce::Synthesiser synthL1;
    juce::Synthesiser synthL2;
    juce::Synthesiser synthL3;
    juce::Synthesiser synthL4;

    juce::dsp::Reverb reverbL1;
    juce::dsp::Reverb reverbL2;
    juce::dsp::Reverb reverbL3;
    juce::dsp::Reverb reverbL4;

    juce::AudioBuffer<float> renderBufferL1;
    juce::AudioBuffer<float> renderBufferL2;
    juce::AudioBuffer<float> renderBufferL3;
    juce::AudioBuffer<float> renderBufferL4;

    juce::dsp::Limiter<float> masterLimiter;

    float lfo1Phase = 0.0f;
    float lfo2Phase = 0.0f;

    double currentSampleRate = 44100.0;
    double currentBPM = 120.0;
    bool isPlaying = false;

    bool lastPlayState = false;
    bool lastHoldState = false;

    bool lastPlayStateL2 = false;
    bool lastHoldStateL2 = false;

    bool lastPlayStateL3 = false;
    bool lastHoldStateL3 = false;

    bool lastPlayStateL4 = false;
    bool lastHoldStateL4 = false;

    float lastMuteGainL1 = 1.0f;
    float lastMuteGainL2 = 1.0f;
    float lastMuteGainL3 = 1.0f;
    float lastMuteGainL4 = 1.0f;

    
    std::unique_ptr<TcpReceiver> tcpReceiver;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Granular_SynthAudioProcessor)
};
