#pragma once

#include "engine/CommonTypes.h"
#include "engine/analysis/YinCandidateDetector.h"
#include "engine/tracking/CausalPitchTracker.h"
#include "engine/mapping/ScaleMapper.h"
#include "engine/correction/CorrectionTrajectory.h"

#include <array>
#include <bitset>
#include <vector>

namespace sonraptune {

class SonRapTuneEngine {
public:
    void prepare(const PrepareSpec& spec);
    void reset() noexcept;
    void setParameters(const RuntimeParameters& p) noexcept { parameters_ = p; }

    // P0 analysis alpha: audio remains unchanged until a shifter candidate passes bake-off.
    void process(float* const* channels, int numChannels, int numSamples,
                 const std::bitset<128>& activeMidiNotes) noexcept;

    int latencySamples() const noexcept { return 0; }
    const PitchFrame& latestFrame() const noexcept { return latestFrame_; }

private:
    PrepareSpec spec_{};
    RuntimeParameters parameters_{};
    YinCandidateDetector detector_{};
    CausalPitchTracker tracker_{};
    ScaleMapper mapper_{};
    CorrectionTrajectory trajectory_{};
    PitchFrame latestFrame_{};
    std::vector<float> analysisMono_{};
    std::vector<float> ratio_{};
    std::vector<float> voicedMask_{};
};

} // namespace sonraptune
