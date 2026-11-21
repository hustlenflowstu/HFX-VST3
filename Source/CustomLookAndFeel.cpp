#include "CustomLookAndFeel.h"

HFXLookAndFeel::HFXLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, HFXColours::white);
    setColour(juce::Slider::textBoxBackgroundColourId, HFXColours::panel2);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::textColourId, HFXColours::white);
    setColour(juce::ComboBox::backgroundColourId, HFXColours::panel2);
    setColour(juce::ComboBox::outlineColourId, HFXColours::grid);
    setColour(juce::Label::textColourId, HFXColours::white);
}

void HFXLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float pos, float start, float end, juce::Slider&)
{
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(7.0f);
    const float d = juce::jmin(b.getWidth(), b.getHeight());
    auto dial = b.withSizeKeepingCentre(d, d);
    const auto c = dial.getCentre();
    const float radius = d * 0.5f;
    const float angle = start + pos * (end - start);

    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.fillEllipse(dial.translated(0.0f, 2.0f));
    g.setColour(juce::Colour(0xff25252b));
    g.fillEllipse(dial.reduced(4.0f));
    g.setColour(juce::Colour(0xff08080a));
    g.drawEllipse(dial.reduced(4.0f), 1.2f);

    juce::Path base;
    base.addCentredArc(c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, start, end, true);
    g.setColour(juce::Colour(0xff38383f));
    g.strokePath(base, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path active;
    active.addCentredArc(c.x, c.y, radius - 2.0f, radius - 2.0f, 0.0f, start, angle, true);
    g.setColour(HFXColours::orange.withAlpha(0.20f));
    g.strokePath(active, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(HFXColours::orange);
    g.strokePath(active, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.3f, -radius * 0.67f, 2.6f, radius * 0.25f, 1.0f);
    g.setColour(HFXColours::orange);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
    g.setColour(juce::Colour(0xff303038));
    g.fillEllipse(c.x - radius * 0.46f, c.y - radius * 0.46f, radius * 0.92f, radius * 0.92f);
}

void HFXLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                          const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = b.getToggleState() || down;
    g.setColour(on ? HFXColours::orange.withAlpha(0.2f) : juce::Colours::transparentBlack);
    g.fillRoundedRectangle(r.expanded(2.0f), 6.0f);
    g.setColour(on ? HFXColours::orange : (over ? juce::Colour(0xff303038) : HFXColours::panel2));
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(on ? HFXColours::orange : HFXColours::grid);
    g.drawRoundedRectangle(r, 6.0f, on ? 1.5f : 1.0f);
}

void HFXLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour(b.getToggleState() ? juce::Colours::black : HFXColours::white);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(4), juce::Justification::centred, 1);
}

void HFXLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool,
                                  int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0.5f, 0.5f, (float)w - 1.0f, (float)h - 1.0f);
    g.setColour(HFXColours::panel2); g.fillRoundedRectangle(r, 6.0f);
    g.setColour(HFXColours::grid); g.drawRoundedRectangle(r, 6.0f, 1.0f);
    juce::Path p;
    p.startNewSubPath((float)w - 18.0f, (float)h * 0.44f);
    p.lineTo((float)w - 10.0f, (float)h * 0.44f);
    p.lineTo((float)w - 14.0f, (float)h * 0.60f);
    g.setColour(HFXColours::muted); g.strokePath(p, juce::PathStrokeType(1.5f));
}
