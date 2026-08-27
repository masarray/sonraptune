#include "src/PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace sonraptune {
namespace {

constexpr auto kPanel = 0xff151a22u;
constexpr auto kPanelRaised = 0xff1b222cu;
constexpr auto kText = 0xffeef5fau;
constexpr auto kMuted = 0xff7f8b99u;
constexpr auto kAccent = 0xff67d9ffu;
constexpr auto kAccent2 = 0xff9b78ffu;
constexpr auto kBorder = 0xff2d3743u;

juce::Colour colour(std::uint32_t argb) noexcept { return juce::Colour(argb); }

void addChoiceItems(juce::ComboBox& box, std::initializer_list<const char*> items)
{
    int id = 1;
    for (const auto* item : items)
        box.addItem(item, id++);
}

juce::String keyName(int key)
{
    static constexpr const char* names[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    return names[juce::jlimit(0, 11, key)];
}

juce::String scaleName(ScaleType scale)
{
    switch (scale) {
        case ScaleType::major: return "MAJ";
        case ScaleType::naturalMinor: return "MIN";
        case ScaleType::chromatic: return "CHR";
        case ScaleType::customMask: return "CUSTOM";
    }
    return "--";
}

} // namespace

SonRapLookAndFeel::SonRapLookAndFeel()
{
    setColour(juce::ComboBox::textColourId, colour(kText));
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::arrowColourId, accent_);
    setColour(juce::PopupMenu::backgroundColourId, colour(kPanel));
    setColour(juce::PopupMenu::textColourId, colour(kText));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, colour(0xff263545u));
    setColour(juce::PopupMenu::highlightedTextColourId, accent_);
    setColour(juce::Slider::textBoxTextColourId, colour(kText));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void SonRapLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                          int width, int height, float sliderPos,
                                          float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                                static_cast<float>(width), static_cast<float>(height)).reduced(8.0f);
    const auto radius = std::max(12.0f,
                                 std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f - 5.0f);
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                        rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(colour(0xff2a333eu));
    g.strokePath(track, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                           rotaryStartAngle, angle, true);
    g.setColour(slider.isEnabled() ? accent_ : colour(kMuted));
    g.strokePath(valueArc, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    const auto faceRadius = std::max(8.0f, radius - 10.0f);
    juce::ColourGradient face(colour(0xff252d37u), centre.x, centre.y - faceRadius,
                              colour(0xff11161du), centre.x, centre.y + faceRadius, false);
    g.setGradientFill(face);
    g.fillEllipse(centre.x - faceRadius, centre.y - faceRadius,
                  faceRadius * 2.0f, faceRadius * 2.0f);
    g.setColour(colour(0xff394552u));
    g.drawEllipse(centre.x - faceRadius, centre.y - faceRadius,
                  faceRadius * 2.0f, faceRadius * 2.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.5f, -faceRadius + 5.0f, 3.0f,
                                std::max(8.0f, faceRadius * 0.34f), 1.5f);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(colour(kText));
    g.fillPath(pointer);
}

void SonRapLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                      bool isButtonDown, int, int, int, int,
                                      juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width),
                                    static_cast<float>(height)).reduced(0.5f);
    g.setColour(colour(isButtonDown ? 0xff222d39u : kPanelRaised));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(box.hasKeyboardFocus(true) ? accent_.withAlpha(0.8f) : colour(kBorder));
    g.drawRoundedRectangle(r, 6.0f, 1.0f);

    juce::Path arrow;
    const auto cx = static_cast<float>(width) - 17.0f;
    const auto cy = static_cast<float>(height) * 0.5f;
    arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
    arrow.lineTo(cx, cy + 2.0f);
    arrow.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(accent_);
    g.strokePath(arrow, juce::PathStrokeType(1.7f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
}

void SonRapLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour&, bool isMouseOverButton,
                                              bool isButtonDown)
{
    auto r = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto active = button.getToggleState();
    auto fill = active ? accent2_.withAlpha(0.28f) : colour(kPanelRaised);
    if (isMouseOverButton) fill = fill.brighter(0.06f);
    if (isButtonDown) fill = fill.darker(0.10f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(active ? accent2_ : colour(kBorder));
    g.drawRoundedRectangle(r, 7.0f, active ? 1.5f : 1.0f);
}

juce::Font SonRapLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions(14.0f));
}

juce::Font SonRapLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions(std::min(14.0f,
                                                  static_cast<float>(buttonHeight) * 0.40f)));
}

LabeledCombo::LabeledCombo(const juce::String& title)
{
    label_.setText(title, juce::dontSendNotification);
    label_.setColour(juce::Label::textColourId, colour(kMuted));
    label_.setJustificationType(juce::Justification::centredLeft);
    label_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(label_);
    box_.setJustificationType(juce::Justification::centredLeft);
    box_.setColour(juce::ComboBox::textColourId, colour(kText));
    addAndMakeVisible(box_);
}

void LabeledCombo::resized()
{
    auto area = getLocalBounds();
    label_.setBounds(area.removeFromTop(20));
    box_.setBounds(area.reduced(0, 2));
}

MacroKnob::MacroKnob(const juce::String& title)
{
    label_.setText(title, juce::dontSendNotification);
    label_.setColour(juce::Label::textColourId, colour(kMuted));
    label_.setJustificationType(juce::Justification::centred);
    label_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(label_);

    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setRotaryParameters(juce::MathConstants<float>::pi * 1.20f,
                                juce::MathConstants<float>::pi * 2.80f, true);
    slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    slider_.setColour(juce::Slider::textBoxTextColourId, colour(kText));
    slider_.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider_.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider_);
}

void MacroKnob::resized()
{
    auto area = getLocalBounds();
    label_.setBounds(area.removeFromTop(20));
    slider_.setBounds(area);
}

void PitchFocusDisplay::setFrame(const PitchFrame& frame)
{
    frame_ = frame;
    hasFrame_ = true;
    repaint();
}

juce::String PitchFocusDisplay::midiName(float midi)
{
    if (midi < 0.0f) return "--";
    static constexpr const char* names[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    const auto note = static_cast<int>(std::lround(midi));
    const auto pitchClass = (note % 12 + 12) % 12;
    return juce::String(names[pitchClass]) + juce::String(note / 12 - 1);
}

void PitchFocusDisplay::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(colour(kPanel));
    g.fillRoundedRectangle(panel, 18.0f);
    g.setColour(colour(kBorder));
    g.drawRoundedRectangle(panel, 18.0f, 1.0f);

    auto header = panel.removeFromTop(32.0f).reduced(15.0f, 0.0f);
    g.setColour(colour(kMuted));
    g.setFont(11.0f);
    g.drawText("PITCH FOCUS", header.toNearestInt(), juce::Justification::centredLeft, false);

    if (hasFrame_ && frame_.autoKeyActive) {
        const auto autoText = frame_.keyEstimateReady
            ? "AUTO  " + keyName(frame_.resolvedKey) + " " + scaleName(frame_.resolvedScale)
                + "  " + juce::String(static_cast<int>(std::lround(frame_.keyConfidence * 100.0f))) + "%"
            : juce::String("AUTO  LEARNING");
        g.setColour(frame_.keyEstimateReady ? colour(kAccent2) : colour(kMuted));
        g.drawText(autoText, header.toNearestInt(), juce::Justification::centred, false);
    }

    const auto stateText = !hasFrame_ ? "WAITING"
        : frame_.state == PitchState::voicedStable ? "LOCKED"
        : frame_.state == PitchState::voicedUnstable ? "TRACKING"
        : frame_.state == PitchState::onset ? "ONSET"
        : frame_.state == PitchState::phraseRelease ? "RELEASE"
        : frame_.state == PitchState::unvoiced ? "UNVOICED" : "SILENCE";
    g.setColour(frame_.state == PitchState::voicedStable ? colour(kAccent) : colour(kMuted));
    g.drawText(stateText, header.toNearestInt(), juce::Justification::centredRight, false);

    auto body = panel.reduced(14.0f, 8.0f);
    const auto centre = juce::Point<float>(body.getCentreX(), body.getCentreY() + 6.0f);
    const auto radius = std::max(46.0f, std::min(body.getWidth(), body.getHeight()) * 0.34f);
    const auto start = juce::MathConstants<float>::pi * 0.74f;
    const auto end = juce::MathConstants<float>::pi * 2.26f;
    const auto mid = (start + end) * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, start, end, true);
    g.setColour(colour(0xff29323du));
    g.strokePath(backgroundArc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

    const auto cents = std::clamp(frame_.correctionCents, -100.0f, 100.0f);
    const auto currentAngle = start + ((cents + 100.0f) / 200.0f) * (end - start);
    juce::Path valueArc;
    valueArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f,
                           std::min(mid, currentAngle), std::max(mid, currentAngle), true);
    g.setColour(cents >= 0.0f ? colour(kAccent) : colour(kAccent2));
    g.strokePath(valueArc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    auto noteArea = juce::Rectangle<float>(centre.x - radius * 0.75f,
                                           centre.y - radius * 0.52f,
                                           radius * 1.50f, radius * 0.82f);
    g.setColour(colour(kText));
    g.setFont(std::max(28.0f, radius * 0.48f));
    g.drawText(midiName(frame_.detectedMidi), noteArea.toNearestInt(),
               juce::Justification::centred, false);

    auto targetArea = noteArea.translated(0.0f, radius * 0.52f);
    g.setFont(12.0f);
    g.setColour(colour(kMuted));
    g.drawText("TARGET  " + midiName(frame_.targetMidi), targetArea.toNearestInt(),
               juce::Justification::centred, false);

    auto centsArea = juce::Rectangle<float>(centre.x - 50.0f, centre.y + radius * 0.63f,
                                            100.0f, 22.0f);
    g.setColour(colour(kText));
    g.setFont(13.0f);
    g.drawText(juce::String(cents, 1) + " ct", centsArea.toNearestInt(),
               juce::Justification::centred, false);

    const auto confidence = std::clamp(frame_.confidence, 0.0f, 1.0f);
    auto confidenceArea = body.removeFromBottom(9.0f).reduced(18.0f, 0.0f);
    g.setColour(colour(0xff252e38u));
    g.fillRoundedRectangle(confidenceArea, 3.0f);
    auto confidenceFill = confidenceArea;
    confidenceFill.setWidth(confidenceArea.getWidth() * confidence);
    g.setColour(colour(kAccent).withAlpha(0.78f));
    g.fillRoundedRectangle(confidenceFill, 3.0f);
}

void ScaleStrip::setScale(int key, int scale, std::uint16_t customMask)
{
    key = juce::jlimit(0, 11, key);
    scale = juce::jlimit(0, 3, scale);
    customMask = static_cast<std::uint16_t>(customMask & 0x0FFFu) | 0x0001u;
    if (key_ == key && scale_ == scale && customMask_ == customMask) return;
    key_ = key;
    scale_ = scale;
    customMask_ = customMask;
    repaint();
}

bool ScaleStrip::isScaleTone(int pitchClass, int key, int scale,
                             std::uint16_t customMask) noexcept
{
    const auto relative = (pitchClass - key + 12) % 12;
    if (scale == 2) return true;
    if (scale == 3) return (customMask & (1u << relative)) != 0;
    static constexpr std::array<int, 7> major{0, 2, 4, 5, 7, 9, 11};
    static constexpr std::array<int, 7> minor{0, 2, 3, 5, 7, 8, 10};
    const auto& intervals = scale == 0 ? major : minor;
    return std::find(intervals.begin(), intervals.end(), relative) != intervals.end();
}

void ScaleStrip::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(colour(kPanel));
    g.fillRoundedRectangle(panel, 12.0f);
    g.setColour(colour(kBorder));
    g.drawRoundedRectangle(panel, 12.0f, 1.0f);

    auto title = panel.removeFromTop(23.0f).reduced(12.0f, 0.0f);
    g.setColour(colour(kMuted));
    g.setFont(10.5f);
    g.drawText("SCALE MAP", title.toNearestInt(), juce::Justification::centredLeft, false);
    g.drawText(scale_ == 3 ? "CUSTOM  /  CLICK NOTES" : "ROOT + ALLOWED NOTES",
               title.toNearestInt(), juce::Justification::centredRight, false);

    static constexpr const char* names[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    auto notes = panel.reduced(10.0f, 7.0f);
    const auto cellWidth = notes.getWidth() / 12.0f;
    for (int i = 0; i < 12; ++i) {
        auto cell = juce::Rectangle<float>(notes.getX() + cellWidth * static_cast<float>(i),
                                           notes.getY(), cellWidth - 4.0f, notes.getHeight());
        const auto active = isScaleTone(i, key_, scale_, customMask_);
        const auto root = i == key_;
        g.setColour(root ? colour(kAccent).withAlpha(0.30f)
                         : active ? colour(0xff222f3au) : colour(0xff10151bu));
        g.fillRoundedRectangle(cell, 5.0f);
        g.setColour(root ? colour(kAccent)
                         : active ? colour(0xff425263u) : colour(0xff222a33u));
        g.drawRoundedRectangle(cell, 5.0f, root ? 1.5f : 1.0f);
        g.setColour(root ? colour(kText)
                         : active ? colour(0xffbdcad5u) : colour(kMuted).withAlpha(0.65f));
        g.setFont(12.0f);
        g.drawText(names[i], cell.toNearestInt(), juce::Justification::centred, false);
    }
}

void ScaleStrip::mouseDown(const juce::MouseEvent& event)
{
    if (scale_ != 3) return;
    auto panel = getLocalBounds().toFloat().reduced(1.0f);
    panel.removeFromTop(23.0f);
    auto notes = panel.reduced(10.0f, 7.0f);
    if (!notes.contains(event.position)) return;
    const auto cellWidth = notes.getWidth() / 12.0f;
    const int pitchClass = juce::jlimit(0, 11,
        static_cast<int>((event.position.x - notes.getX()) / cellWidth));
    const int relative = (pitchClass - key_ + 12) % 12;
    if (relative == 0) return;
    customMask_ ^= static_cast<std::uint16_t>(1u << relative);
    customMask_ |= 0x0001u;
    if (onCustomMaskChanged) onCustomMaskChanged(customMask_);
    repaint();
}

PluginEditor::PluginEditor(PluginProcessor& processor)
    : juce::AudioProcessorEditor(processor), processor_(processor), tooltip_(this, 550)
{
    setLookAndFeel(&lookAndFeel_);
    setOpaque(true);
    setResizable(true, true);
    setResizeLimits(820, 560, 1440, 900);
    setSize(1040, 680);

    addChoiceItems(vocalRange_.box(), {"Auto", "Low", "Mid", "High"});
    addChoiceItems(key_.box(), {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"});
    addChoiceItems(scale_.box(), {"Major", "Natural Minor", "Chromatic", "Custom"});
    addChoiceItems(mode_.box(), {"Natural", "Modern Rap", "Trap Lock", "Hook Debug"});

    configureCombo(vocalRange_, "Detector range. Auto is the safe default for mixed vocal sessions.");
    configureCombo(key_, "Manual song key and Auto Key fallback while the estimator is learning.");
    configureCombo(scale_, "Manual scale and Auto Key fallback. In Custom mode click notes in Scale Map.");
    configureCombo(mode_, "Correction character: Natural loose, Modern Rap tight, Trap Lock fastest, Hook smoother.");
    configurePercentKnob(tune_, "Overall correction strength.");
    configurePercentKnob(stability_, "How strongly note commitment resists flutter and rapid target changes.");
    configurePercentKnob(feel_, "Preserves intentional pitch movement and expressive phrasing.");
    configurePercentKnob(formant_, "PSOLA formant-retention strength. 100% keeps source-period grain geometry.");
    configurePercentKnob(consonant_, "Protects sibilants, noisy consonants and hard onsets from pitch shifting.");
    configurePercentKnob(mix_, "Wet/dry mix with click-safe smoothing.");

    speed_.slider().setTextValueSuffix(" ms");
    speed_.slider().setNumDecimalPlacesToDisplay(1);
    speed_.slider().setTooltip("Pitch-correction response time. Lower values sound tighter and more synthetic.");
    output_.slider().setTextValueSuffix(" dB");
    output_.slider().setNumDecimalPlacesToDisplay(1);
    output_.slider().setTooltip("Output trim with click-safe smoothing.");

    for (auto* button : {&autoKey_, &bypass_}) {
        button->setClickingTogglesState(true);
        button->setColour(juce::TextButton::textColourOffId, colour(0xffaab6c1u));
        button->setColour(juce::TextButton::textColourOnId, colour(kText));
    }
    autoKey_.setTooltip("Estimate major/minor key from stable incoming vocal pitches. Manual Key/Scale remain the learning fallback.");
    bypass_.setTooltip("Smoothly bypass SonRapTune while keeping latency alignment.");

    const std::array<juce::Component*, 16> components{
        &vocalRange_, &key_, &scale_, &mode_,
        &speed_, &feel_, &tune_, &stability_,
        &formant_, &consonant_, &mix_, &output_,
        &pitchDisplay_, &scaleStrip_, &autoKey_, &bypass_
    };
    for (auto* component : components)
        addAndMakeVisible(*component);

    auto& state = processor_.apvts;
    vocalRangeAttachment_ = std::make_unique<ComboAttachment>(state, ParamIds::vocalRange, vocalRange_.box());
    keyAttachment_ = std::make_unique<ComboAttachment>(state, ParamIds::key, key_.box());
    scaleAttachment_ = std::make_unique<ComboAttachment>(state, ParamIds::scale, scale_.box());
    modeAttachment_ = std::make_unique<ComboAttachment>(state, ParamIds::mode, mode_.box());
    speedAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::speed, speed_.slider());
    feelAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::feel, feel_.slider());
    tuneAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::tune, tune_.slider());
    stabilityAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::stability, stability_.slider());
    formantAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::formant, formant_.slider());
    consonantAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::consonant, consonant_.slider());
    mixAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::mix, mix_.slider());
    outputAttachment_ = std::make_unique<SliderAttachment>(state, ParamIds::outputTrim, output_.slider());
    autoKeyAttachment_ = std::make_unique<ButtonAttachment>(state, ParamIds::autoKey, autoKey_);
    bypassAttachment_ = std::make_unique<ButtonAttachment>(state, ParamIds::bypass, bypass_);

    scaleStrip_.onCustomMaskChanged = [this](std::uint16_t mask) {
        if (auto* parameter = processor_.apvts.getParameter(ParamIds::customMask)) {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(mask)));
            parameter->endChangeGesture();
        }
    };

    timerCallback();
    startTimerHz(30);
}

PluginEditor::~PluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PluginEditor::configurePercentKnob(MacroKnob& knob, const juce::String& tooltip)
{
    knob.slider().setTextValueSuffix(" %");
    knob.slider().setNumDecimalPlacesToDisplay(0);
    knob.slider().setTooltip(tooltip);
}

void PluginEditor::configureCombo(LabeledCombo& combo, const juce::String& tooltip)
{
    combo.box().setTooltip(tooltip);
}

void PluginEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient background(colour(0xff090c11u), 0.0f, 0.0f,
                                     colour(0xff14111cu), static_cast<float>(getWidth()),
                                     static_cast<float>(getHeight()), false);
    background.addColour(0.55, colour(0xff0f151cu));
    g.setGradientFill(background);
    g.fillAll();

    auto bounds = getLocalBounds().reduced(18);
    auto header = bounds.removeFromTop(52);
    auto logo = header.removeFromLeft(38).toFloat().reduced(3.0f, 5.0f);
    g.setColour(colour(0xff18232du));
    g.fillRoundedRectangle(logo, 9.0f);
    g.setColour(colour(kAccent));
    juce::Path mark;
    mark.startNewSubPath(logo.getX() + 9.0f, logo.getCentreY() + 7.0f);
    mark.cubicTo(logo.getX() + 13.0f, logo.getY() + 3.0f,
                 logo.getRight() - 11.0f, logo.getBottom() - 3.0f,
                 logo.getRight() - 7.0f, logo.getCentreY() - 6.0f);
    g.strokePath(mark, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

    auto brand = header.removeFromLeft(270).reduced(9, 0);
    auto product = brand.removeFromTop(30);
    g.setColour(colour(kText));
    g.setFont(21.0f);
    g.drawText("SONRAPTUNE", product, juce::Justification::centredLeft, false);
    g.setColour(colour(kMuted));
    g.setFont(10.5f);
    g.drawText("SMART RAP PITCH / BETA", brand, juce::Justification::centredLeft, false);

    auto status = header.removeFromRight(320).reduced(0, 7);
    status.removeFromRight(204);
    g.drawText("PITCH ENGINE / LIVE", status, juce::Justification::centredRight, false);
    g.setColour(colour(kBorder).withAlpha(0.75f));
    g.drawHorizontalLine(68, 18.0f, static_cast<float>(getWidth() - 18));
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(18);
    auto header = area.removeFromTop(52);
    bypass_.setBounds(header.removeFromRight(96).reduced(4, 8));
    autoKey_.setBounds(header.removeFromRight(108).reduced(4, 8));

    area.removeFromTop(10);
    auto setup = area.removeFromTop(88);
    const auto compactKnobWidth = juce::jlimit(72, 98, setup.getWidth() / 10);
    output_.setBounds(setup.removeFromRight(compactKnobWidth).reduced(3, 0));
    mix_.setBounds(setup.removeFromRight(compactKnobWidth).reduced(3, 0));
    setup.removeFromRight(8);
    const auto comboWidth = setup.getWidth() / 4;
    vocalRange_.setBounds(setup.removeFromLeft(comboWidth).reduced(3, 0));
    key_.setBounds(setup.removeFromLeft(comboWidth).reduced(3, 0));
    scale_.setBounds(setup.removeFromLeft(comboWidth).reduced(3, 0));
    mode_.setBounds(setup.reduced(3, 0));

    area.removeFromTop(8);
    scaleStrip_.setBounds(area.removeFromBottom(92).reduced(0, 2));
    area.removeFromBottom(8);
    auto macros = area.removeFromBottom(146);
    const auto macroWidth = macros.getWidth() / 4;
    tune_.setBounds(macros.removeFromLeft(macroWidth).reduced(6, 0));
    stability_.setBounds(macros.removeFromLeft(macroWidth).reduced(6, 0));
    formant_.setBounds(macros.removeFromLeft(macroWidth).reduced(6, 0));
    consonant_.setBounds(macros.reduced(6, 0));

    area.removeFromBottom(8);
    const auto sideWidth = juce::jlimit(145, 205, area.getWidth() / 5);
    speed_.setBounds(area.removeFromLeft(sideWidth).reduced(6, 0));
    feel_.setBounds(area.removeFromRight(sideWidth).reduced(6, 0));
    pitchDisplay_.setBounds(area.reduced(8, 0));
}

void PluginEditor::timerCallback()
{
    PitchFrame frame;
    while (processor_.telemetry.pop(frame)) {
        latestFrame_ = frame;
        hasLatestFrame_ = true;
        pitchDisplay_.setFrame(frame);
    }

    const auto* keyValue = processor_.apvts.getRawParameterValue(ParamIds::key);
    const auto* scaleValue = processor_.apvts.getRawParameterValue(ParamIds::scale);
    const auto* customMaskValue = processor_.apvts.getRawParameterValue(ParamIds::customMask);
    const auto* autoKeyValue = processor_.apvts.getRawParameterValue(ParamIds::autoKey);
    if (keyValue == nullptr || scaleValue == nullptr || customMaskValue == nullptr
        || autoKeyValue == nullptr) return;

    const bool autoEnabled = autoKeyValue->load() > 0.5f;
    key_.box().setEnabled(!autoEnabled);
    scale_.box().setEnabled(!autoEnabled);

    int displayKey = static_cast<int>(keyValue->load());
    int displayScale = static_cast<int>(scaleValue->load());
    if (autoEnabled && hasLatestFrame_ && latestFrame_.keyEstimateReady) {
        displayKey = latestFrame_.resolvedKey;
        displayScale = static_cast<int>(latestFrame_.resolvedScale);
    }

    scaleStrip_.setScale(juce::jlimit(0, 11, displayKey), juce::jlimit(0, 3, displayScale),
        static_cast<std::uint16_t>(juce::jlimit(1, 4095,
            static_cast<int>(customMaskValue->load()))));
}

} // namespace sonraptune
