#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <cmath>

namespace sonraptune {

constexpr int kMaxPitchCandidates = 4;
constexpr float kA4Hz = 440.0f;

enum class PitchState : std::uint8_t {
    silence,
    unvoiced,
    voicedUnstable,
    voicedStable,
    onset,
    phraseRelease
};

enum class VocalRange : std::uint8_t { automatic, low, mid, high };
enum class ScaleType : std::uint8_t { major, naturalMinor, chromatic, customMask };

enum class ProductMode : std::uint8_t { natural, modernRap, trapLock, hookDebug };

struct PitchCandidate {
    float hz = 0.0f;
    float confidence = 0.0f;
    float cmndf = 1.0f;
};

struct PitchAnalysis {
    std::array<PitchCandidate, kMaxPitchCandidates> candidates{};
    int candidateCount = 0;
    float rms = 0.0f;
    float voicing = 0.0f;
    bool onset = false;
    std::int64_t sampleTime = 0;
};

struct TrackedPitch {
    float hz = 0.0f;
    float midi = -1.0f;
    float confidence = 0.0f;
    float voicing = 0.0f;
    PitchState state = PitchState::silence;
};

struct PitchFrame {
    std::int64_t sampleTime = 0;
    float detectedHz = 0.0f;
    float detectedMidi = -1.0f;
    float confidence = 0.0f;
    float voicing = 0.0f;
    float targetMidi = -1.0f;
    float correctionCents = 0.0f;
    PitchState state = PitchState::silence;
    int resolvedKey = 0;
    ScaleType resolvedScale = ScaleType::naturalMinor;
    float keyConfidence = 0.0f;
    bool keyEstimateReady = false;
    bool autoKeyActive = false;
};

struct RuntimeParameters {
    ProductMode mode = ProductMode::modernRap;
    int key = 0;
    ScaleType scale = ScaleType::naturalMinor;
    std::uint16_t customMask = 0x0FFFu;
    bool autoKey = false;
    VocalRange vocalRange = VocalRange::automatic;
    float tune = 0.85f;
    float speedMs = 18.0f;
    float stability = 0.65f;
    float feel = 0.30f;
    float formantPreserve = 1.0f;
    float consonantProtect = 0.80f;
    float mix = 1.0f;
    float outputTrimDb = 0.0f;
    bool bypass = false;
};

struct PrepareSpec {
    double sampleRate = 48000.0;
    int maxBlockSize = 512;
    int channels = 1;
};

inline float hzToMidi(float hz) noexcept {
    return hz > 0.0f ? 69.0f + 12.0f * std::log2(hz / kA4Hz) : -1.0f;
}

inline float midiToHz(float midi) noexcept {
    return kA4Hz * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

inline float centsBetween(float aHz, float bHz) noexcept {
    if (aHz <= 0.0f || bHz <= 0.0f) return 0.0f;
    return 1200.0f * std::log2(aHz / bHz);
}

} // namespace sonraptune
