#include "engine/correction/CorrectionTrajectory.h"
#include "engine/mapping/ScaleMapper.h"
#include "engine/protection/ConsonantProtection.h"
#include "engine/shifting/PeriodSynchronousTimeDomainShifter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace {
constexpr double kSampleRate = 48000.0;
constexpr float kPi = 3.14159265358979323846f;

float renderedCorrection(sonraptune::ProductMode mode)
{
    sonraptune::CorrectionTrajectory trajectory;
    trajectory.prepare(kSampleRate);

    sonraptune::RuntimeParameters p;
    p.mode = mode;
    p.tune = 1.0f;
    p.speedMs = 20.0f;
    p.feel = 0.25f;

    sonraptune::TrackedPitch tracked;
    tracked.midi = 60.38f;
    tracked.hz = sonraptune::midiToHz(tracked.midi);
    tracked.confidence = 1.0f;
    tracked.voicing = 1.0f;
    tracked.state = sonraptune::PitchState::voicedStable;

    trajectory.setFrame(tracked, 60.0f, p);
    std::array<float, 480> ratio{};
    std::array<float, 480> mask{};
    trajectory.render(ratio.data(), mask.data(), static_cast<int>(ratio.size()), p);
    return std::abs(trajectory.correctionCents());
}

float protectionAverage(bool noisy)
{
    sonraptune::ConsonantProtection protection;
    protection.prepare(kSampleRate);

    constexpr int n = 4096;
    std::array<float, n> mono{};
    std::array<float, n> mask{};
    mask.fill(1.0f);

    for (int i = 0; i < n; ++i) {
        if (noisy) {
            mono[static_cast<std::size_t>(i)] = (i & 1) == 0 ? 0.35f : -0.35f;
        } else {
            mono[static_cast<std::size_t>(i)] = 0.35f * std::sin(
                2.0f * kPi * 140.0f * static_cast<float>(i)
                / static_cast<float>(kSampleRate));
        }
    }

    sonraptune::PitchFrame frame;
    frame.voicing = 0.62f;
    frame.confidence = 0.80f;
    frame.state = sonraptune::PitchState::voicedStable;

    protection.process(mono.data(), mask.data(), n, 1.0f, frame);

    double sum = 0.0;
    for (int i = n / 2; i < n; ++i)
        sum += mask[static_cast<std::size_t>(i)];
    return static_cast<float>(sum / static_cast<double>(n / 2));
}

int grainRadius(float formantAmount)
{
    sonraptune::PeriodSynchronousTimeDomainShifter shifter;
    shifter.prepare(kSampleRate, 256, 1);
    shifter.setSourcePitch(120.0f, 1.0f, 1.0f);
    shifter.setFormantPreserve(formantAmount);

    constexpr int total = 24000;
    constexpr int block = 256;
    std::array<float, block> audio{};
    std::array<float, block> ratio{};
    std::array<float, block> mask{};
    ratio.fill(1.5f);
    mask.fill(1.0f);

    int rendered = 0;
    while (rendered < total) {
        const int count = std::min(block, total - rendered);
        for (int i = 0; i < count; ++i) {
            const int t = rendered + i;
            audio[static_cast<std::size_t>(i)] = 0.35f * std::sin(
                2.0f * kPi * 120.0f * static_cast<float>(t)
                / static_cast<float>(kSampleRate));
        }
        float* channels[] = {audio.data()};
        shifter.process(channels, 1, count, ratio.data(), mask.data());
        rendered += count;
    }

    if (shifter.diagnostics().rejectedPastWrites != 0)
        return -1;
    return shifter.diagnostics().lastGrainRadius;
}

bool customScaleWorks()
{
    sonraptune::RuntimeParameters p;
    p.key = 0;
    p.scale = sonraptune::ScaleType::customMask;
    p.customMask = static_cast<std::uint16_t>((1u << 0) | (1u << 7));

    sonraptune::ScaleMapper mapper;
    const float first = mapper.map(66.7f, true, p);
    mapper.reset();
    p.customMask = 1u << 0;
    const float second = mapper.map(66.7f, true, p);
    return std::lround(first) == 67 && std::lround(second) == 72;
}

} // namespace

int main()
{
    const float natural = renderedCorrection(sonraptune::ProductMode::natural);
    const float modern = renderedCorrection(sonraptune::ProductMode::modernRap);
    const float trap = renderedCorrection(sonraptune::ProductMode::trapLock);

    if (!(natural < modern && modern < trap)) {
        std::cerr << "style curves not ordered: natural=" << natural
                  << " modern=" << modern << " trap=" << trap << '\n';
        return 1;
    }

    const float vowelWet = protectionAverage(false);
    const float noisyWet = protectionAverage(true);
    if (!(vowelWet > noisyWet + 0.08f && vowelWet > 0.80f && noisyWet < 0.82f)) {
        std::cerr << "consonant protection separation failed: vowel=" << vowelWet
                  << " noisy=" << noisyWet << '\n';
        return 2;
    }

    const int preservedRadius = grainRadius(1.0f);
    const int coupledRadius = grainRadius(0.0f);
    if (!(preservedRadius > 0 && coupledRadius > 0
          && preservedRadius > coupledRadius + 40)) {
        std::cerr << "formant control did not change grain geometry: preserve="
                  << preservedRadius << " coupled=" << coupledRadius << '\n';
        return 3;
    }

    if (!customScaleWorks()) {
        std::cerr << "custom scale mask did not change target mapping\n";
        return 4;
    }

    std::cout << "style_protection_smoke PASS"
              << " natural=" << natural
              << " modern=" << modern
              << " trap=" << trap
              << " vowel_wet=" << vowelWet
              << " noisy_wet=" << noisyWet
              << " formant_radius=" << preservedRadius
              << '/' << coupledRadius << '\n';
    return 0;
}
