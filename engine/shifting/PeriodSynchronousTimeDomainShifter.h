#pragma once

#include "engine/shifting/IPitchShifter.h"
#include "engine/shifting/PitchMarkEstimator.h"

#include <array>
#include <cstdint>
#include <vector>

namespace sonraptune {

// E3 candidate B: causal period-synchronous overlap-add pitch shifter.
//
// This revision adds P0 containment for real-vocal crackle: scheduled grains
// are generation tagged, writes to already-consumed samples are rejected,
// latency covers the complete two-period grain, and OLA coverage is faded
// rather than switched abruptly.
class PeriodSynchronousTimeDomainShifter final : public IPitchShifter {
public:
    struct Diagnostics {
        std::uint64_t rejectedPastWrites = 0;
        std::uint64_t coverageFallbackSamples = 0;
        std::uint64_t reliabilityTransitions = 0;
    };

    void prepare(double sampleRate, int maxBlock, int channels) override;
    void reset() noexcept override;
    int latencySamples() const noexcept override { return latencySamples_; }

    void setSourcePitch(float hz, float confidence, float voicing) noexcept;

    void process(float* const* channels,
                 int numChannels,
                 int numSamples,
                 const float* ratioPerSample,
                 const float* voicedMaskPerSample) noexcept override;

    const Diagnostics& diagnostics() const noexcept { return diagnostics_; }

private:
    static constexpr int kMaxMarks = 128;

    float readInput(int channel, std::int64_t absoluteSample) const noexcept;
    void writeInput(int channel,
                    std::int64_t absoluteSample,
                    float value) noexcept;

    void clearVoicedState() noexcept;
    void commitPitchMark(std::int64_t mark) noexcept;
    std::int64_t nearestReadyPitchMark(double sourceTime,
                                       std::int64_t latestReadyMark) const noexcept;

    void scheduleAvailableGrains(std::int64_t currentSample,
                                 float ratio) noexcept;
    void scheduleGrain(std::int64_t sourceMark,
                       std::int64_t synthesisMark,
                       int radius,
                       std::int64_t earliestOutputSample) noexcept;

    double sampleRate_ = 48000.0;
    int preparedChannels_ = 1;
    int ringSize_ = 0;
    int ringMask_ = 0;
    int maxRadiusSamples_ = 0;
    int latencySamples_ = 0;

    float ratioSmoothingAlpha_ = 1.0f;
    float coverageAttackAlpha_ = 1.0f;
    float coverageReleaseAlpha_ = 1.0f;
    float unityMixAlpha_ = 1.0f;
    float smoothedRatio_ = 1.0f;
    float coverageMix_ = 0.0f;
    float unityDryMix_ = 1.0f;

    std::int64_t absoluteSample_ = 0;

    std::array<std::vector<float>, 2> inputRing_{};
    std::array<std::vector<float>, 2> outputSum_{};
    std::vector<float> outputWeight_{};
    std::vector<std::uint32_t> outputGeneration_{};
    std::uint32_t generation_ = 1;

    PitchMarkEstimator markEstimator_{};
    bool sourceReliable_ = false;

    std::array<std::int64_t, kMaxMarks> pitchMarks_{};
    int pitchMarkWrite_ = 0;
    int pitchMarkCount_ = 0;
    double nextSynthesisTime_ = -1.0;

    Diagnostics diagnostics_{};
};

} // namespace sonraptune
