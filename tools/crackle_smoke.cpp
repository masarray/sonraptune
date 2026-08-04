#include "engine/shifting/PeriodSynchronousTimeDomainShifter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;

float smoothStep(float value) noexcept
{
    value = std::clamp(value, 0.0f, 1.0f);
    return value * value * (3.0f - 2.0f * value);
}

float phraseEnvelope(float localTime, float voicedLength) noexcept
{
    if (localTime < 0.0f || localTime >= voicedLength)
        return 0.0f;
    const float attack = smoothStep(localTime / 0.020f);
    const float release = smoothStep((voicedLength - localTime) / 0.035f);
    return std::min(attack, release);
}

} // namespace

int main()
{
    constexpr int sampleRate = 48000;
    constexpr int blockSize = 127;
    constexpr float seconds = 4.0f;
    const int sampleCount = static_cast<int>(seconds * sampleRate);

    sonraptune::PeriodSynchronousTimeDomainShifter shifter;
    shifter.prepare(sampleRate, blockSize, 1);

    std::vector<float> rendered(static_cast<std::size_t>(sampleCount), 0.0f);
    std::vector<float> block(static_cast<std::size_t>(blockSize), 0.0f);
    std::vector<float> ratios(static_cast<std::size_t>(blockSize), 1.0f);
    std::vector<float> masks(static_cast<std::size_t>(blockSize), 0.0f);
    float* channels[] = {block.data()};

    double phase = 0.0;
    float ratioState = 1.0f;
    float maskState = 0.0f;
    std::uint32_t noiseState = 0x12345678u;

    const float ratioAlpha = 1.0f - std::exp(-1.0f / (sampleRate * 0.010f));
    const float maskAttack = 1.0f - std::exp(-1.0f / (sampleRate * 0.008f));
    const float maskRelease = 1.0f - std::exp(-1.0f / (sampleRate * 0.004f));

    for (int position = 0; position < sampleCount; position += blockSize) {
        const int count = std::min(blockSize, sampleCount - position);
        const float blockMidTime = static_cast<float>(position + count / 2)
            / static_cast<float>(sampleRate);
        const float blockLocal = std::fmod(blockMidTime, 0.70f);
        const bool blockVoiced = blockLocal < 0.55f;
        const float blockPitch = 75.0f + 85.0f * blockMidTime / seconds
            + 2.5f * std::sin(2.0f * kPi * 5.0f * blockMidTime);

        shifter.setSourcePitch(blockPitch,
                               blockVoiced ? 0.92f : 0.10f,
                               blockVoiced ? 0.95f : 0.08f);

        for (int sample = 0; sample < count; ++sample) {
            const int absolute = position + sample;
            const float time = static_cast<float>(absolute)
                / static_cast<float>(sampleRate);
            const float local = std::fmod(time, 0.70f);
            const bool voiced = local < 0.55f;
            const float envelope = voiced
                ? phraseEnvelope(local, 0.55f)
                : 0.12f * phraseEnvelope(local - 0.55f, 0.15f);

            const float fundamental = 75.0f + 85.0f * time / seconds
                + 2.5f * std::sin(2.0f * kPi * 5.0f * time);
            phase += 2.0 * kPi * static_cast<double>(fundamental)
                / static_cast<double>(sampleRate);
            if (phase >= 2.0 * kPi)
                phase -= 2.0 * kPi;

            float value = 0.0f;
            if (voiced) {
                for (int harmonic = 1; harmonic <= 8; ++harmonic) {
                    const float spectralTilt = 0.18f
                        / static_cast<float>(harmonic);
                    value += spectralTilt * static_cast<float>(
                        std::sin(phase * static_cast<double>(harmonic)));
                }
            } else {
                noiseState = 1664525u * noiseState + 1013904223u;
                const float noise = static_cast<float>(
                    static_cast<std::int32_t>(noiseState))
                    / 2147483648.0f;
                value = 0.04f * noise;
            }
            block[static_cast<std::size_t>(sample)] = envelope * value;

            const float ratioTarget = std::fmod(time, 2.0f) < 1.0f
                ? std::pow(2.0f, 2.0f / 12.0f)
                : std::pow(2.0f, -2.0f / 12.0f);
            ratioState += ratioAlpha * (ratioTarget - ratioState);
            ratios[static_cast<std::size_t>(sample)] = ratioState;

            const float maskTarget = voiced ? 1.0f : 0.0f;
            const float alpha = maskTarget > maskState
                ? maskAttack
                : maskRelease;
            maskState += alpha * (maskTarget - maskState);
            masks[static_cast<std::size_t>(sample)] = maskState;
        }

        shifter.process(channels, 1, count, ratios.data(), masks.data());
        for (int sample = 0; sample < count; ++sample) {
            rendered[static_cast<std::size_t>(position + sample)] =
                block[static_cast<std::size_t>(sample)];
        }
    }

    float maximumAbsolute = 0.0f;
    float maximumStep = 0.0f;
    int crackleSteps = 0;
    const int analysisStart = shifter.latencySamples() + sampleRate / 10;
    for (int sample = std::max(1, analysisStart);
         sample < sampleCount;
         ++sample) {
        const float current = rendered[static_cast<std::size_t>(sample)];
        const float previous = rendered[static_cast<std::size_t>(sample - 1)];
        if (!std::isfinite(current)) {
            std::cerr << "FAIL: non-finite output\n";
            return 1;
        }
        maximumAbsolute = std::max(maximumAbsolute, std::abs(current));
        const float step = std::abs(current - previous);
        maximumStep = std::max(maximumStep, step);
        if (step > 0.25f)
            ++crackleSteps;
    }

    const auto diagnostics = shifter.diagnostics();
    std::cout << "crackle_smoke max_abs=" << maximumAbsolute
              << " max_step=" << maximumStep
              << " crackle_steps=" << crackleSteps
              << " rejected_past_writes=" << diagnostics.rejectedPastWrites
              << " coverage_fallback_samples="
              << diagnostics.coverageFallbackSamples
              << " reliability_transitions="
              << diagnostics.reliabilityTransitions << '\n';

    if (diagnostics.rejectedPastWrites != 0) {
        std::cerr << "FAIL: PSOLA attempted to write into consumed output\n";
        return 2;
    }
    if (maximumAbsolute > 1.25f) {
        std::cerr << "FAIL: output spike exceeds containment limit\n";
        return 3;
    }
    if (crackleSteps != 0 || maximumStep > 0.25f) {
        std::cerr << "FAIL: discontinuity/crackle threshold exceeded\n";
        return 4;
    }
    if (diagnostics.reliabilityTransitions < 4) {
        std::cerr << "FAIL: test did not exercise voiced-state transitions\n";
        return 5;
    }

    std::cout << "PASS\n";
    return 0;
}
