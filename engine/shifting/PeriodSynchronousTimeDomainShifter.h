#pragma once

#include "engine/shifting/IPitchShifter.h"
#include "engine/shifting/PitchMarkEstimator.h"

#include <array>
#include <cstdint>
#include <vector>

namespace sonraptune {

// E3 candidate B: causal period-synchronous overlap-add pitch shifter.
//
// Detector-guided waveform pitch marks drive two-period Hann grains. Synthesis
// marks are spaced by sourcePeriod / correctionRatio, while source grains are
// selected nearest to the synthesis timeline. The output and dry path share a
// fixed reported latency.
//
// This is a materially more period-aware candidate than Candidate A, but it is
// still an engineering alpha: formant preservation, consonant/onset
// reintegration, and recorded-vocal listening validation remain separate gates.
class PeriodSynchronousTimeDomainShifter final : public IPitchShifter {
public:
    void prepare(double sampleRate, int maxBlock, int channels) override;
    void reset() noexcept override;
    int latencySamples() const noexcept override { return latencySamples_; }

    void setSourcePitch(float hz, float confidence, float voicing) noexcept;

    void process(float* const* channels,
                 int numChannels,
                 int numSamples,
                 const float* ratioPerSample,
                 const float* voicedMaskPerSample) noexcept override;

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
                       int radius) noexcept;

    double sampleRate_ = 48000.0;
    int preparedChannels_ = 1;
    int ringSize_ = 0;
    int ringMask_ = 0;
    int maxRadiusSamples_ = 0;
    int latencySamples_ = 0;

    std::int64_t absoluteSample_ = 0;

    std::array<std::vector<float>, 2> inputRing_{};
    std::array<std::vector<float>, 2> outputSum_{};
    std::vector<float> outputWeight_{};

    PitchMarkEstimator markEstimator_{};
    bool sourceReliable_ = false;

    std::array<std::int64_t, kMaxMarks> pitchMarks_{};
    int pitchMarkWrite_ = 0;
    int pitchMarkCount_ = 0;
    double nextSynthesisTime_ = -1.0;
};

} // namespace sonraptune
