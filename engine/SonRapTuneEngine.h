#pragma once

#include "engine/CommonTypes.h"
#include "engine/analysis/YinCandidateDetector.h"
#include "engine/tracking/CausalPitchTracker.h"
#include "engine/mapping/ScaleMapper.h"
#include "engine/correction/CorrectionTrajectory.h"
#include "engine/protection/ConsonantProtection.h"
#include "engine/shifting/PeriodSynchronousTimeDomainShifter.h"

#include <array>
#include <bitset>
#include <vector>

namespace sonraptune {

class SonRapTuneEngine {
public:
    void prepare(const PrepareSpec& spec);
    void reset() noexcept;
    void setParameters(const RuntimeParameters& p) noexcept { parameters_ = p; }

    void process(float* const* channels, int numChannels, int numSamples,
                 const std::bitset<128>& activeMidiNotes) noexcept;

    int latencySamples() const noexcept { return shifter_.latencySamples(); }
    const PitchFrame& latestFrame() const noexcept { return latestFrame_; }

private:
    PrepareSpec spec_{};
    RuntimeParameters parameters_{};
    YinCandidateDetector detector_{};
    CausalPitchTracker tracker_{};
    ScaleMapper mapper_{};
    CorrectionTrajectory trajectory_{};
    ConsonantProtection consonantProtection_{};
    PeriodSynchronousTimeDomainShifter shifter_{};
    PitchFrame latestFrame_{};
    std::vector<float> analysisMono_{};
    std::vector<float> ratio_{};
    std::vector<float> voicedMask_{};
};

} // namespace sonraptune
