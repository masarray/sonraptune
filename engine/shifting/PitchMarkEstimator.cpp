#include "engine/shifting/PitchMarkEstimator.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {

void PitchMarkEstimator::prepare(double sampleRate) noexcept
{
    sampleRate_ = std::max(8000.0, sampleRate);

    // The detector currently supports the vocal/rap range. Keeping the pitch
    // mark limits wider makes transitions tolerant without allowing
    // pathological period estimates into the grain scheduler.
    minPeriodSamples_ = std::max(
        12, static_cast<int>(std::floor(sampleRate_ / 1000.0)));
    maxPeriodSamples_ = std::max(
        minPeriodSamples_ + 1,
        static_cast<int>(std::ceil(sampleRate_ / 55.0)));

    reset();
}

void PitchMarkEstimator::reset() noexcept
{
    confidence_ = 0.0f;
    voicing_ = 0.0f;
    smoothedPeriodSamples_ = 0.0f;
    previousSample_ = 0.0f;
    candidatePeakValue_ = 0.0f;
    candidatePeakSample_ = -1;
    lastPitchMark_ = -1;
}

void PitchMarkEstimator::setPitch(float hz,
                                  float confidence,
                                  float voicing) noexcept
{
    confidence_ = std::clamp(confidence, 0.0f, 1.0f);
    voicing_ = std::clamp(voicing, 0.0f, 1.0f);

    if (hz <= 0.0f || confidence_ <= 0.1f || voicing_ <= 0.1f)
        return;

    float period = static_cast<float>(sampleRate_)
        / std::clamp(hz, 20.0f, 2000.0f);
    period = std::clamp(period,
                        static_cast<float>(minPeriodSamples_),
                        static_cast<float>(maxPeriodSamples_));

    smoothedPeriodSamples_ = smoothedPeriodSamples_ <= 0.0f
        ? period
        : 0.9f * smoothedPeriodSamples_ + 0.1f * period;
}

bool PitchMarkEstimator::commit(std::int64_t mark,
                                std::int64_t& markOut) noexcept
{
    if (mark < 0 || (lastPitchMark_ >= 0 && mark <= lastPitchMark_))
        return false;

    if (lastPitchMark_ >= 0) {
        const float observed = static_cast<float>(mark - lastPitchMark_);
        const float predicted = std::clamp(
            smoothedPeriodSamples_,
            static_cast<float>(minPeriodSamples_),
            static_cast<float>(maxPeriodSamples_));

        if (observed >= 0.55f * predicted
            && observed <= 1.45f * predicted) {
            smoothedPeriodSamples_ = 0.8f * predicted + 0.2f * observed;
        }
    }

    lastPitchMark_ = mark;
    candidatePeakValue_ = 0.0f;
    candidatePeakSample_ = -1;
    markOut = mark;
    return true;
}

bool PitchMarkEstimator::pushSample(float sample,
                                    std::int64_t absoluteSample,
                                    std::int64_t& markOut) noexcept
{
    markOut = -1;

    if (smoothedPeriodSamples_ <= 0.0f
        || confidence_ < 0.15f
        || voicing_ < 0.15f) {
        previousSample_ = sample;
        return false;
    }

    if (lastPitchMark_ < 0) {
        const bool risingZeroCrossing = previousSample_ <= 0.0f && sample > 0.0f;
        previousSample_ = sample;
        return risingZeroCrossing && commit(absoluteSample, markOut);
    }

    const float elapsed = static_cast<float>(absoluteSample - lastPitchMark_);
    const float period = std::clamp(
        smoothedPeriodSamples_,
        static_cast<float>(minPeriodSamples_),
        static_cast<float>(maxPeriodSamples_));

    if (elapsed >= 0.55f * period && sample > candidatePeakValue_) {
        candidatePeakValue_ = sample;
        candidatePeakSample_ = absoluteSample;
    }

    const bool localPositivePeak =
        elapsed >= 0.78f * period
        && elapsed <= 1.22f * period
        && previousSample_ > 0.0f
        && sample < previousSample_
        && candidatePeakSample_ == absoluteSample - 1;

    // The timeout prevents a weak/asymmetric waveform from stalling the mark
    // stream. It commits the strongest positive peak seen in the constrained
    // period region rather than inventing an unconstrained mark.
    const bool timeout = elapsed >= 1.18f * period;
    previousSample_ = sample;

    if ((localPositivePeak || timeout)
        && candidatePeakSample_ > lastPitchMark_) {
        return commit(candidatePeakSample_, markOut);
    }

    return false;
}

} // namespace sonraptune
