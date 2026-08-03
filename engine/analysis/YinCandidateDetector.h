#pragma once

#include "engine/CommonTypes.h"
#include <array>
#include <vector>

namespace sonraptune {

class YinCandidateDetector {
public:
    static constexpr int kAnalysisSize = 1024;
    static constexpr int kHopSize = 32;
    static constexpr int kMaxTau = 256;

    void prepare(double inputSampleRate);
    void reset() noexcept;

    // Pushes full-rate mono samples. Returns true whenever a new analysis frame is ready.
    bool push(const float* input, int numSamples, VocalRange range, PitchAnalysis& out) noexcept;

    double detectorSampleRate() const noexcept { return detectorRate_; }
    int inputHopSamples() const noexcept { return decimation_ * kHopSize; }

private:
    bool analyze(VocalRange range, PitchAnalysis& out) noexcept;
    void pushDecimated(float sample) noexcept;
    std::pair<float, float> rangeHz(VocalRange range) const noexcept;

    double inputRate_ = 48000.0;
    double detectorRate_ = 12000.0;
    int decimation_ = 4;
    int decimationPhase_ = 0;
    float decimationSum_ = 0.0f;

    std::array<float, kAnalysisSize> history_{};
    std::array<float, kAnalysisSize> frame_{};
    std::array<float, kMaxTau + 2> diff_{};
    std::array<float, kMaxTau + 2> cmndf_{};
    int writeIndex_ = 0;
    int available_ = 0;
    int hopCounter_ = 0;
    std::int64_t inputSampleCounter_ = 0;
    float previousRms_ = 0.0f;
};

} // namespace sonraptune
