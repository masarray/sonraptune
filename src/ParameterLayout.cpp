#include "src/ParameterLayout.h"

namespace sonraptune {

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using I = juce::AudioParameterInt;
    juce::AudioProcessorValueTreeState::ParameterLayout p;

    p.add(std::make_unique<C>(juce::ParameterID{ParamIds::mode, 1}, "Mode",
        juce::StringArray{"Natural", "Modern Rap", "Trap Lock", "Hook Debug"}, 1));
    p.add(std::make_unique<C>(juce::ParameterID{ParamIds::key, 1}, "Key",
        juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));
    p.add(std::make_unique<C>(juce::ParameterID{ParamIds::scale, 1}, "Scale",
        juce::StringArray{"Major", "Natural Minor", "Chromatic", "Custom"}, 1));
    p.add(std::make_unique<I>(juce::ParameterID{ParamIds::customMask, 1},
        "Custom Scale Mask", 1, 4095, 4095));
    p.add(std::make_unique<B>(juce::ParameterID{ParamIds::autoKey, 1},
        "Auto Key", false));
    p.add(std::make_unique<C>(juce::ParameterID{ParamIds::vocalRange, 1}, "Vocal Range",
        juce::StringArray{"Auto", "Low", "Mid", "High"}, 0));

    const auto percent = juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f);
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::tune, 1}, "Tune", percent, 85.0f));
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::speed, 1}, "Speed",
        juce::NormalisableRange<float>(0.5f, 200.0f, 0.1f, 0.35f), 18.0f));
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::stability, 1}, "Stability", percent, 65.0f));
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::feel, 1}, "Feel", percent, 30.0f));
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::formant, 1}, "Formant Preserve", percent, 100.0f));
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::consonant, 1}, "Consonant Protect", percent, 80.0f));
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::mix, 1}, "Mix", percent, 100.0f));
    p.add(std::make_unique<F>(juce::ParameterID{ParamIds::outputTrim, 1}, "Output Trim",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.05f), 0.0f));
    p.add(std::make_unique<B>(juce::ParameterID{ParamIds::bypass, 1}, "Bypass", false));
    return p;
}

} // namespace sonraptune
