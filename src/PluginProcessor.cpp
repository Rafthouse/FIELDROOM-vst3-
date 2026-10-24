#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace fieldroom {

namespace {
constexpr float kOutputCeilingDb = -0.3f;  // output limiter threshold
constexpr float kOutputReleaseMs = 100.0f;
constexpr double kSpatialFadeSeconds = 0.03;
} // namespace

FieldroomAudioProcessor::FieldroomAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "FieldroomParameters", ParameterManager::createParameterLayout()) {
}

bool FieldroomAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

juce::AudioProcessorParameter* FieldroomAudioProcessor::getBypassParameter() const {
    return apvts.getParameter(ParameterIDs::masterBypass);
}

void FieldroomAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Everything that allocates is done here; processBlock() only reads and writes preallocated memory
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);
    maxBlockSize = std::max(1, samplesPerBlock);

    dryBuffer.setSize(2, maxBlockSize, false, false, true);

    binauralRenderer.prepare(sampleRate, maxBlockSize);
    roomEngine.prepare(sampleRate, maxBlockSize);
    headphoneCalibration.prepare(sampleRate, maxBlockSize);
    monitoringUtilities.prepare(sampleRate, maxBlockSize);

    juce::dsp::ProcessSpec spec{};
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 2;
    outputLimiter.prepare(spec);
    outputLimiter.setThreshold(kOutputCeilingDb);
    outputLimiter.setRelease(kOutputReleaseMs);

    const bool eqOnly = apvts.getRawParameterValue(ParameterIDs::headphoneEqOnly)->load() > 0.5f;
    spatialMix.reset(sampleRate, kSpatialFadeSeconds);
    spatialMix.setCurrentAndTargetValue(eqOnly ? 0.0f : 1.0f);

    wasBypassed = false;
}

void FieldroomAudioProcessor::releaseResources() {
    binauralRenderer.reset();
    roomEngine.reset();
    headphoneCalibration.reset();
    monitoringUtilities.reset();
    outputLimiter.reset();
}

void FieldroomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
        buffer.clear(ch, 0, numSamples);
    }
    if (numSamples <= 0 || buffer.getNumChannels() < 2) {
        return;
    }

    auto read = [this](const juce::String& id) { return apvts.getRawParameterValue(id)->load(); };

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // Master bypass: the audio is not touched at all, only the meters follow the input
    if (read(ParameterIDs::masterBypass) > 0.5f) {
        monitoringUtilities.measure(left, right, numSamples);
        wasBypassed = true;
        return;
    }

    // Coming out of bypass: clear stale delay and reverb state so no old tail resurfaces
    if (wasBypassed) {
        binauralRenderer.reset();
        roomEngine.reset();
        headphoneCalibration.reset();
        outputLimiter.reset();
        wasBypassed = false;
    }

    // Parameters are read once per block; everything below uses these local copies
    const bool eqOnly = read(ParameterIDs::headphoneEqOnly) > 0.5f;
    spatialMix.setTargetValue(eqOnly ? 0.0f : 1.0f);

    binauralRenderer.setSpeakerField(static_cast<int>(read(ParameterIDs::speakerField)));
    binauralRenderer.setSpeakerSpread(read(ParameterIDs::speakerSpread));
    binauralRenderer.setHeadRotation(read(ParameterIDs::headRotation));
    binauralRenderer.setLoudnessMatch(read(ParameterIDs::loudnessMatch) > 0.5f);

    roomEngine.setRoomProfile(static_cast<int>(read(ParameterIDs::roomProfile)));
    roomEngine.setRoomAmount(read(ParameterIDs::roomAmount));
    roomEngine.setSpeakerField(static_cast<int>(read(ParameterIDs::speakerField)));

    headphoneCalibration.requestState(static_cast<int>(read(ParameterIDs::headphoneModel)),
                                      read(ParameterIDs::calibrationEnabled) > 0.5f,
                                      read(ParameterIDs::calibrationPreampAuto) > 0.5f,
                                      read(ParameterIDs::calibrationPreampGain));

    monitoringUtilities.setMode(static_cast<MonitoringMode>(static_cast<int>(read(ParameterIDs::monitoringMode))));
    monitoringUtilities.setSwap(read(ParameterIDs::swapLREnabled) > 0.5f);
    monitoringUtilities.setDim(read(ParameterIDs::dimEnabled) > 0.5f);
    monitoringUtilities.setOutputTrim(read(ParameterIDs::outputTrim));

    // Long blocks are processed in preallocated chunks, so any host block size is safe
    for (int start = 0; start < numSamples; start += maxBlockSize) {
        const int count = std::min(maxBlockSize, numSamples - start);
        processChunk(left + start, right + start, count);
    }
}

void FieldroomAudioProcessor::processChunk(float* left, float* right, int numSamples) noexcept {
    // Keep the dry input: the EQ-only path uses it, and the spatial stages read from it
    dryBuffer.copyFrom(0, 0, left, numSamples);
    dryBuffer.copyFrom(1, 0, right, numSamples);
    const float* dryL = dryBuffer.getReadPointer(0);
    const float* dryR = dryBuffer.getReadPointer(1);

    // 1. Spatial stages. They always run, so their state stays continuous and EQ-only can fade without a click
    binauralRenderer.process(dryL, dryR, left, right, numSamples);
    roomEngine.process(left, right, left, right, numSamples);

    // 2. Blend the spatial (wet) result with the dry path
    if (spatialMix.isSmoothing() || spatialMix.getTargetValue() < 1.0f) {
        for (int n = 0; n < numSamples; ++n) {
            const float mix = spatialMix.getNextValue();
            left[n] = dryL[n] + (left[n] - dryL[n]) * mix;
            right[n] = dryR[n] + (right[n] - dryR[n]) * mix;
        }
    }

    // 3. Headphone correction (lock-free, applies requests at idle moments)
    headphoneCalibration.process(left, right, numSamples);

    // 4. Routing, swap, DIM, trim and metering
    monitoringUtilities.process(left, right, numSamples);

    // 5. Output limiter: catches Loudness Match and trim overs before they reach the DAW.
    //    Meters are measured before this stage, so the clip LED still shows what would have clipped.
    float* channels[2] = { left, right };
    juce::dsp::AudioBlock<float> block(channels, 2, static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> context(block);
    outputLimiter.process(context);
}

void FieldroomAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FieldroomAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessorEditor* FieldroomAudioProcessor::createEditor() {
    return new FieldroomAudioProcessorEditor(*this);
}

} // namespace fieldroom

// Plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new fieldroom::FieldroomAudioProcessor();
}
