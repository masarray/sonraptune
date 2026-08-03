#include "engine/SonRapTuneEngine.h"

#include <algorithm>
#include <cmath>

namespace sonraptune {

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
    trajectory_.prepare(spec_.sampleRate);
    shifter_.prepare(spec_.sampleRate, spec_.maxBlockSize, spec_.channels);
    reset();
}

void SonRapTuneEngine::reset() noexcept
{
    detector_.reset();
    tracker_.reset();
    mapper_.reset();
    trajectory_.reset();
    shifter_.reset();
    latestFrame_ = {};
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
        const float target = mapper_.map(tracked.midi, analysis.onset, parameters_);
        trajectory_.setFrame(tracked, target, parameters_);
        latestFrame_.sampleTime = analysis.sampleTime;
        latestFrame_.detectedHz = tracked.hz;
        latestFrame_.detectedMidi = tracked.midi;
        latestFrame_.confidence = tracked.confidence;
        latestFrame_.voicing = tracked.voicing;
        latestFrame_.targetMidi = target;
        latestFrame_.correctionCents = trajectory_.correctionCents();
        latestFrame_.state = tracked.state;
    }

    trajectory_.render(ratio_.data(), voicedMask_.data(), numSamples, parameters_);

    const float userMix = parameters_.bypass
        ? 0.0f
        : std::clamp(parameters_.mix, 0.0f, 1.0f);
    for (int i = 0; i < numSamples; ++i)
        voicedMask_[static_cast<std::size_t>(i)] *= userMix;

    // The shifter always runs, including internal bypass, so dry and wet paths
    // remain aligned to the latency reported to the host.
    shifter_.process(channels, numChannels, numSamples,
                     ratio_.data(), voicedMask_.data());

    const float gain = std::pow(10.0f, parameters_.outputTrimDb / 20.0f);
    for (int ch = 0; ch < numChannels; ++ch) {
        if (channels[ch] == nullptr)
            continue;
        for (int i = 0; i < numSamples; ++i)
            channels[ch][i] *= gain;
    }
}

} // namespace sonraptune
