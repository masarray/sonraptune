#pragma once

#include "engine/CommonTypes.h"
#include <bitset>

namespace sonraptune {

class ScaleMapper {
public:
    void reset() noexcept;
    void setMidiNotes(const std::bitset<128>& notes) noexcept { midiNotes_ = notes; }

    float map(float detectedMidi, bool onset, const RuntimeParameters& p) noexcept;
    static std::uint16_t scaleMask(ScaleType type, std::uint16_t customMask) noexcept;

private:
    bool noteAllowed(int midiNote, int key, std::uint16_t mask) const noexcept;
    int nearestAllowed(int roundedMidi, int key, std::uint16_t mask) const noexcept;
    int nearestMidiTarget(float detectedMidi) const noexcept;

    std::bitset<128> midiNotes_{};
    float currentTarget_ = -1.0f;
    int committedFrames_ = 0;
};

} // namespace sonraptune
