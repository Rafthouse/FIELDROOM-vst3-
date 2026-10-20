#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace fieldroom {

FieldroomAudioProcessor::FieldroomAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "FieldroomParameters", ParameterManager::createParameterLayout()) {
    apvts.addParameterListener(ParameterIDs::headphoneModel, this);
    apvts.addParameterListener(ParameterIDs::calibrationEnabled, this);
    apvts.addParameterListener(ParameterIDs::calibrationPreampAuto, this);
    apvts.addParameterListener(ParameterIDs::calibrationPreampGain, this);
}

FieldroomAudioProcessor::~FieldroomAudioProcessor() {
    apvts.removeParameterListener(ParameterIDs::headphoneModel, this);
    apvts.removeParameterListener(ParameterIDs::calibrationEnabled, this);
    apvts.removeParameterListener(ParameterIDs::calibrationPreampAuto, this);
    apvts.removeParameterListener(ParameterIDs::calibrationPreampGain, this);
}

bool FieldroomAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void FieldroomAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    juce::ignoreUnused(parameterID, newValue);
    updateCalibrationStateFromAPVTS();
}

void FieldroomAudioProcessor::updateCalibrationStateFromAPVTS() {
    if (auto* modelParam = apvts.getRawParameterValue(ParameterIDs::headphoneModel)) {
        const int modelIdx = static_cast<int>(modelParam->load());
        const bool enabled = apvts.getRawParameterValue(ParameterIDs::calibrationEnabled)->load() > 0.5f;
        const bool autoPreamp = apvts.getRawParameterValue(ParameterIDs::calibrationPreampAuto)->load() > 0.5f;
        const float manualGain = apvts.getRawParameterValue(ParameterIDs::calibrationPreampGain)->load();

        headphoneCalibration.prepareAndPublishState(modelIdx, enabled, autoPreamp, manualGain);
    }
}

void FieldroomAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    binauralRenderer.prepare(sampleRate, samplesPerBlock);
    roomEngine.prepare(sampleRate, samplesPerBlock);
    headphoneCalibration.prepare(sampleRate, samplesPerBlock);
    monitoringUtilities.prepare(sampleRate, samplesPerBlock);

    updateCalibrationStateFromAPVTS();
    roomScratchBuffer.setSize(2, samplesPerBlock);
}

void FieldroomAudioProcessor::releaseResources() {
    binauralRenderer.reset();
    roomEngine.reset();
    headphoneCalibration.reset();
    monitoringUtilities.reset();
}

void FieldroomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples <= 0 || totalNumInputChannels < 2)
        return;

    // Master Bypass Check
    const bool masterBypass = apvts.getRawParameterValue(ParameterIDs::masterBypass)->load() > 0.5f;
    if (masterBypass) {
        monitoringUtilities.process(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
        currentMeters = monitoringUtilities.getMeterLevels();
        return;
    }

    // Retrieve active parameters
    const int speakerFieldIdx = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::speakerField)->load());
    const int roomProfileIdx = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::roomProfile)->load());
    const float roomAmount = apvts.getRawParameterValue(ParameterIDs::roomAmount)->load();
    const float headRotation = apvts.getRawParameterValue(ParameterIDs::headRotation)->load();
    const bool loudnessMatch = apvts.getRawParameterValue(ParameterIDs::loudnessMatch)->load() > 0.5f;

    const int monitoringModeIdx = static_cast<int>(apvts.getRawParameterValue(ParameterIDs::monitoringMode)->load());
    const bool swapLREnabled = apvts.getRawParameterValue(ParameterIDs::swapLREnabled)->load() > 0.5f;
    const bool dimActive = apvts.getRawParameterValue(ParameterIDs::dimEnabled)->load() > 0.5f;
    const bool eqOnly = apvts.getRawParameterValue(ParameterIDs::headphoneEqOnly)->load() > 0.5f;
    const float outputTrim = apvts.getRawParameterValue(ParameterIDs::outputTrim)->load();

    float* channelL = buffer.getWritePointer(0);
    float* channelR = buffer.getWritePointer(1);

    // If "Headphone EQ Only" is engaged, skip room & binaural processing
    if (!eqOnly) {
        // 1. Binaural Virtual Loudspeaker Rendering (Distance, 2x2 Matrix, ITD/ILD, Head Rotation)
        binauralRenderer.setSpeakerField(speakerFieldIdx);
        binauralRenderer.setHeadRotation(headRotation);
        binauralRenderer.setLoudnessMatch(loudnessMatch);
        binauralRenderer.process(channelL, channelR, channelL, channelR, numSamples);

        // 2. Room Acoustics (Early Reflections + Feedback Delay Network)
        roomEngine.setRoomProfile(roomProfileIdx);
        roomEngine.setRoomAmount(roomAmount);
        roomEngine.setSpeakerField(speakerFieldIdx);
        roomEngine.process(channelL, channelR, channelL, channelR, numSamples);
    }

    // 3. Headphone Calibration Engine (Real-time safe, wait-free, zero-allocation processBlock)
    headphoneCalibration.process(channelL, channelR, numSamples);

    // 4. Monitoring Utilities (Mono, Side, L/R Solo, Swap, DIM -18dB, Output Trim, Metering)
    monitoringUtilities.setMode(static_cast<MonitoringMode>(monitoringModeIdx));
    monitoringUtilities.setSwap(swapLREnabled);
    monitoringUtilities.setDim(dimActive);
    monitoringUtilities.setOutputTrim(outputTrim);
    monitoringUtilities.process(channelL, channelR, numSamples);

    // Store meters
    currentMeters = monitoringUtilities.getMeterLevels();
}

void FieldroomAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FieldroomAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
            // Deterministic state preparation after DAW project or preset restoration
            updateCalibrationStateFromAPVTS();
        }
    }
}

juce::AudioProcessorEditor* FieldroomAudioProcessor::createEditor() {
    return new FieldroomAudioProcessorEditor(*this);
}

} // namespace fieldroom

// Create JUCE plugin instance export
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new fieldroom::FieldroomAudioProcessor();
}
