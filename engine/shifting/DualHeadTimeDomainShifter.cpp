#include "engine/shifting/DualHeadTimeDomainShifter.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {

void DualHeadTimeDomainShifter::prepare(double sampleRate, int, int channels)
{
    sampleRate_ = std::max(8000.0, sampleRate);
    preparedChannels_ = std::clamp(channels, 1, 2);

    // 20 ms grain + 6 ms base delay. This is intentionally conservative for
    // the first audible bake-off candidate; E3 optimisation will reduce the
    // latency only after recorded-vocal quality has been measured.
    grainSamples_ = std::max(64, static_cast<int>(std::lround(sampleRate_ * 0.020)));
    baseDelaySamples_ = std::max(16, static_cast<int>(std::lround(sampleRate_ * 0.006)));
    latencySamples_ = baseDelaySamples_ + grainSamples_;

    ringSize_ = 1;
    while (ringSize_ < latencySamples_ + grainSamples_ + 4096)
        ringSize_ <<= 1;

    for (auto& channel : ring_)
        channel.assign(static_cast<std::size_t>(ringSize_), 0.0f);

    reset();
}

void DualHeadTimeDomainShifter::reset() noexcept
{
    writePosition_ = 0;
    phase_ = 0.0f;
    for (auto& channel : ring_)
        std::fill(channel.begin(), channel.end(), 0.0f);
}

float DualHeadTimeDomainShifter::readInterpolated(const std::vector<float>& ring,
                                                   float position) const noexcept
{
    while (position < 0.0f)
        position += static_cast<float>(ringSize_);
    while (position >= static_cast<float>(ringSize_))
        position -= static_cast<float>(ringSize_);

    const int first = static_cast<int>(position);
    const int second = (first + 1) & (ringSize_ - 1);
    const float fraction = position - static_cast<float>(first);
    return ring[static_cast<std::size_t>(first)]
         + fraction * (ring[static_cast<std::size_t>(second)]
                     - ring[static_cast<std::size_t>(first)]);
}

void DualHeadTimeDomainShifter::process(float* const* channels,
                                        int numChannels,
                                        int numSamples,
                                        const float* ratioPerSample,
                                        const float* voicedMaskPerSample) noexcept
{
    if (channels == nullptr || ratioPerSample == nullptr
        || voicedMaskPerSample == nullptr || numSamples <= 0)
        return;

    numChannels = std::clamp(numChannels, 1, preparedChannels_);
    constexpr float pi = 3.14159265358979323846f;

    for (int sample = 0; sample < numSamples; ++sample) {
        const float ratio = std::clamp(ratioPerSample[sample], 0.5f, 2.0f);
        const float delaySlope = 1.0f - ratio;
        const float phaseIncrement = std::abs(delaySlope)
            / static_cast<float>(grainSamples_);

        phase_ += phaseIncrement;
        if (phase_ >= 1.0f)
            phase_ -= std::floor(phase_);

        const float phaseA = phase_;
        float phaseB = phaseA + 0.5f;
        if (phaseB >= 1.0f)
            phaseB -= 1.0f;

        float weightA = std::sin(pi * phaseA);
        weightA *= weightA;
        const float weightB = 1.0f - weightA;

        // For upward shifts, delay decreases during each grain. For downward
        // shifts, delay increases. The second head is half a cycle apart and
        // the sin^2 windows sum to unity.
        const float delayA = static_cast<float>(baseDelaySamples_)
            + (delaySlope < 0.0f ? (1.0f - phaseA) : phaseA)
                * static_cast<float>(grainSamples_);
        const float delayB = static_cast<float>(baseDelaySamples_)
            + (delaySlope < 0.0f ? (1.0f - phaseB) : phaseB)
                * static_cast<float>(grainSamples_);

        const float wetMask = std::clamp(voicedMaskPerSample[sample], 0.0f, 1.0f);

        for (int channel = 0; channel < numChannels; ++channel) {
            if (channels[channel] == nullptr)
                continue;

            auto& ring = ring_[static_cast<std::size_t>(channel)];
            const float input = channels[channel][sample];
            ring[static_cast<std::size_t>(writePosition_)] = input;

            const float alignedDry = readInterpolated(
                ring, static_cast<float>(writePosition_ - latencySamples_));

            float shifted = alignedDry;
            if (std::abs(ratio - 1.0f) >= 0.0001f) {
                const float grainA = readInterpolated(
                    ring, static_cast<float>(writePosition_) - delayA);
                const float grainB = readInterpolated(
                    ring, static_cast<float>(writePosition_) - delayB);
                shifted = weightA * grainA + weightB * grainB;
            }

            channels[channel][sample] = alignedDry
                + wetMask * (shifted - alignedDry);
        }

        writePosition_ = (writePosition_ + 1) & (ringSize_ - 1);
    }
}

} // namespace sonraptune
