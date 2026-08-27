#include "engine/SonRapTuneEngine.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {
namespace {

float smoothingAlpha(double sampleRate, double seconds) noexcept
{
    return 1.0f - std::exp(-1.0f
        / static_cast<float>(std::max(1.0, sampleRate * seconds)));
}

} // namespace

void SonRapTuneEngine::prepare(const PrepareSpec& spec)
{
    spec_ = spec;
    spec_.sampleRate = std::max(8000.0, spec_.sampleRate);
    spec_.maxBlockSize = std::max(16, spec_.maxBlockSize);
    spec_.channels = std::clamp(spec_.channels, 1, 2);
    analysisMono_.assign(static_cast<std::size_t>(spec_.maxBlockSize), 0.0f);
    ratio_.assign(static_cast<std::size_t>(spec_.maxBlockSize), 1.0f);
    voicedMask_.assign(static_cast<std::size_t>(spec_.maxBlockSize), 0.0f);
    detector_.prepare(spec_.sampleRate);
    songKeyEstimator_.prepare(spec_.sampleRate);
    trajectory_.prepare(spec_.sampleRate);
    consonantProtection_.prepare(spec_.sampleRate);
    shifter_.prepare(spec_.sampleRate, spec_.maxBlockSize, spec_.channels);
    mixSmoothingAlpha_ = smoothingAlpha(spec_.sampleRate, 0.005);
    gainSmoothingAlpha_ = smoothingAlpha(spec_.sampleRate, 0.005);
    reset();
}

void SonRapTuneEngine::reset() noexcept
{
    detector_.reset();
    tracker_.reset();
    songKeyEstimator_.reset();
    mapper_.reset();
    trajectory_.reset();
    consonantProtection_.reset();
    shifter_.reset();
    latestFrame_ = {};
    resolvedKey_ = std::clamp(parameters_.key, 0, 11);
    resolvedScale_ = parameters_.scale;
    resolvedKeyInitialized_ = false;
    smoothedMix_ = parameters_.bypass
        ? 0.0f
        : std::clamp(parameters_.mix, 0.0f, 1.0f);
    smoothedGain_ = std::pow(10.0f, parameters_.outputTrimDb / 20.0f);
    std::fill(analysisMono_.begin(), analysisMono_.end(), 0.0f);
    std::fill(ratio_.begin(), ratio_.end(), 1.0f);
    std::fill(voicedMask_.begin(), voicedMask_.end(), 0.0f);
}

void SonRapTuneEngine::process(float* const* channels, int numChannels, int numSamples,
                              const std::bitset<128>& activeMidiNotes) noexcept
{
    if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
        return;
    numSamples = std::min(numSamples, spec_.maxBlockSize);

    for (int i = 0; i < numSamples; ++i) {
        float mono = channels[0] != nullptr ? channels[0][i] : 0.0f;
        if (numChannels > 1 && channels[1] != nullptr)
            mono = 0.5f * (mono + channels[1][i]);
        analysisMono_[static_cast<std::size_t>(i)] = mono;
    }

    mapper_.setMidiNotes(activeMidiNotes);
    PitchAnalysis analysis;
    if (detector_.push(analysisMono_.data(), numSamples, parameters_.vocalRange, analysis)) {
        const auto tracked = tracker_.update(analysis, parameters_.stability);
        const auto keyEstimate = songKeyEstimator_.update(tracked, analysis.sampleTime);

        RuntimeParameters effectiveParameters = parameters_;
        effectiveParameters.key = std::clamp(parameters_.key, 0, 11);
        if (parameters_.autoKey && keyEstimate.ready) {
            effectiveParameters.key = keyEstimate.key;
            effectiveParameters.scale = keyEstimate.scale;
        }

        if (!resolvedKeyInitialized_
            || effectiveParameters.key != resolvedKey_
            || effectiveParameters.scale != resolvedScale_) {
            resolvedKey_ = effectiveParameters.key;
            resolvedScale_ = effectiveParameters.scale;
            resolvedKeyInitialized_ = true;
            mapper_.reset();
            mapper_.setMidiNotes(activeMidiNotes);
        }

        const float target = mapper_.map(tracked.midi, analysis.onset, effectiveParameters);
        trajectory_.setFrame(tracked, target, effectiveParameters);
        latestFrame_.sampleTime = analysis.sampleTime;
        latestFrame_.detectedHz = tracked.hz;
        latestFrame_.detectedMidi = tracked.midi;
        latestFrame_.confidence = tracked.confidence;
        latestFrame_.voicing = tracked.voicing;
        latestFrame_.targetMidi = target;
        latestFrame_.correctionCents = trajectory_.correctionCents();
        latestFrame_.state = tracked.state;
        latestFrame_.resolvedKey = resolvedKey_;
        latestFrame_.resolvedScale = resolvedScale_;
        latestFrame_.keyConfidence = keyEstimate.confidence;
        latestFrame_.keyEstimateReady = keyEstimate.ready;
        latestFrame_.autoKeyActive = parameters_.autoKey;
    }

    trajectory_.render(ratio_.data(), voicedMask_.data(), numSamples, parameters_);

    consonantProtection_.process(analysisMono_.data(),
                                 voicedMask_.data(),
                                 numSamples,
                                 parameters_.consonantProtect,
                                 latestFrame_);

    const float targetMix = parameters_.bypass
        ? 0.0f
        : std::clamp(parameters_.mix, 0.0f, 1.0f);
    for (int i = 0; i < numSamples; ++i) {
        smoothedMix_ += mixSmoothingAlpha_ * (targetMix - smoothedMix_);
        voicedMask_[static_cast<std::size_t>(i)] *= smoothedMix_;
    }

    shifter_.setSourcePitch(latestFrame_.detectedHz,
                            latestFrame_.confidence,
                            latestFrame_.voicing);
    shifter_.setFormantPreserve(parameters_.formantPreserve);
    shifter_.process(channels, numChannels, numSamples,
                     ratio_.data(), voicedMask_.data());

    const float targetGain = std::pow(10.0f, parameters_.outputTrimDb / 20.0f);
    for (int i = 0; i < numSamples; ++i) {
        smoothedGain_ += gainSmoothingAlpha_ * (targetGain - smoothedGain_);
        for (int ch = 0; ch < numChannels; ++ch) {
            if (channels[ch] != nullptr)
                channels[ch][i] *= smoothedGain_;
        }
    }
}

} // namespace sonraptune
