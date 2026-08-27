#include "engine/correction/CorrectionTrajectory.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {
namespace {

struct StyleCurve {
    float tuneScale;
    float speedScale;
    float deadZoneScale;
    float attackSeconds;
    float releaseSeconds;
};

StyleCurve styleCurve(ProductMode mode) noexcept
{
    switch (mode) {
        case ProductMode::natural:
            return {0.82f, 1.45f, 1.35f, 0.012f, 0.009f};
        case ProductMode::modernRap:
            return {1.00f, 0.82f, 0.82f, 0.006f, 0.006f};
        case ProductMode::trapLock:
            return {1.00f, 0.38f, 0.35f, 0.003f, 0.006f};
        case ProductMode::hookDebug:
            return {0.92f, 1.05f, 0.95f, 0.007f, 0.010f};
    }
    return {1.0f, 1.0f, 1.0f, 0.008f, 0.006f};
}

} // namespace

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

void CorrectionTrajectory::setFrame(const TrackedPitch& tracked,
                                    float targetMidi,
                                    const RuntimeParameters& p) noexcept
{
    const bool stableVoiced =
        tracked.state == PitchState::voicedStable
        && tracked.midi >= 0.0f
        && targetMidi >= 0.0f
        && tracked.confidence >= 0.55f
        && tracked.voicing >= 0.55f;

    // Confidence is a decision signal, not a dry/wet percentage. Continuously
    // mixing dry and shifted audio at different pitch/phase creates beating,
    // comb filtering, and the audible "off-station radio" effect.
    targetMask_ = stableVoiced ? 1.0f : 0.0f;

    if (!stableVoiced) {
        // During an unvoiced/phrase-release boundary, let the wet envelope
        // decay while holding the last correction ratio. Driving the ratio
        // back toward unity while the wet path is still audible creates a
        // small end-note pitch scoop. Silence/onset starts a fresh phrase and
        // therefore resets the target correction normally.
        if (tracked.state == PitchState::unvoiced
            || tracked.state == PitchState::phraseRelease) {
            return;
        }
        targetCents_ = 0.0f;
        return;
    }

    const auto curve = styleCurve(p.mode);
    const float rawError = (targetMidi - tracked.midi) * 100.0f;
    const float feel = std::clamp(p.feel, 0.0f, 1.0f);
    const float deadZone = (2.0f + 28.0f * feel) * curve.deadZoneScale;
    float effective = rawError;
    if (std::abs(effective) <= deadZone)
        effective = 0.0f;
    else
        effective -= std::copysign(deadZone, effective);

    const float effectiveTune = std::clamp(
        p.tune * curve.tuneScale, 0.0f, 1.0f);
    targetCents_ = effective * effectiveTune;
}

void CorrectionTrajectory::render(float* ratioPerSample,
                                  float* voicedMaskPerSample,
                                  int numSamples,
                                  const RuntimeParameters& p) noexcept
{
    if (ratioPerSample == nullptr
        || voicedMaskPerSample == nullptr
        || numSamples <= 0) {
        return;
    }

    const auto curve = styleCurve(p.mode);
    const float speedSeconds = std::max(
        0.0005f, p.speedMs * 0.001f * curve.speedScale);
    const float pitchAlpha = 1.0f - std::exp(
        -1.0f / static_cast<float>(sampleRate_ * speedSeconds));

    const float maskAttackAlpha = 1.0f - std::exp(
        -1.0f / static_cast<float>(sampleRate_ * curve.attackSeconds));
    const float maskReleaseAlpha = 1.0f - std::exp(
        -1.0f / static_cast<float>(sampleRate_ * curve.releaseSeconds));

    float mask = currentMask_;
    for (int i = 0; i < numSamples; ++i) {
        currentCents_ += pitchAlpha * (targetCents_ - currentCents_);
        const float maskAlpha = targetMask_ > mask
            ? maskAttackAlpha
            : maskReleaseAlpha;
        mask += maskAlpha * (targetMask_ - mask);

        ratioPerSample[i] = std::exp2(currentCents_ / 1200.0f);
        voicedMaskPerSample[i] = std::clamp(mask, 0.0f, 1.0f);
    }
    currentMask_ = mask;
}

} // namespace sonraptune
