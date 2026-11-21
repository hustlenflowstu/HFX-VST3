#pragma once
#include <JuceHeader.h>

namespace HFXColours
{
    static const juce::Colour bg      { 0xff0b0b0d };
    static const juce::Colour panel   { 0xff141418 };
    static const juce::Colour panel2  { 0xff101014 };
    static const juce::Colour grid    { 0xff29292f };
    static const juce::Colour orange  { 0xffff6b00 };
    static const juce::Colour white   { 0xffeeeeef };
    static const juce::Colour muted   { 0xff85858d };
}

class HFXLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    HFXLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
};
