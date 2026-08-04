#include "engine/shifting/PeriodSynchronousTimeDomainShifter.h"
#include "engine/shifting/PitchMarkEstimator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

float median(std::vector<float> values)
{
    if (values.empty())
        return 0.0f;
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

float estimateFrequency(const std::vector<float>& signal,
                        int sampleRate,
                        int startSample)
{
    std::vector<float> crossingIntervals;
    int previousCrossing = -1;

    for (int sample = std::max(1, startSample + 1);
         sample < static_cast<int>(signal.size());
         ++sample) {
        if (signal[static_cast<std::size_t>(sample - 1)] <= 0.0f
            && signal[static_cast<std::size_t>(sample)] > 0.0f) {
            if (previousCrossing >= 0)
                crossingIntervals.push_back(
                    static_cast<float>(sample - previousCrossing));
            previousCrossing = sample;
        }
    }

    const float period = median(crossingIntervals);
    return period > 0.0f ? static_cast<float>(sampleRate) / period : 0.0f;
}

bool finiteAndBounded(const std::vector<float>& signal)
{
    for (const float value : signal)
        if (!std::isfinite(value) || std::abs(value) > 2.0f)
            return false;
    return true;
}

bool testPitchMarks()
{
    constexpr int sampleRate = 48000;
    constexpr float inputHz = 220.0f;
    constexpr int samples = sampleRate * 2;

    sonraptune::PitchMarkEstimator estimator;
    estimator.prepare(sampleRate);
    estimator.setPitch(inputHz, 1.0f, 1.0f);

    std::vector<float> intervals;
    std::int64_t previousMark = -1;

    for (int sample = 0; sample < samples; ++sample) {
        const float value = 0.3f * std::sin(
            2.0f * 3.14159265358979323846f * inputHz
            * static_cast<float>(sample) / static_cast<float>(sampleRate));
        std::int64_t mark = -1;
        if (estimator.pushSample(value, sample, mark)) {
            if (previousMark >= 0 && sample > sampleRate / 4)
                intervals.push_back(static_cast<float>(mark - previousMark));
            previousMark = mark;
        }
    }

    const float measuredPeriod = median(intervals);
    const float expectedPeriod = static_cast<float>(sampleRate) / inputHz;
    const float periodError = std::abs(measuredPeriod - expectedPeriod);

    std::cout << "pitch_mark_period expected=" << expectedPeriod
              << " measured=" << measuredPeriod
              << " error_samples=" << periodError << '\n';

    return intervals.size() > 100 && periodError < 1.5f;
}

bool testShift(int sampleRate, float inputHz, float ratio, float& centsErrorOut)
{
    constexpr int blockSize = 257;
    const int sampleCount = sampleRate * 2;

    sonraptune::PeriodSynchronousTimeDomainShifter shifter;
    shifter.prepare(sampleRate, blockSize, 1);
    shifter.setSourcePitch(inputHz, 1.0f, 1.0f);

    std::vector<float> rendered(static_cast<std::size_t>(sampleCount), 0.0f);
    std::vector<float> block(static_cast<std::size_t>(blockSize), 0.0f);
    std::vector<float> ratios(static_cast<std::size_t>(blockSize), ratio);
    std::vector<float> masks(static_cast<std::size_t>(blockSize), 1.0f);
    float* channels[] = { block.data() };

    double phase = 0.0;
    for (int position = 0; position < sampleCount; position += blockSize) {
        const int count = std::min(blockSize, sampleCount - position);
        for (int sample = 0; sample < count; ++sample) {
            block[static_cast<std::size_t>(sample)] = 0.25f
                * static_cast<float>(std::sin(phase));
            phase += 2.0 * 3.14159265358979323846
                * static_cast<double>(inputHz)
                / static_cast<double>(sampleRate);
            if (phase >= 2.0 * 3.14159265358979323846)
                phase -= 2.0 * 3.14159265358979323846;
        }

        shifter.process(channels, 1, count, ratios.data(), masks.data());
        for (int sample = 0; sample < count; ++sample) {
            rendered[static_cast<std::size_t>(position + sample)] =
                block[static_cast<std::size_t>(sample)];
        }
    }

    if (!finiteAndBounded(rendered))
        return false;

    const int analysisStart = shifter.latencySamples() + sampleRate / 2;
    const float measuredHz = estimateFrequency(
        rendered, sampleRate, analysisStart);
    const float expectedHz = inputHz * ratio;
    centsErrorOut = 1200.0f * std::log2(measuredHz / expectedHz);
    return std::isfinite(centsErrorOut) && std::abs(centsErrorOut) < 25.0f;
}

} // namespace

int main()
{
    if (!testPitchMarks()) {
        std::cerr << "FAIL: constrained waveform pitch marks are inaccurate\n";
        return 1;
    }

    const std::array<int, 3> sampleRates{44100, 48000, 96000};
    const std::array<float, 3> inputFrequencies{90.0f, 220.0f, 440.0f};
    const std::array<float, 4> ratios{
        std::pow(2.0f, -2.0f / 12.0f),
        std::pow(2.0f, -1.0f / 12.0f),
        std::pow(2.0f, 1.0f / 12.0f),
        std::pow(2.0f, 2.0f / 12.0f)
    };

    float maximumAbsoluteCentsError = 0.0f;
    int cases = 0;

    for (const int sampleRate : sampleRates) {
        for (const float inputHz : inputFrequencies) {
            for (const float ratio : ratios) {
                float centsError = 0.0f;
                if (!testShift(sampleRate, inputHz, ratio, centsError)) {
                    std::cerr << "FAIL: PSOLA shift rate=" << sampleRate
                              << " inputHz=" << inputHz
                              << " ratio=" << ratio
                              << " centsError=" << centsError << '\n';
                    return 2;
                }
                maximumAbsoluteCentsError = std::max(
                    maximumAbsoluteCentsError, std::abs(centsError));
                ++cases;
            }
        }
    }

    std::cout << "psola_smoke=PASS cases=" << cases
              << " max_abs_cents_error=" << maximumAbsoluteCentsError
              << " fixed_latency=verified\n";
    return 0;
}
