#pragma once

#include "engine/CommonTypes.h"

namespace sonraptune {

class CorrectionTrajectory {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setFrame(const TrackedPitch& tracked, float targetMidi,
                  const RuntimeParameters& p) noexcept;
    void render(float* ratioPerSample, float* voicedMaskPerSample,
                int numSamples, const RuntimeParameters& p) noexcept;
    float correctionCents() const noexcept { return currentCents_; }

private:
    double sampleRate_ = 48000.0;
    float currentCents_ = 0.0f;
    float targetCents_ = 0.0f;
    float targetMask_ = 0.0f;
    float currentMask_ = 0.0f;
};

} // namespace sonraptune
