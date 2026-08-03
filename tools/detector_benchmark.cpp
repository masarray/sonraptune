#include "engine/analysis/YinCandidateDetector.h"
#include "engine/tracking/CausalPitchTracker.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

using namespace sonraptune;

static float median(std::vector<float> values)
{
    if (values.empty()) return 0.0f;
    const auto mid = values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
    std::nth_element(values.begin(), mid, values.end());
    return *mid;
}

int main()
{
    constexpr double sr = 48000.0;
    constexpr int block = 64;
    const std::vector<float> frequencies{70.0f, 82.41f, 98.0f, 110.0f, 146.83f,
        196.0f, 220.0f, 293.66f, 440.0f, 659.25f, 880.0f};

    std::vector<float> allErrors;
    int gross = 0;
    int voicedFrames = 0;
    int totalFrames = 0;

    for (float f : frequencies) {
        YinCandidateDetector detector;
        CausalPitchTracker tracker;
        detector.prepare(sr);
        tracker.reset();
        double phase = 0.0;
        std::vector<float> buffer(block);
        std::vector<float> errors;
        const int samples = static_cast<int>(sr * 0.75);
        for (int pos = 0; pos < samples; pos += block) {
            const int n = std::min(block, samples - pos);
            for (int i = 0; i < n; ++i) {
                phase += 2.0 * 3.14159265358979323846 * f / sr;
                if (phase > 2.0 * 3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;
                const float s = 0.32f * std::sin(phase)
                              + 0.12f * std::sin(2.0 * phase)
                              + 0.05f * std::sin(3.0 * phase);
                buffer[static_cast<std::size_t>(i)] = s;
            }
            PitchAnalysis analysis;
            if (detector.push(buffer.data(), n, VocalRange::automatic, analysis)) {
                ++totalFrames;
                const auto tracked = tracker.update(analysis, 0.7f);
                if (tracked.hz > 0.0f && pos > static_cast<int>(sr * 0.20)) {
                    ++voicedFrames;
                    const float e = std::abs(centsBetween(tracked.hz, f));
                    errors.push_back(e);
                    allErrors.push_back(e);
                    if (e > 100.0f) ++gross;
                }
            }
        }
        std::cout << std::fixed << std::setprecision(2)
                  << "F0 " << f << " Hz median_error_cents=" << median(errors)
                  << " frames=" << errors.size() << "\n";
    }

    const float med = median(allErrors);
    const float grossRate = allErrors.empty() ? 1.0f : static_cast<float>(gross) / allErrors.size();
    const float voicedRate = totalFrames > 0 ? static_cast<float>(voicedFrames) / totalFrames : 0.0f;
    std::cout << "{\n"
              << "  \"medianFinePitchErrorCents\": " << med << ",\n"
              << "  \"grossErrorRate\": " << grossRate << ",\n"
              << "  \"voicedFrameRateAfterWarmup\": " << voicedRate << ",\n"
              << "  \"sampleRate\": 48000\n"
              << "}\n";

    const bool pass = med <= 10.0f && grossRate < 0.005f && voicedRate > 0.65f;
    return pass ? 0 : 2;
}
