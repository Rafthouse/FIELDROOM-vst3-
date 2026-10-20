#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters/Parameters.h"
#include "DSP/BinauralRenderer.h"
#include "DSP/RoomEngine.h"
#include "DSP/HeadphoneCalibration.h"
#include "DSP/MonitoringUtilities.h"

namespace fieldroom {

class FieldroomAudioProcessor : public juce::AudioProcessor,
                                public juce::AudioProcessorValueTreeState::Listener {
public:
    FieldroomAudioProcessor();
    ~FieldroomAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FIELDROOM"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
    const juce::String getProgramName(int index) override { juce::ignoreUnused(index); return {}; }
    void changeProgramName(int index, const juce::String& newName) override { juce::ignoreUnused(index, newName); }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const MeterLevels& getMeterLevels() const noexcept { return currentMeters; }

    HeadphoneCalibration& getCalibration() noexcept { return headphoneCalibration; }

    // APVTS Listener callback for off-audio-thread state preparation
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateCalibrationStateFromAPVTS();

private:
    juce::AudioProcessorValueTreeState apvts;

    BinauralRenderer binauralRenderer;
    RoomEngine roomEngine;
    HeadphoneCalibration headphoneCalibration;
    MonitoringUtilities monitoringUtilities;

    // Intermediate scratch buffers for real-time safety
    juce::AudioBuffer<float> roomScratchBuffer;

    MeterLevels currentMeters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldroomAudioProcessor)
};

} // namespace fieldroom
