#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "engine/SonRapTuneEngine.h"
#include "src/PitchTelemetryTap.h"
#include "src/ParameterLayout.h"
#include <bitset>

namespace sonraptune {

class PluginProcessor final : public juce::AudioProcessor {
public:
    PluginProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SonRapTune"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PitchTelemetryTap telemetry;

private:
    RuntimeParameters snapshotParameters() const noexcept;
    void applyMidi(const juce::MidiBuffer& midi) noexcept;

    SonRapTuneEngine engine_{};
    std::bitset<128> activeMidiNotes_{};
    std::int64_t lastTelemetrySample_ = -1;
};

} // namespace sonraptune
