#include "engine/intelligence/SongKeyEstimator.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {
namespace {

constexpr std::array<float, 12> kMajorProfile{
    6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
    2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f
};

constexpr std::array<float, 12> kMinorProfile{
    6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f,
    2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f
};

int wrapPitchClass(int value) noexcept
{
    value %= 12;
    return value < 0 ? value + 12 : value;
}

} // namespace

void SongKeyEstimator::prepare(double sampleRate) noexcept
{
    sampleRate_ = std::max(8000.0, sampleRate);
    reset();
}

void SongKeyEstimator::reset() noexcept
{
    histogram_.fill(0.0f);
    evidence_ = 0.0f;
    estimate_ = {};
    lastSampleTime_ = -1;
    challengerKey_ = -1;
    challengerScale_ = ScaleType::naturalMinor;
    challengerFrames_ = 0;
}

float SongKeyEstimator::scoreCandidate(int key, ScaleType scale) const noexcept
{
    const auto& profile = scale == ScaleType::major ? kMajorProfile : kMinorProfile;

    float histogramMean = 0.0f;
    float profileMean = 0.0f;
    for (int i = 0; i < 12; ++i) {
        histogramMean += histogram_[static_cast<std::size_t>(i)];
        profileMean += profile[static_cast<std::size_t>(i)];
    }
    histogramMean /= 12.0f;
    profileMean /= 12.0f;

    float dot = 0.0f;
    float histogramPower = 0.0f;
    float profilePower = 0.0f;
    for (int pitchClass = 0; pitchClass < 12; ++pitchClass) {
        const float h = histogram_[static_cast<std::size_t>(pitchClass)] - histogramMean;
        const int relative = wrapPitchClass(pitchClass - key);
        const float p = profile[static_cast<std::size_t>(relative)] - profileMean;
        dot += h * p;
        histogramPower += h * h;
        profilePower += p * p;
    }

    const float denominator = std::sqrt(histogramPower * profilePower);
    return denominator > 1.0e-9f ? dot / denominator : -1.0f;
}

SongKeyEstimator::Candidate SongKeyEstimator::scoreBest(float& secondBest) const noexcept
{
    Candidate best;
    secondBest = -2.0f;

    for (int key = 0; key < 12; ++key) {
        for (const auto scale : {ScaleType::major, ScaleType::naturalMinor}) {
            const float score = scoreCandidate(key, scale);
            if (score > best.score) {
                secondBest = best.score;
                best = {key, scale, score};
            } else if (score > secondBest) {
                secondBest = score;
            }
        }
    }
    return best;
}

SongKeyEstimate SongKeyEstimator::update(const TrackedPitch& tracked,
                                         std::int64_t sampleTime) noexcept
{
    double elapsedSamples = 0.0;
    if (lastSampleTime_ >= 0 && sampleTime > lastSampleTime_)
        elapsedSamples = static_cast<double>(sampleTime - lastSampleTime_);
    lastSampleTime_ = sampleTime;

    // Approximately 18 seconds of harmonic memory. The decay uses actual
    // sample-time deltas, so it is independent of DAW block size.
    const float decay = elapsedSamples > 0.0
        ? static_cast<float>(std::exp(-elapsedSamples / (sampleRate_ * 18.0)))
        : 1.0f;
    for (auto& value : histogram_)
        value *= decay;
    evidence_ *= decay;

    const bool usable = tracked.midi >= 0.0f
        && tracked.state == PitchState::voicedStable
        && tracked.confidence >= 0.55f
        && tracked.voicing >= 0.55f;

    if (usable) {
        const float weight = tracked.confidence * tracked.voicing;
        const float baseNote = std::floor(tracked.midi);
        const int lower = wrapPitchClass(static_cast<int>(baseNote));
        const int upper = wrapPitchClass(lower + 1);
        const float fraction = std::clamp(tracked.midi - baseNote, 0.0f, 1.0f);

        histogram_[static_cast<std::size_t>(lower)] += weight * (1.0f - fraction);
        histogram_[static_cast<std::size_t>(upper)] += weight * fraction;
        evidence_ += weight;
    }

    float secondBest = -2.0f;
    const auto best = scoreBest(secondBest);
    const float margin = std::max(0.0f, best.score - secondBest);
    const float evidenceConfidence = 1.0f - std::exp(-evidence_ / 18.0f);
    const float confidence = std::clamp(margin * 2.2f * evidenceConfidence,
                                        0.0f, 1.0f);
    const bool enoughEvidence = evidence_ >= 14.0f;
    const bool candidateReady = enoughEvidence && confidence >= 0.16f;

    if (!estimate_.ready) {
        estimate_.key = best.key;
        estimate_.scale = best.scale;
        estimate_.confidence = confidence;
        estimate_.evidence = evidence_;
        estimate_.ready = candidateReady;
        return estimate_;
    }

    const float lockedScore = scoreCandidate(estimate_.key, estimate_.scale);
    const bool sameCandidate = best.key == estimate_.key && best.scale == estimate_.scale;
    if (sameCandidate) {
        challengerFrames_ = 0;
        challengerKey_ = -1;
    } else if (candidateReady && best.score > lockedScore + 0.08f) {
        if (challengerKey_ == best.key && challengerScale_ == best.scale) {
            ++challengerFrames_;
        } else {
            challengerKey_ = best.key;
            challengerScale_ = best.scale;
            challengerFrames_ = 1;
        }

        // Prevent momentary melodic notes from changing the global key.
        if (challengerFrames_ >= 8) {
            estimate_.key = best.key;
            estimate_.scale = best.scale;
            challengerFrames_ = 0;
            challengerKey_ = -1;
        }
    } else {
        challengerFrames_ = 0;
        challengerKey_ = -1;
    }

    estimate_.confidence = confidence;
    estimate_.evidence = evidence_;
    estimate_.ready = enoughEvidence && confidence >= 0.12f;
    return estimate_;
}

} // namespace sonraptune
