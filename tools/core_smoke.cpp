#include "engine/mapping/ScaleMapper.h"
#include "engine/correction/CorrectionTrajectory.h"

#include <bitset>
#include <cmath>
#include <iostream>
#include <vector>

using namespace sonraptune;

int main()
{
    RuntimeParameters p;
    p.key = 0;
    p.scale = ScaleType::naturalMinor;
    p.stability = 0.8f;
    p.tune = 1.0f;
    p.speedMs = 5.0f;
    p.feel = 0.0f;

    if (ScaleMapper::scaleMask(ScaleType::major, 0) != 0b101010110101u) return 1;

    ScaleMapper mapper;
    mapper.reset();
    const float target = mapper.map(61.8f, true, p); // C# should map to D in C minor.
    if (std::abs(target - 62.0f) > 0.01f) {
        std::cerr << "Scale mapping failed: " << target << "\n";
        return 2;
    }

    std::bitset<128> midi;
    midi.set(67);
    mapper.setMidiNotes(midi);
    const float midiTarget = mapper.map(66.7f, true, p);
    if (std::abs(midiTarget - 67.0f) > 0.01f) {
        std::cerr << "MIDI mapping failed: " << midiTarget << "\n";
        return 3;
    }

    CorrectionTrajectory trajectory;
    trajectory.prepare(48000.0);
    TrackedPitch tracked;
    tracked.hz = midiToHz(66.5f);
    tracked.midi = 66.5f;
    tracked.voicing = 1.0f;
    tracked.confidence = 1.0f;
    tracked.state = PitchState::voicedStable;
    trajectory.setFrame(tracked, 67.0f, p);
    std::vector<float> ratio(512, 1.0f), mask(512, 0.0f);
    trajectory.render(ratio.data(), mask.data(), static_cast<int>(ratio.size()), p);
    for (std::size_t i = 0; i < ratio.size(); ++i) {
        if (!std::isfinite(ratio[i]) || !std::isfinite(mask[i])) return 4;
    }
    if (ratio.back() <= 1.0f || mask.back() < 0.5f) return 5;

    std::cout << "core_smoke=PASS target=" << target
              << " midiTarget=" << midiTarget
              << " finalRatio=" << ratio.back() << "\n";
    return 0;
}
