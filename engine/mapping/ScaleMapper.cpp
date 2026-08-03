#include "engine/mapping/ScaleMapper.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {

void ScaleMapper::reset() noexcept
{
    currentTarget_ = -1.0f;
    committedFrames_ = 0;
    midiNotes_.reset();
}

std::uint16_t ScaleMapper::scaleMask(ScaleType type, std::uint16_t customMask) noexcept
{
    switch (type) {
        case ScaleType::major:        return 0b101010110101u; // 0,2,4,5,7,9,11
        case ScaleType::naturalMinor: return 0b010110101101u; // 0,2,3,5,7,8,10
        case ScaleType::chromatic:    return 0x0FFFu;
        case ScaleType::customMask:   return customMask & 0x0FFFu;
    }
    return 0x0FFFu;
}

bool ScaleMapper::noteAllowed(int midiNote, int key, std::uint16_t mask) const noexcept
{
    const int pitchClass = ((midiNote - key) % 12 + 12) % 12;
    return (mask & (1u << pitchClass)) != 0;
}

int ScaleMapper::nearestAllowed(int roundedMidi, int key, std::uint16_t mask) const noexcept
{
    for (int distance = 0; distance <= 12; ++distance) {
        const int down = roundedMidi - distance;
        const int up = roundedMidi + distance;
        if (noteAllowed(down, key, mask)) return down;
        if (distance > 0 && noteAllowed(up, key, mask)) return up;
    }
    return roundedMidi;
}

int ScaleMapper::nearestMidiTarget(float detectedMidi) const noexcept
{
    if (midiNotes_.none()) return -1;
    int best = -1;
    float bestDistance = 1.0e9f;
    for (int note = 0; note < 128; ++note) {
        if (!midiNotes_.test(static_cast<std::size_t>(note))) continue;
        const float d = std::abs(detectedMidi - static_cast<float>(note));
        if (d < bestDistance) { bestDistance = d; best = note; }
    }
    return best;
}

float ScaleMapper::map(float detectedMidi, bool onset, const RuntimeParameters& p) noexcept
{
    if (detectedMidi < 0.0f) return currentTarget_;

    int proposed = nearestMidiTarget(detectedMidi);
    if (proposed < 0) {
        const auto mask = scaleMask(p.scale, p.customMask);
        proposed = nearestAllowed(static_cast<int>(std::lround(detectedMidi)), p.key, mask);
    }

    if (currentTarget_ < 0.0f) {
        currentTarget_ = static_cast<float>(proposed);
        committedFrames_ = 1;
        return currentTarget_;
    }

    const float boundaryMargin = 0.05f + 0.35f * std::clamp(p.stability, 0.0f, 1.0f);
    const int minimumCommit = 1 + static_cast<int>(std::lround(5.0f * p.stability));
    const float distanceToCurrent = std::abs(detectedMidi - currentTarget_);
    const float distanceToProposed = std::abs(detectedMidi - static_cast<float>(proposed));
    const bool decisivelyCloser = distanceToProposed + boundaryMargin < distanceToCurrent;

    if (proposed != static_cast<int>(std::lround(currentTarget_)) &&
        (onset || (committedFrames_ >= minimumCommit && decisivelyCloser))) {
        currentTarget_ = static_cast<float>(proposed);
        committedFrames_ = 1;
    } else {
        ++committedFrames_;
    }
    return currentTarget_;
}

} // namespace sonraptune
