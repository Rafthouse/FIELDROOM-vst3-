#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters/Parameters.h"
#include "DSP/BinauralRenderer.h"
#include "DSP/RoomEngine.h"
#include "DSP/HeadphoneCalibration.h"
#include "DSP/MonitoringUtilities.h"

namespace fieldroom {

/**
 * Signal chain (stereo, per sample block):
 *
 *   input -> [spatial: binaural loudspeaker matrix -> room] --(blend with EQ-only dry path)-->
 *         -> headphone calibration -> monitoring (routing, swap, DIM, trim, meters) -> output limiter
 *
 * Master bypass passes the input through untouched and keeps only the meters running.
 */
class FieldroomAudioProcessor : public juce::AudioProcessor {
public:
    FieldroomAudioProcessor();
    ~FieldroomAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Host bypass button drives the same parameter as the in-plugin bypass
    juce::AudioProcessorParameter* getBypassParameter() const override;

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

    // Snapshot of the latest metering values (safe to call from the UI thread)
    MeterLevels getMeterLevels() const noexcept { return monitoringUtilities.getMeterLevels(); }

    double getCurrentSampleRate() const noexcept { return currentSampleRate.load(std::memory_order_relaxed); }

private:
    void processChunk(float* left, float* right, int numSamples) noexcept;

    juce::AudioProcessorValueTreeState apvts;

    BinauralRenderer binauralRenderer;
    RoomEngine roomEngine;
    HeadphoneCalibration headphoneCalibration;
    MonitoringUtilities monitoringUtilities;
    juce::dsp::Limiter<float> outputLimiter;

    juce::AudioBuffer<float> dryBuffer;          // copy of the input, preallocated in prepareToPlay()
    juce::SmoothedValue<float> spatialMix;       // 1 = virtual speakers, 0 = EQ-only dry path
    int maxBlockSize{512};
    bool wasBypassed{false};

    std::atomic<double> currentSampleRate{0.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldroomAudioProcessor)
};

} // namespace fieldroom
