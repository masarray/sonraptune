#include "src/PluginProcessor.h"

namespace sonraptune {

PluginProcessor::PluginProcessor()
    : juce::AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SONRAPTUNE_STATE", createParameterLayout())
{
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in == juce::AudioChannelSet::mono())
        return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
    return in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::stereo();
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    PrepareSpec spec;
    spec.sampleRate = sampleRate;
    spec.maxBlockSize = samplesPerBlock;
    spec.channels = juce::jlimit(1, 2, getTotalNumOutputChannels());
    engine_.prepare(spec);
    setLatencySamples(engine_.latencySamples());
    activeMidiNotes_.reset();
    lastTelemetrySample_ = -1;
}

RuntimeParameters PluginProcessor::snapshotParameters() const noexcept
{
    RuntimeParameters p;
    p.mode = static_cast<ProductMode>(juce::jlimit(0, 3,
        static_cast<int>(apvts.getRawParameterValue(ParamIds::mode)->load())));
    p.key = juce::jlimit(0, 11, static_cast<int>(apvts.getRawParameterValue(ParamIds::key)->load()));
    p.scale = static_cast<ScaleType>(juce::jlimit(0, 3,
        static_cast<int>(apvts.getRawParameterValue(ParamIds::scale)->load())));
    p.vocalRange = static_cast<VocalRange>(juce::jlimit(0, 3,
        static_cast<int>(apvts.getRawParameterValue(ParamIds::vocalRange)->load())));
    p.tune = apvts.getRawParameterValue(ParamIds::tune)->load() * 0.01f;
    p.speedMs = apvts.getRawParameterValue(ParamIds::speed)->load();
    p.stability = apvts.getRawParameterValue(ParamIds::stability)->load() * 0.01f;
    p.feel = apvts.getRawParameterValue(ParamIds::feel)->load() * 0.01f;
    p.formantPreserve = apvts.getRawParameterValue(ParamIds::formant)->load() * 0.01f;
    p.consonantProtect = apvts.getRawParameterValue(ParamIds::consonant)->load() * 0.01f;
    p.mix = apvts.getRawParameterValue(ParamIds::mix)->load() * 0.01f;
    p.outputTrimDb = apvts.getRawParameterValue(ParamIds::outputTrim)->load();
    p.bypass = apvts.getRawParameterValue(ParamIds::bypass)->load() > 0.5f;
    return p;
}

void PluginProcessor::applyMidi(const juce::MidiBuffer& midi) noexcept
{
    for (const auto metadata : midi) {
        const auto m = metadata.getMessage();
        if (m.isNoteOn()) activeMidiNotes_.set(static_cast<std::size_t>(m.getNoteNumber()));
        else if (m.isNoteOff()) activeMidiNotes_.reset(static_cast<std::size_t>(m.getNoteNumber()));
        else if (m.isAllNotesOff() || m.isAllSoundOff()) activeMidiNotes_.reset();
    }
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalIn = getTotalNumInputChannels();
    const auto totalOut = getTotalNumOutputChannels();
    for (int ch = totalIn; ch < totalOut; ++ch) buffer.clear(ch, 0, buffer.getNumSamples());

    applyMidi(midi);
    engine_.setParameters(snapshotParameters());
    std::array<float*, 2> channels{};
    const int channelCount = juce::jmin(2, buffer.getNumChannels());
    for (int ch = 0; ch < channelCount; ++ch) channels[static_cast<std::size_t>(ch)] = buffer.getWritePointer(ch);
    engine_.process(channels.data(), channelCount, buffer.getNumSamples(), activeMidiNotes_);

    const auto& frame = engine_.latestFrame();
    if (frame.sampleTime != lastTelemetrySample_) {
        telemetry.push(frame);
        lastTelemetrySample_ = frame.sampleTime;
    }
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

} // namespace sonraptune

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new sonraptune::PluginProcessor();
}
