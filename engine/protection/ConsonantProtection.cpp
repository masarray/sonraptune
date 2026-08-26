#include "engine/protection/ConsonantProtection.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {
namespace {

float onePoleAlpha(double sampleRate, double seconds) noexcept
{
    return 1.0f - std::exp(-1.0f
        / static_cast<float>(std::max(1.0, sampleRate * seconds)));
}

float smoothStep01(float x) noexcept
{
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

} // namespace

void ConsonantProtection::prepare(double sampleRate) noexcept
{
    sampleRate_ = std::max(8000.0, sampleRate);
    reset();
}

void ConsonantProtection::reset() noexcept
{
    previousSample_ = 0.0f;
    fullEnvelope_ = 0.0f;
    highEnvelope_ = 0.0f;
    zeroCrossEnvelope_ = 0.0f;
    currentWetMultiplier_ = 1.0f;
}

void ConsonantProtection::process(const float* mono,
                                  float* wetMask,
                                  int numSamples,
                                  float amount,
                                  const PitchFrame& frame) noexcept
{
    if (mono == nullptr || wetMask == nullptr || numSamples <= 0)
        return;

    amount = std::clamp(amount, 0.0f, 1.0f);
    if (amount <= 0.0f) {
        currentWetMultiplier_ = 1.0f;
        return;
    }

    const float envelopeAlpha = onePoleAlpha(sampleRate_, 0.0035);
    const float zeroCrossAlpha = onePoleAlpha(sampleRate_, 0.0050);
    const float protectAttack = onePoleAlpha(sampleRate_, 0.0015);
    const float protectRelease = onePoleAlpha(sampleRate_, 0.0120);

    const float ambiguity = 1.0f - std::clamp(frame.voicing, 0.0f, 1.0f);
    const float stateProtection =
        frame.state == PitchState::onset ? 1.0f
        : frame.state == PitchState::unvoiced ? 1.0f
        : frame.state == PitchState::phraseRelease ? 0.85f
        : frame.state == PitchState::voicedUnstable ? 0.35f
        : 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        const float x = mono[i];
        const float absX = std::abs(x);
        const float high = std::abs(x - previousSample_);
        const bool crossed = (x >= 0.0f) != (previousSample_ >= 0.0f);
        previousSample_ = x;

        fullEnvelope_ += envelopeAlpha * (absX - fullEnvelope_);
        highEnvelope_ += envelopeAlpha * (high - highEnvelope_);
        zeroCrossEnvelope_ += zeroCrossAlpha
            * ((crossed ? 1.0f : 0.0f) - zeroCrossEnvelope_);

        const float highRatio = highEnvelope_ / (fullEnvelope_ + 1.0e-4f);
        const float spectralNoise = smoothStep01((highRatio - 0.55f) / 1.15f);
        const float crossingNoise = smoothStep01((zeroCrossEnvelope_ - 0.055f) / 0.20f);
        const float noiseLike = std::max(spectralNoise, crossingNoise);

        // Strong protection is reserved for ambiguous/noisy material so high
        // sung notes are not mistaken for sibilance purely because their
        // waveform changes quickly.
        const float noisyProtection = noiseLike * (0.25f + 0.75f * ambiguity);
        const float protection = std::max(stateProtection, noisyProtection);
        const float targetWetMultiplier = 1.0f - amount * protection;
        const float alpha = targetWetMultiplier < currentWetMultiplier_
            ? protectAttack
            : protectRelease;
        currentWetMultiplier_ += alpha
            * (targetWetMultiplier - currentWetMultiplier_);
        currentWetMultiplier_ = std::clamp(currentWetMultiplier_, 0.0f, 1.0f);

        wetMask[i] *= currentWetMultiplier_;
    }
}

} // namespace sonraptune
