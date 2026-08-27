#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace sonraptune {

namespace ParamIds {
inline constexpr auto mode = "mode";
inline constexpr auto key = "key";
inline constexpr auto scale = "scale";
inline constexpr auto customMask = "custom_scale_mask";
inline constexpr auto autoKey = "auto_key";
inline constexpr auto vocalRange = "vocal_range";
inline constexpr auto tune = "tune";
inline constexpr auto speed = "speed_ms";
inline constexpr auto stability = "stability";
inline constexpr auto feel = "feel";
inline constexpr auto formant = "formant_preserve";
inline constexpr auto consonant = "consonant_protect";
inline constexpr auto mix = "mix";
inline constexpr auto outputTrim = "output_trim_db";
inline constexpr auto bypass = "bypass";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace sonraptune
