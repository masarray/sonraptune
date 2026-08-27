#pragma once

#include "engine/CommonTypes.h"

#include <array>
#include <cstdint>

namespace sonraptune {

struct SongKeyEstimate {
    int key = 0;
    ScaleType scale = ScaleType::naturalMinor;
    float confidence = 0.0f;
    float evidence = 0.0f;
    bool ready = false;
};

// Causal, allocation-free key/major-minor estimator for the incoming vocal
// melody. It intentionally does not claim backing-track chord detection: the
// estimator only sees the monophonic pitch track produced by SonRapTune.
class SongKeyEstimator {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    SongKeyEstimate update(const TrackedPitch& tracked,
                           std::int64_t sampleTime) noexcept;

    const SongKeyEstimate& estimate() const noexcept { return estimate_; }

private:
    struct Candidate {
        int key = 0;
        ScaleType scale = ScaleType::naturalMinor;
        float score = -2.0f;
    };

    Candidate scoreBest(float& secondBest) const noexcept;
    float scoreCandidate(int key, ScaleType scale) const noexcept;

    double sampleRate_ = 48000.0;
    std::int64_t lastSampleTime_ = -1;
    std::array<float, 12> histogram_{};
    float evidence_ = 0.0f;
    SongKeyEstimate estimate_{};
    int challengerKey_ = -1;
    ScaleType challengerScale_ = ScaleType::naturalMinor;
    int challengerFrames_ = 0;
};

} // namespace sonraptune
