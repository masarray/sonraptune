#pragma once

#include "engine/shifting/IPitchShifter.h"

#include <array>
#include <vector>

namespace sonraptune {

// E3 candidate A: causal dual-read-head granular shifter.
//
// This is deliberately labelled a bake-off candidate, not the final
// TD-PSOLA product engine. It provides audible, fixed-latency tuning so the
// detector/mapper/trajectory can be tested end-to-end while the true
// period-synchronous candidate is developed and compared.
class DualHeadTimeDomainShifter final : public IPitchShifter {
public:
    void prepare(double sampleRate, int maxBlock, int channels) override;
    void reset() noexcept override;
    int latencySamples() const noexcept override { return latencySamples_; }

    void process(float* const* channels, int numChannels, int numSamples,
                 const float* ratioPerSample,
                 const float* voicedMaskPerSample) noexcept override;

private:
    float readInterpolated(const std::vector<float>& ring, float position) const noexcept;

    double sampleRate_ = 48000.0;
    int preparedChannels_ = 1;
    int ringSize_ = 0;
    int writePosition_ = 0;
    int grainSamples_ = 0;
    int baseDelaySamples_ = 0;
    int latencySamples_ = 0;
    float phase_ = 0.0f;
    std::array<std::vector<float>, 2> ring_{};
};

} // namespace sonraptune
