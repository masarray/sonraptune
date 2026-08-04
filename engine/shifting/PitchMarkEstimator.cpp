#include "engine/shifting/PitchMarkEstimator.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {

void PitchMarkEstimator::prepare(double sampleRate)
{
    sampleRate_ = std::max(8000.0, sampleRate);

    minPeriodSamples_ = std::max(
        12, static_cast<int>(std::floor(sampleRate_ / 1000.0)));
    maxPeriodSamples_ = std::max(
        minPeriodSamples_ + 1,
        static_cast<int>(std::ceil(sampleRate_ / 55.0)));

    historySize_ = 1;
    while (historySize_ < 4 * maxPeriodSamples_ + 1024)
        historySize_ <<= 1;
    historyMask_ = historySize_ - 1;
    history_.assign(static_cast<std::size_t>(historySize_), 0.0f);

    reset();
}

void PitchMarkEstimator::reset() noexcept
{
    confidence_ = 0.0f;
    voicing_ = 0.0f;
    smoothedPeriodSamples_ = 0.0f;
    previousSample_ = 0.0f;
    currentSample_ = -1;
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

    // Detector motion is trusted, but limited enough that one noisy frame
    // cannot move the waveform phase reference by a large fraction of a cycle.
    if (smoothedPeriodSamples_ <= 0.0f) {
        smoothedPeriodSamples_ = period;
    } else {
        const float maximumMove = std::max(1.0f, 0.04f * smoothedPeriodSamples_);
        const float constrained = std::clamp(
            period,
            smoothedPeriodSamples_ - maximumMove,
            smoothedPeriodSamples_ + maximumMove);
        smoothedPeriodSamples_ = 0.92f * smoothedPeriodSamples_
            + 0.08f * constrained;
    }
}

float PitchMarkEstimator::readHistory(
    std::int64_t absoluteSample) const noexcept
{
    if (absoluteSample < 0
        || currentSample_ < 0
        || absoluteSample > currentSample_
        || currentSample_ - absoluteSample >= historySize_) {
        return 0.0f;
    }

    return history_[static_cast<std::size_t>(absoluteSample & historyMask_)];
}

float PitchMarkEstimator::correlation(
    std::int64_t referenceMark,
    std::int64_t candidateMark,
    int halfWindow) const noexcept
{
    double dot = 0.0;
    double referenceEnergy = 0.0;
    double candidateEnergy = 0.0;

    for (int offset = -halfWindow; offset <= halfWindow; ++offset) {
        const float reference = readHistory(referenceMark + offset);
        const float candidate = readHistory(candidateMark + offset);
        dot += static_cast<double>(reference) * candidate;
        referenceEnergy += static_cast<double>(reference) * reference;
        candidateEnergy += static_cast<double>(candidate) * candidate;
    }

    const double denominator = std::sqrt(referenceEnergy * candidateEnergy);
    if (denominator <= 1.0e-12)
        return -1.0f;

    return static_cast<float>(dot / denominator);
}

std::int64_t PitchMarkEstimator::findCorrelatedMark(float period) const noexcept
{
    const auto predicted = static_cast<std::int64_t>(
        std::llround(static_cast<double>(lastPitchMark_) + period));
    const int searchRadius = std::clamp(
        static_cast<int>(std::lround(0.14f * period)), 3, 96);
    const int halfWindow = std::clamp(
        static_cast<int>(std::lround(0.24f * period)), 8, 128);

    std::int64_t bestMark = predicted;
    float bestScore = -2.0f;

    for (int offset = -searchRadius; offset <= searchRadius; ++offset) {
        const auto candidate = predicted + offset;
        const float similarity = correlation(
            lastPitchMark_, candidate, halfWindow);
        const float distancePenalty = 0.08f
            * std::abs(static_cast<float>(offset))
            / static_cast<float>(searchRadius);
        const float score = similarity - distancePenalty;
        if (score > bestScore) {
            bestScore = score;
            bestMark = candidate;
        }
    }

    // A weak correlation means the waveform changed rapidly. Keep the phase
    // clock near its detector prediction rather than jumping to a harmonic peak.
    if (bestScore < 0.20f)
        return predicted;

    return bestMark;
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

        if (observed >= 0.72f * predicted
            && observed <= 1.28f * predicted) {
            smoothedPeriodSamples_ = 0.88f * predicted + 0.12f * observed;
        }
    }

    lastPitchMark_ = mark;
    markOut = mark;
    return true;
}

bool PitchMarkEstimator::pushSample(float sample,
                                    std::int64_t absoluteSample,
                                    std::int64_t& markOut) noexcept
{
    markOut = -1;
    currentSample_ = absoluteSample;
    history_[static_cast<std::size_t>(absoluteSample & historyMask_)] = sample;

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

    const float period = std::clamp(
        smoothedPeriodSamples_,
        static_cast<float>(minPeriodSamples_),
        static_cast<float>(maxPeriodSamples_));
    const auto predicted = static_cast<std::int64_t>(
        std::llround(static_cast<double>(lastPitchMark_) + period));
    const int searchRadius = std::clamp(
        static_cast<int>(std::lround(0.14f * period)), 3, 96);
    const int halfWindow = std::clamp(
        static_cast<int>(std::lround(0.24f * period)), 8, 128);

    previousSample_ = sample;

    // Wait until the complete candidate neighbourhood is available. The delay
    // remains below the two-period latency budget already reported to the host.
    if (absoluteSample < predicted + searchRadius + halfWindow)
        return false;

    return commit(findCorrelatedMark(period), markOut);
}

} // namespace sonraptune
