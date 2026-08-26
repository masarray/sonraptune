#pragma once

#include "engine/CommonTypes.h"

namespace sonraptune {

// Lightweight realtime protection for noisy consonants, sibilants and hard
// onsets. The detector remains responsible for voiced/unvoiced classification;
// this stage only reduces the pitch-shift wet path when short-time waveform
// features look noise-like or transient-heavy.
class ConsonantProtection {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void process(const float* mono,
                 float* wetMask,
                 int numSamples,
                 float amount,
                 const PitchFrame& frame) noexcept;

    float currentProtection() const noexcept { return 1.0f - currentWetMultiplier_; }

private:
    double sampleRate_ = 48000.0;
    float previousSample_ = 0.0f;
    float fullEnvelope_ = 0.0f;
    float highEnvelope_ = 0.0f;
    float zeroCrossEnvelope_ = 0.0f;
    float currentWetMultiplier_ = 1.0f;
};

} // namespace sonraptune
