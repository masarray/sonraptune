#pragma once

namespace sonraptune {

class IPitchShifter {
public:
    virtual ~IPitchShifter() = default;
    virtual void prepare(double sampleRate, int maxBlock, int channels) = 0;
    virtual void reset() noexcept = 0;
    virtual int latencySamples() const noexcept = 0;
    virtual void process(float* const* channels, int numChannels, int numSamples,
                         const float* ratioPerSample,
                         const float* voicedMaskPerSample) noexcept = 0;
};

} // namespace sonraptune
