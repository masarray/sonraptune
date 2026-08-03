#include "engine/correction/CorrectionTrajectory.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {

void CorrectionTrajectory::prepare(double sampleRate) noexcept
{
    sampleRate_ = std::max(8000.0, sampleRate);
    reset();
}

void CorrectionTrajectory::reset() noexcept
{
    currentCents_ = 0.0f;
    targetCents_ = 0.0f;
    targetMask_ = 0.0f;
    currentMask_ = 0.0f;
}

void CorrectionTrajectory::setFrame(const TrackedPitch& tracked, float targetMidi,
                                    const RuntimeParameters& p) noexcept
{
    if (tracked.midi < 0.0f || targetMidi < 0.0f || tracked.voicing < 0.15f) {
        targetCents_ = 0.0f;
        targetMask_ = 0.0f;
        return;
    }

    const float rawError = (targetMidi - tracked.midi) * 100.0f;
    const float feel = std::clamp(p.feel, 0.0f, 1.0f);
    const float deadZone = 2.0f + 28.0f * feel;
    float effective = rawError;
    if (std::abs(effective) <= deadZone) effective = 0.0f;
    else effective -= std::copysign(deadZone, effective);

    targetCents_ = effective * std::clamp(p.tune, 0.0f, 1.0f);
    targetMask_ = std::clamp(tracked.voicing * tracked.confidence, 0.0f, 1.0f);
}

void CorrectionTrajectory::render(float* ratioPerSample, float* voicedMaskPerSample,
                                  int numSamples, const RuntimeParameters& p) noexcept
{
    if (ratioPerSample == nullptr || voicedMaskPerSample == nullptr || numSamples <= 0) return;
    const float speedSeconds = std::max(0.0005f, p.speedMs * 0.001f);
    const float alpha = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * speedSeconds));
    const float releaseAlpha = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate_ * 0.012));
    float mask = currentMask_;
    for (int i = 0; i < numSamples; ++i) {
        currentCents_ += alpha * (targetCents_ - currentCents_);
        mask += (targetMask_ > mask ? alpha : releaseAlpha) * (targetMask_ - mask);
        ratioPerSample[i] = std::exp2(currentCents_ / 1200.0f);
        voicedMaskPerSample[i] = std::clamp(mask, 0.0f, 1.0f);
    }
    currentMask_ = mask;
}

} // namespace sonraptune
