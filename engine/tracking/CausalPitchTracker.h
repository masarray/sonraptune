#pragma once

#include "engine/CommonTypes.h"
#include <array>

namespace sonraptune {

class CausalPitchTracker {
public:
    void reset() noexcept;
    TrackedPitch update(const PitchAnalysis& analysis, float stability) noexcept;

private:
    struct Beam {
        float hz = 0.0f;
        float confidence = 0.0f;
        float cost = 1000.0f;
        bool voiced = false;
    };

    std::array<Beam, 4> beams_{};
    bool initialized_ = false;
    int stableFrames_ = 0;
    int unvoicedFrames_ = 0;
};

} // namespace sonraptune
