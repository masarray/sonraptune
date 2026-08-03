#include "engine/analysis/YinCandidateDetector.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sonraptune {

void YinCandidateDetector::prepare(double inputSampleRate)
{
    inputRate_ = std::max(8000.0, inputSampleRate);
    decimation_ = std::max(1, static_cast<int>(std::lround(inputRate_ / 12000.0)));
    detectorRate_ = inputRate_ / static_cast<double>(decimation_);
    reset();
}

void YinCandidateDetector::reset() noexcept
{
    history_.fill(0.0f);
    frame_.fill(0.0f);
    diff_.fill(0.0f);
    cmndf_.fill(1.0f);
    writeIndex_ = 0;
    available_ = 0;
    hopCounter_ = 0;
    decimationPhase_ = 0;
    decimationSum_ = 0.0f;
    inputSampleCounter_ = 0;
    previousRms_ = 0.0f;
}

std::pair<float, float> YinCandidateDetector::rangeHz(VocalRange range) const noexcept
{
    switch (range) {
        case VocalRange::low:  return {65.0f, 320.0f};
        case VocalRange::mid:  return {90.0f, 600.0f};
        case VocalRange::high: return {140.0f, 1000.0f};
        case VocalRange::automatic: default: return {65.0f, 1000.0f};
    }
}

void YinCandidateDetector::pushDecimated(float sample) noexcept
{
    history_[static_cast<std::size_t>(writeIndex_)] = sample;
    writeIndex_ = (writeIndex_ + 1) % kAnalysisSize;
    available_ = std::min(kAnalysisSize, available_ + 1);
    ++hopCounter_;
}

bool YinCandidateDetector::push(const float* input, int numSamples,
                                VocalRange range, PitchAnalysis& out) noexcept
{
    if (input == nullptr || numSamples <= 0)
        return false;

    bool produced = false;
    for (int i = 0; i < numSamples; ++i) {
        decimationSum_ += input[i];
        ++decimationPhase_;
        ++inputSampleCounter_;
        if (decimationPhase_ >= decimation_) {
            pushDecimated(decimationSum_ / static_cast<float>(decimation_));
            decimationSum_ = 0.0f;
            decimationPhase_ = 0;
            if (available_ == kAnalysisSize && hopCounter_ >= kHopSize) {
                hopCounter_ = 0;
                produced = analyze(range, out) || produced;
            }
        }
    }
    return produced;
}

bool YinCandidateDetector::analyze(VocalRange range, PitchAnalysis& out) noexcept
{
    for (int i = 0; i < kAnalysisSize; ++i)
        frame_[static_cast<std::size_t>(i)] = history_[static_cast<std::size_t>((writeIndex_ + i) % kAnalysisSize)];

    double power = 0.0;
    for (float x : frame_) power += static_cast<double>(x) * x;
    const float rms = static_cast<float>(std::sqrt(power / static_cast<double>(kAnalysisSize)));

    const auto [minHz, maxHz] = rangeHz(range);
    int tauMin = std::max(2, static_cast<int>(std::floor(detectorRate_ / maxHz)));
    int tauMax = std::min(kMaxTau, static_cast<int>(std::ceil(detectorRate_ / minHz)));
    if (tauMax <= tauMin + 2) return false;

    diff_.fill(0.0f);
    cmndf_.fill(1.0f);

    for (int tau = tauMin; tau <= tauMax; ++tau) {
        double sum = 0.0;
        const int count = kAnalysisSize - tau;
        for (int i = 0; i < count; ++i) {
            const float d = frame_[static_cast<std::size_t>(i)] - frame_[static_cast<std::size_t>(i + tau)];
            sum += static_cast<double>(d) * d;
        }
        diff_[static_cast<std::size_t>(tau)] = static_cast<float>(sum / std::max(1, count));
    }

    double running = 0.0;
    for (int tau = tauMin; tau <= tauMax; ++tau) {
        running += diff_[static_cast<std::size_t>(tau)];
        const int n = tau - tauMin + 1;
        cmndf_[static_cast<std::size_t>(tau)] = running > 1.0e-18
            ? diff_[static_cast<std::size_t>(tau)] * static_cast<float>(n) / static_cast<float>(running)
            : 1.0f;
    }

    struct Minima { int tau; float value; float score; };
    std::array<Minima, 16> minima{};
    int minimaCount = 0;
    int firstReliableTau = 0;

    for (int tau = tauMin + 1; tau < tauMax; ++tau) {
        const float v = cmndf_[static_cast<std::size_t>(tau)];
        if (v > 0.45f) continue;
        if (v <= cmndf_[static_cast<std::size_t>(tau - 1)] &&
            v < cmndf_[static_cast<std::size_t>(tau + 1)]) {
            if (firstReliableTau == 0 && v < 0.25f)
                firstReliableTau = tau;
            if (minimaCount < static_cast<int>(minima.size()))
                minima[static_cast<std::size_t>(minimaCount++)] = {tau, v, 0.0f};
        }
    }

    if (firstReliableTau == 0 && minimaCount > 0)
        firstReliableTau = minima[0].tau;

    std::array<Minima, kMaxPitchCandidates> best{};
    for (auto& b : best) b = {0, 2.0f, -1.0f};
    for (int i = 0; i < minimaCount; ++i) {
        auto m = minima[static_cast<std::size_t>(i)];
        const float periodRatio = firstReliableTau > 0
            ? static_cast<float>(m.tau) / static_cast<float>(firstReliableTau) : 1.0f;
        // YIN's first reliable dip is usually the fundamental. Deeper dips at
        // 2T/3T are valid periodic repetitions but must not outrank T.
        const float subharmonicPenalty = periodRatio > 1.35f
            ? 1.0f / (1.0f + 0.85f * (periodRatio - 1.0f)) : 1.0f;
        m.score = (1.0f - m.value) * subharmonicPenalty;
        for (int slot = 0; slot < kMaxPitchCandidates; ++slot) {
            if (m.score > best[static_cast<std::size_t>(slot)].score) {
                for (int j = kMaxPitchCandidates - 1; j > slot; --j)
                    best[static_cast<std::size_t>(j)] = best[static_cast<std::size_t>(j - 1)];
                best[static_cast<std::size_t>(slot)] = m;
                break;
            }
        }
    }

    out = {};
    out.rms = rms;
    out.sampleTime = inputSampleCounter_;
    out.onset = rms > previousRms_ * 1.8f && rms > 0.01f;
    previousRms_ = 0.90f * previousRms_ + 0.10f * rms;

    for (const auto& b : best) {
        if (b.tau <= 0) continue;
        const int tau = b.tau;
        const float ym1 = cmndf_[static_cast<std::size_t>(tau - 1)];
        const float y0  = cmndf_[static_cast<std::size_t>(tau)];
        const float yp1 = cmndf_[static_cast<std::size_t>(tau + 1)];
        const float denom = ym1 - 2.0f * y0 + yp1;
        float refined = static_cast<float>(tau);
        if (std::abs(denom) > 1.0e-8f)
            refined += 0.5f * (ym1 - yp1) / denom;
        refined = std::max(1.0f, refined);
        const float hz = static_cast<float>(detectorRate_) / refined;
        const float periodicity = std::clamp(b.score, 0.0f, 1.0f);
        const float energyConfidence = std::clamp((rms - 0.0015f) / 0.02f, 0.0f, 1.0f);
        auto& c = out.candidates[static_cast<std::size_t>(out.candidateCount++)];
        c.hz = hz;
        c.cmndf = b.value;
        c.confidence = periodicity * (0.35f + 0.65f * energyConfidence);
    }

    const float bestConfidence = out.candidateCount > 0 ? out.candidates[0].confidence : 0.0f;
    out.voicing = std::clamp(bestConfidence * std::clamp(rms / 0.012f, 0.0f, 1.0f), 0.0f, 1.0f);
    return true;
}

} // namespace sonraptune
