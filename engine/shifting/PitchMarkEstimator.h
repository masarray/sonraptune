#pragma once

#include <cstdint>

namespace sonraptune {

// Causal waveform pitch-mark estimator for voiced material.
//
// A trusted detector pitch constrains the expected period. Inside each
// period-sized search region, the estimator selects a positive waveform peak.
// This avoids harmonic zero-crossing ambiguity while remaining inexpensive and
// allocation-free on the audio thread.
class PitchMarkEstimator {
public:
    void prepare(double sampleRate) noexcept;
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

    double sampleRate_ = 48000.0;
    int minPeriodSamples_ = 48;
    int maxPeriodSamples_ = 873;

    float confidence_ = 0.0f;
    float voicing_ = 0.0f;
    float smoothedPeriodSamples_ = 0.0f;
    float previousSample_ = 0.0f;
    float candidatePeakValue_ = 0.0f;

    std::int64_t candidatePeakSample_ = -1;
    std::int64_t lastPitchMark_ = -1;
};

} // namespace sonraptune
