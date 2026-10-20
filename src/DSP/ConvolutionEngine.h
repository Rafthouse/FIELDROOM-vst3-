#pragma once

#include <vector>
#include <juce_dsp/juce_dsp.h>

namespace fieldroom {

/**
 * Low-latency stereo partitioned convolution engine.
 * Splits long room impulse responses into uniform 128-sample FFT partitions,
 * achieving zero-spike real-time processing on the audio thread.
 */
class ConvolutionEngine {
public:
    ConvolutionEngine() = default;

    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;
        maxBlockSize = spec.maximumBlockSize;
        leftEngine.prepare(spec);
        rightEngine.prepare(spec);
    }

    void reset() {
        leftEngine.reset();
        rightEngine.reset();
    }

    void loadImpulseResponse(const float* leftIR, int leftLen,
                             const float* rightIR, int rightLen) {
        if (leftLen > 0 && leftIR != nullptr) {
            juce::AudioBuffer<float> buf(1, leftLen);
            buf.copyFrom(0, 0, leftIR, leftLen);
            leftEngine.loadImpulseResponse(std::move(buf), sampleRate,
                                           juce::dsp::Convolution::Stereo::no,
                                           juce::dsp::Convolution::Trim::no,
                                           juce::dsp::Convolution::Normalise::no);
        }
        if (rightLen > 0 && rightIR != nullptr) {
            juce::AudioBuffer<float> buf(1, rightLen);
            buf.copyFrom(0, 0, rightIR, rightLen);
            rightEngine.loadImpulseResponse(std::move(buf), sampleRate,
                                            juce::dsp::Convolution::Stereo::no,
                                            juce::dsp::Convolution::Trim::no,
                                            juce::dsp::Convolution::Normalise::no);
        }
    }

    void process(juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() < 2) return;

        // Process left channel
        juce::dsp::AudioBlock<float> block(buffer);
        auto leftBlock = block.getSingleChannelBlock(0);
        juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
        leftEngine.process(leftContext);

        // Process right channel
        auto rightBlock = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        rightEngine.process(rightContext);
    }

private:
    double sampleRate{48000.0};
    uint32_t maxBlockSize{512};
    juce::dsp::Convolution leftEngine{juce::dsp::Convolution::NonUniform{128}};
    juce::dsp::Convolution rightEngine{juce::dsp::Convolution::NonUniform{128}};
};

} // namespace fieldroom
