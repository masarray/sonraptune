#pragma once

#include <cstdint>
#include <vector>

namespace sonraptune {

// Causal waveform pitch-mark estimator for voiced material.
//
// Detector pitch predicts the next period. Once a small future search window
// is available, the estimator compares waveform neighbourhoods around the
// previous mark and each candidate near the prediction. The highest
// normalised-correlation candidate is selected. This prevents the estimator
// from jumping between different harmonic peaks of a complex vocal waveform.
class PitchMarkEstimator {
public:
    void prepare(double sampleRate);
    void reset() noexcept;

    void setPitch(float hz, float confidence, float voicing) noexcept;

    bool pushSample(float sample,
                    std::int64_t absoluteSample,
                    std::int64_t& markOut) noexcept;

    float periodSamples() const noexcept { return smoothedPeriodSamples_; }
    int minPeriodSamples() const noexcept { return minPeriodSamples_; }
    int maxPeriodSamples() const noexcept { return maxPeriodSamples_; }

private:
    bool commit(std::int64_t mark, std::int64_t& markOut) noexcept;
    float readHistory(std::int64_t absoluteSample) const noexcept;
    float correlation(std::int64_t referenceMark,
                      std::int64_t candidateMark,
                      int halfWindow) const noexcept;
    std::int64_t findCorrelatedMark(float period) const noexcept;

    double sampleRate_ = 48000.0;
    int minPeriodSamples_ = 48;
    int maxPeriodSamples_ = 873;
    int historySize_ = 0;
    int historyMask_ = 0;

    float confidence_ = 0.0f;
    float voicing_ = 0.0f;
    float smoothedPeriodSamples_ = 0.0f;
    float previousSample_ = 0.0f;

    std::int64_t currentSample_ = -1;
    std::int64_t lastPitchMark_ = -1;
    std::vector<float> history_{};
};

} // namespace sonraptune
