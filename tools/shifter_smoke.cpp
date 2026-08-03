#include "engine/shifting/DualHeadTimeDomainShifter.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

float estimatePositiveCrossingFrequency(const std::vector<float>& signal,
                                        int sampleRate,
                                        int startSample)
{
    int crossings = 0;
    for (int i = std::max(1, startSample + 1); i < static_cast<int>(signal.size()); ++i)
        if (signal[static_cast<std::size_t>(i - 1)] <= 0.0f
            && signal[static_cast<std::size_t>(i)] > 0.0f)
            ++crossings;

    const float seconds = static_cast<float>(signal.size() - startSample)
        / static_cast<float>(sampleRate);
    return seconds > 0.0f ? static_cast<float>(crossings) / seconds : 0.0f;
}

bool finiteAndBounded(const std::vector<float>& signal)
{
    for (const float value : signal)
        if (!std::isfinite(value) || std::abs(value) > 2.0f)
            return false;
    return true;
}

} // namespace

int main()
{
    constexpr int sampleRate = 48000;
    constexpr int blockSize = 128;
    constexpr float inputHz = 220.0f;
    constexpr int seconds = 3;
    const int sampleCount = sampleRate * seconds;
    const float semitoneRatio = std::pow(2.0f, 1.0f / 12.0f);
    const float expectedHz = inputHz * semitoneRatio;

    std::vector<float> audio(static_cast<std::size_t>(sampleCount));
    std::vector<float> ratio(static_cast<std::size_t>(sampleCount), semitoneRatio);
    std::vector<float> mask(static_cast<std::size_t>(sampleCount), 1.0f);

    for (int i = 0; i < sampleCount; ++i)
        audio[static_cast<std::size_t>(i)] = 0.3f * std::sin(
            2.0f * 3.14159265358979323846f * inputHz
            * static_cast<float>(i) / static_cast<float>(sampleRate));

    sonraptune::DualHeadTimeDomainShifter shifter;
    shifter.prepare(sampleRate, blockSize, 1);

    for (int position = 0; position < sampleCount; position += blockSize) {
        const int count = std::min(blockSize, sampleCount - position);
        float* channel[] = { audio.data() + position };
        shifter.process(channel, 1, count,
                        ratio.data() + position,
                        mask.data() + position);
    }

    const float measuredHz = estimatePositiveCrossingFrequency(
        audio, sampleRate, sampleRate);
    const float centsError = 1200.0f * std::log2(measuredHz / expectedHz);

    std::cout << "E3 shifter smoke\n"
              << "  latency samples: " << shifter.latencySamples() << '\n'
              << "  expected Hz: " << expectedHz << '\n'
              << "  measured Hz: " << measuredHz << '\n'
              << "  cents error: " << centsError << '\n';

    if (!finiteAndBounded(audio)) {
        std::cerr << "FAIL: non-finite or unbounded output\n";
        return 1;
    }

    if (std::abs(centsError) > 35.0f) {
        std::cerr << "FAIL: frequency shift outside prototype tolerance\n";
        return 2;
    }

    if (shifter.latencySamples() <= 0) {
        std::cerr << "FAIL: candidate must report fixed positive latency\n";
        return 3;
    }

    std::cout << "PASS\n";
    return 0;
}
