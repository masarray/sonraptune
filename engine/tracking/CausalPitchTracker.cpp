#include "engine/tracking/CausalPitchTracker.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {

void CausalPitchTracker::reset() noexcept
{
    beams_.fill({});
    initialized_ = false;
    stableFrames_ = 0;
    unvoicedFrames_ = 0;
}

TrackedPitch CausalPitchTracker::update(const PitchAnalysis& analysis, float stability) noexcept
{
    stability = std::clamp(stability, 0.0f, 1.0f);
    std::array<Beam, 5> observations{};
    int obsCount = 0;
    for (int i = 0; i < analysis.candidateCount && i < 4; ++i) {
        const auto& c = analysis.candidates[static_cast<std::size_t>(i)];
        observations[static_cast<std::size_t>(obsCount++)] = {c.hz, c.confidence,
            -std::log(std::max(1.0e-4f, c.confidence)), true};
    }
    observations[static_cast<std::size_t>(obsCount++)] = {0.0f, 1.0f - analysis.voicing,
        -std::log(std::max(1.0e-4f, 1.0f - analysis.voicing)), false};

    std::array<Beam, 4> next{};
    for (auto& b : next) b.cost = 1.0e9f;

    for (int oi = 0; oi < obsCount; ++oi) {
        const auto& obs = observations[static_cast<std::size_t>(oi)];
        float bestCost = obs.cost;
        if (initialized_) {
            bestCost = 1.0e9f;
            for (const auto& prev : beams_) {
                float transition = 0.0f;
                if (prev.voiced && obs.voiced) {
                    const float jump = std::abs(centsBetween(obs.hz, prev.hz));
                    transition += jump / (220.0f - 120.0f * stability);
                    if (jump > 700.0f && !analysis.onset)
                        transition += 4.0f + 5.0f * stability;
                } else if (prev.voiced != obs.voiced) {
                    transition += 0.7f + 1.2f * stability;
                }
                bestCost = std::min(bestCost, prev.cost * 0.72f + obs.cost + transition);
            }
        }

        Beam candidate = obs;
        candidate.cost = bestCost;
        for (int slot = 0; slot < 4; ++slot) {
            if (candidate.cost < next[static_cast<std::size_t>(slot)].cost) {
                for (int j = 3; j > slot; --j)
                    next[static_cast<std::size_t>(j)] = next[static_cast<std::size_t>(j - 1)];
                next[static_cast<std::size_t>(slot)] = candidate;
                break;
            }
        }
    }

    beams_ = next;
    initialized_ = true;
    const auto& winner = beams_[0];

    TrackedPitch result;
    result.voicing = analysis.voicing;
    result.confidence = winner.voiced ? winner.confidence : 1.0f - analysis.voicing;
    if (winner.voiced && analysis.voicing > 0.25f) {
        result.hz = winner.hz;
        result.midi = hzToMidi(winner.hz);
        ++stableFrames_;
        unvoicedFrames_ = 0;
        if (analysis.onset) result.state = PitchState::onset;
        else result.state = stableFrames_ >= 3 ? PitchState::voicedStable : PitchState::voicedUnstable;
    } else {
        result.hz = 0.0f;
        result.midi = -1.0f;
        stableFrames_ = 0;
        ++unvoicedFrames_;
        result.state = analysis.rms < 0.0015f ? PitchState::silence
                     : (unvoicedFrames_ > 4 ? PitchState::phraseRelease : PitchState::unvoiced);
    }
    return result;
}

} // namespace sonraptune
