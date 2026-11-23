#include "PluginEditor.h"

namespace
{
    constexpr int panelModeCount = 5;
    const juce::StringArray vocalPresets {
        "Clean Vocal", "Warm Studio Lead", "Modern Rap Lead", "Melodic Rap", "Airy R&B", "Bright Pop", "Wide Adlibs", "Aggressive Vocal"
    };
    const juce::StringArray beatPresets {
        "Modern Trap", "Drill Punch", "Punchy 808", "Boom Bap Glue", "Warm R&B Beat", "Lo-Fi Texture", "Afro Bounce", "Clean Instrumental"
    };
}

void HFXSpectrumView::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour(HFXColours::panel2);
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(HFXColours::grid);
    for (int i = 1; i < 6; ++i)
        g.drawHorizontalLine((int)(r.getY() + r.getHeight() * i / 6.0f), r.getX(), r.getRight());
    for (int i = 1; i < 10; ++i)
        g.drawVerticalLine((int)(r.getX() + r.getWidth() * i / 10.0f), r.getY(), r.getBottom());

    // Each processor gets its own visual language rather than reusing one spectrum graphic.
    if (displayMode == 0) // EQ: live spectrum + response curve
    {
        juce::Path spectrum, fill;
        constexpr int bins = 160;
        for (int i = 0; i < bins; ++i)
        {
            const int sourceBin = juce::jlimit(0, 511, (int)(std::pow((double)i / (bins - 1), 2.0) * 511.0));
            const float v = processor.spectrumAt(sourceBin);
            const float x = r.getX() + r.getWidth() * (float)i / (float)(bins - 1);
            const float y = r.getBottom() - v * r.getHeight() * 0.88f;
            if (i == 0) { spectrum.startNewSubPath(x, y); fill.startNewSubPath(x, r.getBottom()); fill.lineTo(x, y); }
            else { spectrum.lineTo(x, y); fill.lineTo(x, y); }
        }
        fill.lineTo(r.getRight(), r.getBottom()); fill.closeSubPath();
        g.setColour(HFXColours::orange.withAlpha(0.13f)); g.fillPath(fill);
        g.setColour(HFXColours::white.withAlpha(0.88f));
        g.strokePath(spectrum, juce::PathStrokeType(1.4f));
        juce::Path response;
        response.startNewSubPath(r.getX(), r.getCentreY() + 18.0f);
        response.cubicTo(r.getX() + r.getWidth() * 0.18f, r.getCentreY() + 18.0f,
                         r.getX() + r.getWidth() * 0.23f, r.getCentreY() - 28.0f,
                         r.getX() + r.getWidth() * 0.40f, r.getCentreY() - 8.0f);
        response.cubicTo(r.getX() + r.getWidth() * 0.60f, r.getCentreY() + 10.0f,
                         r.getX() + r.getWidth() * 0.74f, r.getCentreY() - 35.0f,
                         r.getRight(), r.getCentreY() - 4.0f);
        g.setColour(HFXColours::orange.withAlpha(0.95f));
        g.strokePath(response, juce::PathStrokeType(2.2f));
    }
    else if (displayMode == 1) // Compressor: input/output transfer curve
    {
        const auto c = r.getCentre();
        juce::Path diagonal; diagonal.startNewSubPath(r.getX() + 16, r.getBottom() - 16); diagonal.lineTo(r.getRight() - 16, r.getY() + 16);
        g.setColour(HFXColours::muted.withAlpha(0.65f)); g.strokePath(diagonal, juce::PathStrokeType(1.2f));
        juce::Path curve; curve.startNewSubPath(r.getX() + 16, r.getBottom() - 16);
        curve.cubicTo(c.x - 15, c.y + 22, c.x + 18, c.y - 2, r.getRight() - 16, r.getY() + 48);
        g.setColour(HFXColours::orange.withAlpha(0.16f));
        juce::Path area = curve; area.lineTo(r.getRight() - 16, r.getBottom() - 16); area.closeSubPath(); g.fillPath(area);
        g.setColour(HFXColours::orange); g.strokePath(curve, juce::PathStrokeType(2.5f));
        g.setColour(HFXColours::white); g.setFont(12.0f); g.drawText("INPUT  →  OUTPUT / GAIN REDUCTION", r.toNearestInt().reduced(12, 8), juce::Justification::topLeft);
    }
    else if (displayMode == 2) // Saturator: transfer curve
    {
        juce::Path curve;
        const float mid = r.getCentreY();
        for (int i = 0; i <= 120; ++i)
        {
            const float xNorm = (float)i / 120.0f * 2.0f - 1.0f;
            const float yNorm = std::tanh(xNorm * 1.8f) / std::tanh(1.8f);
            const float x = r.getCentreX() + xNorm * r.getWidth() * 0.46f;
            const float y = mid - yNorm * r.getHeight() * 0.43f;
            if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
        }
        g.setColour(HFXColours::orange.withAlpha(0.2f)); g.strokePath(curve, juce::PathStrokeType(9.0f));
        g.setColour(HFXColours::orange); g.strokePath(curve, juce::PathStrokeType(2.5f));
        g.setColour(HFXColours::muted); g.drawText("SOFT CLIP / HARMONIC TRANSFER", r.toNearestInt().reduced(12, 8), juce::Justification::topLeft);
    }
    else if (displayMode == 3) // Stereo field / vectorscope-inspired display
    {
        const float radius = juce::jmin(r.getWidth(), r.getHeight()) * 0.36f;
        const auto c = r.getCentre();
        g.setColour(HFXColours::grid);
        g.drawEllipse(c.x - radius, c.y - radius, radius * 2, radius * 2, 1.0f);
        g.drawEllipse(c.x - radius * 0.66f, c.y - radius * 0.66f, radius * 1.32f, radius * 1.32f, 1.0f);
        g.drawLine(c.x - radius, c.y, c.x + radius, c.y, 1.0f);
        g.drawLine(c.x, c.y - radius, c.x, c.y + radius, 1.0f);
        juce::Path cloud;
        for (int i = 0; i <= 100; ++i)
        {
            const float a = (float)i * 0.31f;
            const float spread = 0.18f + 0.7f * std::abs(std::sin(a * 0.47f));
            const float x = c.x + std::cos(a) * radius * spread;
            const float y = c.y + std::sin(a * 1.7f) * radius * spread * 0.56f;
            if (i == 0) cloud.startNewSubPath(x, y); else cloud.lineTo(x, y);
        }
        g.setColour(HFXColours::orange.withAlpha(0.18f)); g.strokePath(cloud, juce::PathStrokeType(7.0f));
        g.setColour(HFXColours::orange); g.strokePath(cloud, juce::PathStrokeType(1.5f));
        g.setColour(HFXColours::muted); g.drawText("STEREO FIELD / L-R", r.toNearestInt().reduced(12, 8), juce::Justification::topLeft);
    }
    else // Limiter: peak history and ceiling line
    {
        juce::Path trace;
        for (int i = 0; i < 140; ++i)
        {
            const float x = r.getX() + 10.0f + (r.getWidth() - 20.0f) * (float)i / 139.0f;
            const float wave = 0.22f + 0.62f * std::abs(std::sin(i * 0.19f) * std::sin(i * 0.053f + 0.7f));
            const float y = r.getBottom() - 12.0f - wave * (r.getHeight() - 24.0f);
            if (i == 0) trace.startNewSubPath(x, y); else trace.lineTo(x, y);
        }
        g.setColour(HFXColours::orange.withAlpha(0.14f));
        juce::Path area = trace; area.lineTo(r.getRight() - 10, r.getBottom() - 10); area.lineTo(r.getX() + 10, r.getBottom() - 10); area.closeSubPath(); g.fillPath(area);
        g.setColour(HFXColours::white); g.strokePath(trace, juce::PathStrokeType(1.5f));
        const float ceilingY = r.getY() + r.getHeight() * 0.18f;
        g.setColour(HFXColours::orange); g.drawHorizontalLine((int)ceilingY, r.getX() + 8, r.getRight() - 8);
        g.setColour(HFXColours::orange); g.drawText("OUTPUT CEILING", r.toNearestInt().reduced(12, 8), juce::Justification::topLeft);
    }
}

HFXAudioProcessorEditor::HFXAudioProcessorEditor(HFXAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), spectrumView(p)
{
    setLookAndFeel(&laf);
    setSize(1120, 690);
    logo.setText("HFX", juce::dontSendNotification);
    logo.setFont(juce::Font(42.0f, juce::Font::bold));
    logo.setColour(juce::Label::textColourId, HFXColours::orange);
    addAndMakeVisible(logo);

    trackMode.addItem("MAIN VOCAL", 1);
    trackMode.addItem("BEAT", 2);
    addAndMakeVisible(trackMode);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state(), "trackMode", trackMode);

    stemRole.addItem("KICK", 1);
    stemRole.addItem("SNARE", 2);
    stemRole.addItem("PERCUSSION", 3);
    stemRole.addItem("CLOSED HAT", 4);
    stemRole.addItem("OPEN HAT", 5);
    stemRole.addItem("EFX", 6);
    stemRole.addItem("808 / BASS", 7);
    stemRole.addItem("DRUM FILLER", 8);
    stemRole.addItem("OTHER", 9);
    addAndMakeVisible(stemRole);
    stemRoleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state(), "stemRole", stemRole);
    stemRole.setVisible(trackMode.getSelectedId() == 2);
    trackMode.onChange = [this]
    {
        const bool beatMode = trackMode.getSelectedId() == 2;
        stemRole.setVisible(beatMode);
        refreshPresetChoices();
        resized();
    };
    refreshPresetChoices();
    lastPresetMode = trackMode.getSelectedId();

    presetBox.onChange = [this] { if (presetBox.getSelectedId() > 0) applyPreset(presetBox.getSelectedId()); };
    addAndMakeVisible(presetBox);

    autoMix.onClick = [this] { processor.requestAutoMix(); status.setText("ANALYZING INPUT — PLAY AUDIO FOR 4 SECONDS", juce::dontSendNotification); };
    addAndMakeVisible(autoMix);
    bypass.setClickingTogglesState(true);
    addAndMakeVisible(bypass);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state(), "bypass", bypass);

    const juce::String tabNames[] { "AUTO-EQ", "COMPRESSOR", "SATURATOR", "PAN & WIDTH", "LIMITER" };
    for (int i = 0; i < panelModeCount; ++i)
    {
        tabs[(size_t)i].setButtonText(tabNames[i]);
        tabs[(size_t)i].setClickingTogglesState(true);
        tabs[(size_t)i].onClick = [this, i] { selectTab(i); };
        addAndMakeVisible(tabs[(size_t)i]);
    }
    tabs[0].setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible(spectrumView);
    for (int i = 0; i < 4; ++i)
    {
        auto& slider = knobs[(size_t)i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
        slider.setLookAndFeel(&laf);
        addAndMakeVisible(slider);
        knobLabels[(size_t)i].setJustificationType(juce::Justification::centred);
        knobLabels[(size_t)i].setColour(juce::Label::textColourId, HFXColours::muted);
        knobLabels[(size_t)i].setFont(juce::Font(11.0f, juce::Font::bold));
        addAndMakeVisible(knobLabels[(size_t)i]);
    }
    status.setColour(juce::Label::textColourId, HFXColours::muted); addAndMakeVisible(status);
    leftMeterText.setColour(juce::Label::textColourId, HFXColours::orange);
    rightMeterText.setColour(juce::Label::textColourId, HFXColours::orange);
    rmsText.setColour(juce::Label::textColourId, HFXColours::muted);
    addAndMakeVisible(leftMeterText); addAndMakeVisible(rightMeterText); addAndMakeVisible(rmsText);
    rebuildAttachments();
    startTimerHz(20);
}

HFXAudioProcessorEditor::~HFXAudioProcessorEditor()
{
    stopTimer();
    for (auto& slider : knobs) slider.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void HFXAudioProcessorEditor::refreshPresetChoices()
{
    const bool beat = trackMode.getSelectedId() == 2;
    presetBox.onChange = nullptr;
    presetBox.clear(juce::dontSendNotification);
    const auto& choices = beat ? beatPresets : vocalPresets;
    for (int i = 0; i < choices.size(); ++i) presetBox.addItem(choices[i], i + 1);
    presetBox.setText(beat ? "BEAT PRESETS" : "VOCAL PRESETS", juce::dontSendNotification);
    presetBox.onChange = [this] { if (presetBox.getSelectedId() > 0) applyPreset(presetBox.getSelectedId()); };
}

void HFXAudioProcessorEditor::selectTab(int index)
{
    activeTab = juce::jlimit(0, 4, index);
    for (int i = 0; i < panelModeCount; ++i)
        tabs[(size_t)i].setToggleState(i == activeTab, juce::dontSendNotification);
    spectrumView.setDisplayMode(activeTab);
    rebuildAttachments();
    resized();
    repaint();
}

void HFXAudioProcessorEditor::rebuildAttachments()
{
    for (auto& attachment : attachments) attachment.reset();
    for (int i = 0; i < 4; ++i)
    {
        const bool used = !(activeTab == 0 && i == 3);
        knobLabels[(size_t)i].setVisible(used);
        knobs[(size_t)i].setVisible(used);
        if (used)
        {
            knobLabels[(size_t)i].setText(names[(size_t)activeTab][(size_t)i], juce::dontSendNotification);
            attachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state(), ids[(size_t)activeTab][(size_t)i], knobs[(size_t)i]);
        }
    }
}

void HFXAudioProcessorEditor::applyPreset(int id)
{
    auto set = [this](const char* key, float value)
    {
        if (auto* parameter = processor.state().getParameter(key))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    const bool beat = trackMode.getSelectedId() == 2;
    if (!beat)
    {
        switch (id)
        {
            case 1: set("eqFreq", 320); set("eqGain", -1.5f); set("eqQ", 1.1f); set("threshold", -20); set("ratio", 3); set("attack", 15); set("release", 100); set("drive", 1); set("satMix", .15f); set("width", 1.02f); set("ceiling", -1); break;
            case 2: set("eqFreq", 240); set("eqGain", -1); set("eqQ", .9f); set("threshold", -22); set("ratio", 2.8f); set("attack", 22); set("release", 130); set("drive", 1.4f); set("satMix", .18f); set("width", 1.04f); break;
            case 3: set("eqFreq", 280); set("eqGain", -1); set("threshold", -22); set("ratio", 4); set("attack", 8); set("release", 85); set("drive", 2); set("satMix", .25f); set("width", 1.08f); break;
            case 4: set("eqFreq", 350); set("eqGain", -1.2f); set("threshold", -21); set("ratio", 4.5f); set("attack", 6); set("release", 75); set("drive", 3); set("satMix", .30f); set("width", 1.05f); break;
            case 5: set("eqFreq", 420); set("eqGain", -1.8f); set("threshold", -24); set("ratio", 2.5f); set("attack", 18); set("release", 145); set("drive", .8f); set("tone", .18f); set("satMix", .12f); set("width", 1.12f); break;
            case 6: set("eqFreq", 5200); set("eqGain", 1.2f); set("threshold", -20); set("ratio", 2.5f); set("attack", 12); set("release", 95); set("drive", 1.2f); set("tone", .25f); set("width", 1.06f); break;
            case 7: set("eqFreq", 300); set("eqGain", -1); set("threshold", -24); set("ratio", 3.5f); set("attack", 5); set("release", 65); set("drive", 2.5f); set("satMix", .24f); set("width", 1.35f); break;
            case 8: set("eqFreq", 380); set("eqGain", -2); set("threshold", -25); set("ratio", 6); set("attack", 3); set("release", 60); set("drive", 4); set("satMix", .38f); set("width", 1.08f); break;
        }
    }
    else
    {
        switch (id)
        {
            case 1: set("eqFreq", 250); set("eqGain", -.8f); set("threshold", -16); set("ratio", 3); set("attack", 25); set("release", 110); set("drive", 2.5f); set("satMix", .25f); set("width", 1.12f); break;
            case 2: set("eqFreq", 180); set("eqGain", 1); set("threshold", -14); set("ratio", 3); set("attack", 30); set("release", 120); set("drive", 2); set("satMix", .20f); set("width", 1.05f); break;
            case 3: set("eqFreq", 220); set("eqGain", -.5f); set("threshold", -17); set("ratio", 4); set("attack", 18); set("release", 90); set("drive", 3.2f); set("satMix", .28f); set("width", 1.16f); break;
            case 4: set("eqFreq", 300); set("eqGain", -1.2f); set("threshold", -19); set("ratio", 2.5f); set("attack", 32); set("release", 145); set("drive", 1.4f); set("satMix", .16f); set("width", 1.0f); break;
            case 5: set("eqFreq", 260); set("eqGain", -.8f); set("threshold", -18); set("ratio", 2.8f); set("attack", 24); set("release", 125); set("drive", 1.5f); set("tone", -.12f); set("width", 1.08f); break;
            case 6: set("eqFreq", 500); set("eqGain", -2); set("threshold", -22); set("ratio", 2); set("attack", 35); set("release", 180); set("drive", 3.8f); set("warmth", .4f); set("satMix", .32f); set("width", .92f); break;
            case 7: set("eqFreq", 280); set("eqGain", -.6f); set("threshold", -16); set("ratio", 3); set("attack", 20); set("release", 100); set("drive", 2.2f); set("satMix", .22f); set("width", 1.18f); break;
            case 8: set("eqFreq", 320); set("eqGain", -.8f); set("threshold", -18); set("ratio", 2.2f); set("attack", 28); set("release", 130); set("drive", 1); set("satMix", .1f); set("width", 1.02f); break;
        }
    }
    status.setText((beat ? "BEAT" : "VOCAL") + juce::String(" PRESET LOADED — ADJUST TO TASTE"), juce::dontSendNotification);
}

void HFXAudioProcessorEditor::timerCallback()
{
    const int currentMode = trackMode.getSelectedId();
    if (currentMode != lastPresetMode)
    {
        lastPresetMode = currentMode;
        refreshPresetChoices();
    }
    spectrumView.repaint();
    leftMeterText.setText("L PEAK  " + juce::String(processor.leftPeakDb(), 1) + " dBFS", juce::dontSendNotification);
    rightMeterText.setText("R PEAK  " + juce::String(processor.rightPeakDb(), 1) + " dBFS", juce::dontSendNotification);
    rmsText.setText("OUTPUT RMS  " + juce::String(processor.outputRmsDb(), 1) + " dBFS", juce::dontSendNotification);
    repaint();
    if (processor.autoMixReady())
    {
        const auto result = processor.takeAutoMixResult();
        auto set = [this](const char* key, float value)
        {
            if (auto* parameter = processor.state().getParameter(key)) parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        set("eqGain", result.eqGain); set("threshold", result.threshold); set("drive", result.drive); set("width", result.width); set("inputGain", result.inputGain);
        status.setText("AUTO-MIX STARTING POINT APPLIED  |  RMS " + juce::String(result.rmsDb, 1) + " dBFS  |  CREST " + juce::String(result.crestDb, 1) + " dB", juce::dontSendNotification);
    }
}

void HFXAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(HFXColours::bg);
    auto main = getLocalBounds().toFloat().reduced(16.0f);
    g.setColour(HFXColours::panel); g.fillRoundedRectangle(main.withTrimmedTop(90.0f), 12.0f);
    g.setColour(HFXColours::grid); g.drawRoundedRectangle(main.withTrimmedTop(90.0f), 12.0f, 1.0f);

    const juce::String titles[] { "AUTO-EQ  /  FREQUENCY SHAPING", "COMPRESSOR  /  DYNAMIC CONTROL", "SATURATOR  /  HARMONIC COLOR", "PAN & WIDTH  /  STEREO FIELD", "LIMITER  /  PEAK CONTROL" };
    g.setColour(HFXColours::orange); g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(titles[activeTab], 32, 155, 600, 22, juce::Justification::left);

    // Soft multi-pass text glow for the HFX / HF identity.
    g.setFont(juce::Font(42.0f, juce::Font::bold));
    for (int pass = 4; pass >= 1; --pass)
    {
        g.setColour(HFXColours::orange.withAlpha(0.035f * (float)pass));
        g.drawText("HFX", 22 - pass, 13 - pass, 120 + pass * 2, 58 + pass * 2, juce::Justification::left);
    }
    g.setColour(HFXColours::orange); g.drawText("HF", 28, getHeight() - 105, 54, 42, juce::Justification::centred);
    g.setColour(HFXColours::orange.withAlpha(0.16f)); g.drawText("HF", 24, getHeight() - 109, 62, 50, juce::Justification::centred);

    const int mx = getWidth() - 76;
    g.setColour(HFXColours::panel2); g.fillRoundedRectangle((float)mx, 205.0f, 18.0f, 280.0f, 5.0f); g.fillRoundedRectangle((float)mx + 26.0f, 205.0f, 18.0f, 280.0f, 5.0f);
    const float left = juce::jlimit(0.0f, 1.0f, (processor.leftPeakDb() + 60.0f) / 60.0f);
    const float right = juce::jlimit(0.0f, 1.0f, (processor.rightPeakDb() + 60.0f) / 60.0f);
    g.setColour(HFXColours::orange); g.fillRoundedRectangle((float)mx + 2.0f, 483.0f - left * 276.0f, 14.0f, left * 276.0f, 3.0f); g.fillRoundedRectangle((float)mx + 28.0f, 483.0f - right * 276.0f, 14.0f, right * 276.0f, 3.0f);
    g.setColour(HFXColours::muted); g.setFont(10.0f); g.drawText("L", mx, 490, 18, 14, juce::Justification::centred); g.drawText("R", mx + 26, 490, 18, 14, juce::Justification::centred);
}

void HFXAudioProcessorEditor::resized()
{
    logo.setBounds(4, 2, 160, 65);
    trackMode.setBounds(getWidth() - 510, 18, 165, 34);
    stemRole.setBounds(getWidth() - 335, 18, 170, 34);
    presetBox.setBounds(getWidth() - 510, 58, 310, 32);
    autoMix.setBounds(getWidth() - 148, 18, 128, 36);
    const int tabY = 91, tabHeight = 38, tabGap = 5;
    const int tabWidth = (getWidth() - 40 - tabGap * 4) / 5;
    for (int i = 0; i < panelModeCount; ++i) tabs[(size_t)i].setBounds(20 + i * (tabWidth + tabGap), tabY, tabWidth, tabHeight);
    spectrumView.setBounds(30, 185, getWidth() - 130, 290);
    bypass.setBounds(30, 495, 90, 28);
    status.setBounds(135, 498, getWidth() - 245, 24);
    leftMeterText.setBounds(getWidth() - 110, 520, 100, 18); rightMeterText.setBounds(getWidth() - 110, 540, 100, 18); rmsText.setBounds(getWidth() - 110, 560, 100, 18);
    const int deckY = 525, labelWidth = 135, startX = 110;
    for (int i = 0; i < 4; ++i)
    {
        const int x = startX + i * labelWidth;
        knobLabels[(size_t)i].setBounds(x, deckY - 8, labelWidth - 8, 18);
        knobs[(size_t)i].setBounds(x + 12, deckY + 12, 100, 105);
    }
}
