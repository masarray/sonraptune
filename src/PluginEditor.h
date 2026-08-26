#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "src/PluginProcessor.h"
#include <array>
#include <memory>

namespace sonraptune {

class SonRapLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    SonRapLookAndFeel();

    void drawRotarySlider(juce::Graphics&,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider&) override;

    void drawComboBox(juce::Graphics&,
                      int width,
                      int height,
                      bool isButtonDown,
                      int buttonX,
                      int buttonY,
                      int buttonW,
                      int buttonH,
                      juce::ComboBox&) override;

    void drawButtonBackground(juce::Graphics&,
                              juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

private:
    juce::Colour accent_{0xff67d9ff};
    juce::Colour accent2_{0xff9b78ff};
};

class LabeledCombo final : public juce::Component {
public:
    explicit LabeledCombo(const juce::String& title);
    juce::ComboBox& box() noexcept { return box_; }
    void resized() override;

private:
    juce::Label label_;
    juce::ComboBox box_;
};

class MacroKnob final : public juce::Component {
public:
    explicit MacroKnob(const juce::String& title);
    juce::Slider& slider() noexcept { return slider_; }
    void resized() override;

private:
    juce::Label label_;
    juce::Slider slider_;
};

class PitchFocusDisplay final : public juce::Component {
public:
    void setFrame(const PitchFrame& frame);
    void paint(juce::Graphics&) override;

private:
    static juce::String midiName(float midi);
    PitchFrame frame_{};
    bool hasFrame_ = false;
};

class ScaleStrip final : public juce::Component {
public:
    void setScale(int key, int scale);
    void paint(juce::Graphics&) override;

private:
    static bool isScaleTone(int pitchClass, int key, int scale) noexcept;
    int key_ = 0;
    int scale_ = 1;
};

class PluginEditor final : public juce::AudioProcessorEditor,
                           private juce::Timer {
public:
    explicit PluginEditor(PluginProcessor& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void configurePercentKnob(MacroKnob& knob, const juce::String& tooltip);
    void configureCombo(LabeledCombo& combo, const juce::String& tooltip);

    PluginProcessor& processor_;
    SonRapLookAndFeel lookAndFeel_;

    LabeledCombo vocalRange_{"INPUT"};
    LabeledCombo key_{"KEY"};
    LabeledCombo scale_{"SCALE"};
    LabeledCombo mode_{"STYLE"};

    MacroKnob speed_{"SPEED"};
    MacroKnob feel_{"FEEL"};
    MacroKnob tune_{"TUNE"};
    MacroKnob stability_{"STABILITY"};
    MacroKnob formant_{"FORMANT"};
    MacroKnob consonant_{"CONSONANT"};
    MacroKnob mix_{"MIX"};
    MacroKnob output_{"OUTPUT"};

    PitchFocusDisplay pitchDisplay_;
    ScaleStrip scaleStrip_;
    juce::TextButton bypass_{"BYPASS"};
    juce::TooltipWindow tooltip_;

    std::unique_ptr<ComboAttachment> vocalRangeAttachment_;
    std::unique_ptr<ComboAttachment> keyAttachment_;
    std::unique_ptr<ComboAttachment> scaleAttachment_;
    std::unique_ptr<ComboAttachment> modeAttachment_;

    std::unique_ptr<SliderAttachment> speedAttachment_;
    std::unique_ptr<SliderAttachment> feelAttachment_;
    std::unique_ptr<SliderAttachment> tuneAttachment_;
    std::unique_ptr<SliderAttachment> stabilityAttachment_;
    std::unique_ptr<SliderAttachment> formantAttachment_;
    std::unique_ptr<SliderAttachment> consonantAttachment_;
    std::unique_ptr<SliderAttachment> mixAttachment_;
    std::unique_ptr<SliderAttachment> outputAttachment_;
    std::unique_ptr<ButtonAttachment> bypassAttachment_;
};

} // namespace sonraptune
