#include "engine/shifting/PeriodSynchronousTimeDomainShifter.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {
namespace {
constexpr float kPi = 3.14159265358979323846f;

float smoothingAlpha(double sampleRate, double seconds) noexcept
{
    return 1.0f - std::exp(-1.0f
        / static_cast<float>(std::max(1.0, sampleRate * seconds)));
}
}

void PeriodSynchronousTimeDomainShifter::prepare(double sampleRate,
                                                  int maxBlock,
                                                  int channels)
{
    sampleRate_ = std::max(8000.0, sampleRate);
    preparedChannels_ = std::clamp(channels, 1, 2);

    markEstimator_.prepare(sampleRate_);
    maxRadiusSamples_ = markEstimator_.maxPeriodSamples();

    const int schedulingMargin = std::max(
        32, static_cast<int>(std::ceil(sampleRate_ * 0.008)));
    latencySamples_ = 2 * maxRadiusSamples_ + schedulingMargin;

    ringSize_ = 1;
    while (ringSize_ < latencySamples_
            + 2 * maxRadiusSamples_
            + std::max(16, maxBlock)
            + 4096) {
        ringSize_ <<= 1;
    }
    ringMask_ = ringSize_ - 1;

    for (auto& channel : inputRing_)
        channel.assign(static_cast<std::size_t>(ringSize_), 0.0f);
    for (auto& channel : outputSum_)
        channel.assign(static_cast<std::size_t>(ringSize_), 0.0f);
    outputWeight_.assign(static_cast<std::size_t>(ringSize_), 0.0f);
    outputGeneration_.assign(static_cast<std::size_t>(ringSize_), 0u);

    ratioSmoothingAlpha_ = smoothingAlpha(sampleRate_, 0.004);
    coverageAttackAlpha_ = smoothingAlpha(sampleRate_, 0.006);
    coverageReleaseAlpha_ = smoothingAlpha(sampleRate_, 0.006);

    reset();
}

void PeriodSynchronousTimeDomainShifter::reset() noexcept
{
    absoluteSample_ = 0;
    sourceReliable_ = false;
    smoothedRatio_ = 1.0f;
    coverageMix_ = 0.0f;
    generation_ = 1;
    diagnostics_ = {};

    markEstimator_.reset();
    clearVoicedState();

    for (auto& channel : inputRing_)
        std::fill(channel.begin(), channel.end(), 0.0f);
    for (auto& channel : outputSum_)
        std::fill(channel.begin(), channel.end(), 0.0f);
    std::fill(outputWeight_.begin(), outputWeight_.end(), 0.0f);
    std::fill(outputGeneration_.begin(), outputGeneration_.end(), 0u);
}

void PeriodSynchronousTimeDomainShifter::clearVoicedState() noexcept
{
    pitchMarks_.fill(-1);
    pitchMarkWrite_ = 0;
    pitchMarkCount_ = 0;
    nextSynthesisTime_ = -1.0;
}

void PeriodSynchronousTimeDomainShifter::setSourcePitch(float hz,
                                                         float confidence,
                                                         float voicing) noexcept
{
    const bool canEnter = hz > 0.0f && confidence >= 0.60f && voicing >= 0.65f;
    const bool canStay = hz > 0.0f && confidence >= 0.35f && voicing >= 0.40f;
    const bool reliable = sourceReliable_ ? canStay : canEnter;

    if (reliable != sourceReliable_) {
        sourceReliable_ = reliable;
        ++diagnostics_.reliabilityTransitions;
        markEstimator_.reset();
        clearVoicedState();

        // Do not invalidate grains that are already scheduled. They are
        // windowed and must finish while coverageMix_ crossfades to aligned
        // dry. Discarding them instantly created the audible pretek/pretek at
        // voiced/unvoiced boundaries.
    }

    if (sourceReliable_)
        markEstimator_.setPitch(hz, confidence, voicing);
}

float PeriodSynchronousTimeDomainShifter::readInput(
    int channel,
    std::int64_t absoluteSample) const noexcept
{
    if (absoluteSample < 0)
        return 0.0f;

    return inputRing_[static_cast<std::size_t>(channel)]
                     [static_cast<std::size_t>(absoluteSample & ringMask_)];
}

void PeriodSynchronousTimeDomainShifter::writeInput(
    int channel,
    std::int64_t absoluteSample,
    float value) noexcept
{
    inputRing_[static_cast<std::size_t>(channel)]
              [static_cast<std::size_t>(absoluteSample & ringMask_)] = value;
}

void PeriodSynchronousTimeDomainShifter::commitPitchMark(
    std::int64_t mark) noexcept
{
    pitchMarks_[static_cast<std::size_t>(pitchMarkWrite_)] = mark;
    pitchMarkWrite_ = (pitchMarkWrite_ + 1) % kMaxMarks;
    pitchMarkCount_ = std::min(kMaxMarks, pitchMarkCount_ + 1);

    if (nextSynthesisTime_ < 0.0 && pitchMarkCount_ >= 2)
        nextSynthesisTime_ = static_cast<double>(mark);
}

std::int64_t PeriodSynchronousTimeDomainShifter::nearestReadyPitchMark(
    double sourceTime,
    std::int64_t latestReadyMark) const noexcept
{
    std::int64_t best = -1;
    double bestDistance = 1.0e30;

    for (int i = 0; i < pitchMarkCount_; ++i) {
        const auto mark = pitchMarks_[static_cast<std::size_t>(i)];
        if (mark < 0 || mark > latestReadyMark)
            continue;

        const double distance = std::abs(static_cast<double>(mark) - sourceTime);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = mark;
        }
    }

    return best;
}

void PeriodSynchronousTimeDomainShifter::scheduleGrain(
    std::int64_t sourceMark,
    std::int64_t synthesisMark,
    int radius,
    std::int64_t earliestOutputSample) noexcept
{
    radius = std::clamp(radius, 16, maxRadiusSamples_);
    const float inverseRadius = 1.0f / static_cast<float>(radius);

    for (int offset = -radius; offset <= radius; ++offset) {
        const auto outputSample = synthesisMark + offset;
        if (outputSample < earliestOutputSample) {
            ++diagnostics_.rejectedPastWrites;
            continue;
        }

        const float normalised = static_cast<float>(offset) * inverseRadius;
        const float window = 0.5f * (1.0f + std::cos(kPi * normalised));
        const auto outputIndex = static_cast<std::size_t>(outputSample & ringMask_);

        if (outputGeneration_[outputIndex] != generation_) {
            outputGeneration_[outputIndex] = generation_;
            outputWeight_[outputIndex] = 0.0f;
            for (int channel = 0; channel < preparedChannels_; ++channel)
                outputSum_[static_cast<std::size_t>(channel)][outputIndex] = 0.0f;
        }

        outputWeight_[outputIndex] += window;
        for (int channel = 0; channel < preparedChannels_; ++channel) {
            outputSum_[static_cast<std::size_t>(channel)][outputIndex]
                += window * readInput(channel, sourceMark + offset);
        }
    }
}

void PeriodSynchronousTimeDomainShifter::scheduleAvailableGrains(
    std::int64_t currentSample,
    float ratio) noexcept
{
    if (!sourceReliable_ || nextSynthesisTime_ < 0.0 || pitchMarkCount_ < 2)
        return;

    float sourcePeriod = markEstimator_.periodSamples();
    if (sourcePeriod <= 0.0f)
        return;

    sourcePeriod = std::clamp(
        sourcePeriod,
        static_cast<float>(markEstimator_.minPeriodSamples()),
        static_cast<float>(markEstimator_.maxPeriodSamples()));

    const int radius = std::clamp(
        static_cast<int>(std::lround(sourcePeriod)),
        16,
        maxRadiusSamples_);
    const std::int64_t latestReadyMark = currentSample - radius - 2;
    const double safeSynthesisTime = static_cast<double>(latestReadyMark);

    ratio = std::clamp(ratio, 0.5f, 2.0f);
    const double targetPeriod = static_cast<double>(sourcePeriod)
        / static_cast<double>(ratio);

    // The synthesis clock remains tied to elapsed stream time. At upward
    // shifts it naturally reuses a nearby source grain; at downward shifts it
    // skips source marks. This preserves duration while changing pitch.
    int guard = 0;
    while (nextSynthesisTime_ <= safeSynthesisTime && guard++ < 8) {
        const auto sourceMark = nearestReadyPitchMark(
            nextSynthesisTime_, latestReadyMark);
        if (sourceMark < 0)
            break;

        const auto synthesisMark = static_cast<std::int64_t>(
            std::llround(nextSynthesisTime_)) + latencySamples_;
        scheduleGrain(sourceMark, synthesisMark, radius, currentSample);
        nextSynthesisTime_ += targetPeriod;
    }
}

void PeriodSynchronousTimeDomainShifter::process(
    float* const* channels,
    int numChannels,
    int numSamples,
    const float* ratioPerSample,
    const float* voicedMaskPerSample) noexcept
{
    if (channels == nullptr
        || ratioPerSample == nullptr
        || voicedMaskPerSample == nullptr
        || numSamples <= 0) {
        return;
    }

    numChannels = std::clamp(numChannels, 1, preparedChannels_);

    for (int sample = 0; sample < numSamples; ++sample) {
        float mono = 0.0f;
        int activeChannels = 0;

        for (int channel = 0; channel < numChannels; ++channel) {
            const float input = channels[channel] != nullptr
                ? channels[channel][sample]
                : 0.0f;
            writeInput(channel, absoluteSample_, input);
            mono += input;
            ++activeChannels;
        }

        if (activeChannels > 0)
            mono /= static_cast<float>(activeChannels);

        std::int64_t pitchMark = -1;
        if (sourceReliable_
            && markEstimator_.pushSample(mono, absoluteSample_, pitchMark)) {
            commitPitchMark(pitchMark);
        }

        const float requestedRatio = std::clamp(
            ratioPerSample[sample], 0.5f, 2.0f);
        smoothedRatio_ += ratioSmoothingAlpha_
            * (requestedRatio - smoothedRatio_);
        scheduleAvailableGrains(absoluteSample_, smoothedRatio_);

        const auto outputIndex = static_cast<std::size_t>(
            absoluteSample_ & ringMask_);
        const bool currentGeneration =
            outputGeneration_[outputIndex] == generation_;
        const float weight = currentGeneration
            ? outputWeight_[outputIndex]
            : 0.0f;

        const float coverageTarget = weight >= 0.20f
            ? std::clamp((weight - 0.20f) / 0.60f, 0.0f, 1.0f)
            : 0.0f;
        const float coverageAlpha = coverageTarget > coverageMix_
            ? coverageAttackAlpha_
            : coverageReleaseAlpha_;
        coverageMix_ += coverageAlpha * (coverageTarget - coverageMix_);

        const float requestedWet = std::clamp(
            voicedMaskPerSample[sample], 0.0f, 1.0f);
        const float effectiveWet = requestedWet * coverageMix_;
        if (requestedWet > 0.5f && coverageTarget <= 0.0f)
            ++diagnostics_.coverageFallbackSamples;

        for (int channel = 0; channel < numChannels; ++channel) {
            if (channels[channel] == nullptr)
                continue;

            const float alignedDry = readInput(
                channel, absoluteSample_ - latencySamples_);
            float shifted = alignedDry;
            if (currentGeneration && weight >= 0.20f) {
                shifted = outputSum_[static_cast<std::size_t>(channel)][outputIndex]
                    / weight;
            }

            if (std::abs(smoothedRatio_ - 1.0f) < 0.0005f)
                shifted = alignedDry;

            channels[channel][sample] = alignedDry
                + effectiveWet * (shifted - alignedDry);
        }

        if (currentGeneration) {
            for (int channel = 0; channel < preparedChannels_; ++channel)
                outputSum_[static_cast<std::size_t>(channel)][outputIndex] = 0.0f;
            outputWeight_[outputIndex] = 0.0f;
            outputGeneration_[outputIndex] = 0u;
        }

        ++absoluteSample_;
    }
}

} // namespace sonraptune
