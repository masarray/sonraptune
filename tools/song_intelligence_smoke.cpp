#include "engine/intelligence/SongKeyEstimator.h"
#include "engine/correction/CorrectionTrajectory.h"

#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

sonraptune::TrackedPitch stablePitch(float midi)
{
    sonraptune::TrackedPitch tracked;
    tracked.midi = midi;
    tracked.hz = sonraptune::midiToHz(midi);
    tracked.confidence = 0.98f;
    tracked.voicing = 0.98f;
    tracked.state = sonraptune::PitchState::voicedStable;
    return tracked;
}

} // namespace

int main()
{
    using namespace sonraptune;

    SongKeyEstimator estimator;
    estimator.prepare(48000.0);

    // A-minor melodic fixture with tonic/dominant emphasis. This deliberately
    // distinguishes A minor from its relative C major rather than merely
    // checking that all notes belong to the same pitch-class collection.
    constexpr std::array<float, 12> aMinorPhrase{
        69.0f, 69.0f, 64.0f, 72.0f, 69.0f, 67.0f,
        64.0f, 74.0f, 72.0f, 69.0f, 64.0f, 69.0f
    };

    std::int64_t sampleTime = 0;
    SongKeyEstimate estimate;
    for (int repeat = 0; repeat < 16; ++repeat) {
        for (const float midi : aMinorPhrase) {
            sampleTime += 512;
            estimate = estimator.update(stablePitch(midi), sampleTime);
        }
    }

    if (!estimate.ready || estimate.key != 9
        || estimate.scale != ScaleType::naturalMinor
        || estimate.confidence < 0.18f) {
        std::cerr << "A-minor key inference failed: ready=" << estimate.ready
                  << " key=" << estimate.key
                  << " scale=" << static_cast<int>(estimate.scale)
                  << " confidence=" << estimate.confidence << '\n';
        return 1;
    }

    // Equal chromatic evidence must not produce a confident tonal claim.
    estimator.reset();
    sampleTime = 0;
    for (int repeat = 0; repeat < 10; ++repeat) {
        for (int pitchClass = 0; pitchClass < 12; ++pitchClass) {
            sampleTime += 512;
            estimator.update(stablePitch(60.0f + static_cast<float>(pitchClass)), sampleTime);
        }
    }
    const auto chromatic = estimator.estimate();
    if (chromatic.ready && chromatic.confidence > 0.20f) {
        std::cerr << "chromatic fixture was over-confident: "
                  << chromatic.confidence << '\n';
        return 2;
    }

    // Phrase-end behavior: when voicing disappears, wet gain may release to
    // dry, but the correction ratio should not simultaneously scoop to unity.
    RuntimeParameters parameters;
    parameters.mode = ProductMode::modernRap;
    parameters.tune = 1.0f;
    parameters.speedMs = 1.0f;
    parameters.feel = 0.0f;

    CorrectionTrajectory trajectory;
    trajectory.prepare(48000.0);
    auto tracked = stablePitch(60.50f);
    trajectory.setFrame(tracked, 61.0f, parameters);

    std::vector<float> ratio(4096, 1.0f);
    std::vector<float> mask(4096, 0.0f);
    trajectory.render(ratio.data(), mask.data(), static_cast<int>(ratio.size()), parameters);
    const float lockedRatio = ratio.back();
    if (lockedRatio < 1.02f || mask.back() < 0.95f) {
        std::cerr << "stable correction fixture did not settle: ratio=" << lockedRatio
                  << " mask=" << mask.back() << '\n';
        return 3;
    }

    TrackedPitch release;
    release.state = PitchState::unvoiced;
    trajectory.setFrame(release, -1.0f, parameters);
    std::vector<float> releaseRatio(2048, 1.0f);
    std::vector<float> releaseMask(2048, 0.0f);
    trajectory.render(releaseRatio.data(), releaseMask.data(),
                      static_cast<int>(releaseRatio.size()), parameters);

    if (releaseRatio.back() < lockedRatio * 0.995f || releaseMask.back() > 0.05f) {
        std::cerr << "phrase-end lock failed: locked=" << lockedRatio
                  << " releasedRatio=" << releaseRatio.back()
                  << " releasedMask=" << releaseMask.back() << '\n';
        return 4;
    }

    std::cout << "song_intelligence_smoke PASS key=A minor confidence="
              << estimate.confidence
              << " chromatic_confidence=" << chromatic.confidence
              << " phrase_ratio=" << lockedRatio << '/' << releaseRatio.back()
              << '\n';
    return 0;
}
