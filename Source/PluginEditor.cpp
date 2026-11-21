#include "PluginEditor.h"

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

    juce::Path line, fill;
    const int bins = 160;
    for (int i = 0; i < bins; ++i)
    {
        const int sourceBin = juce::jlimit(0, 511, static_cast<int>(std::pow(static_cast<double>(i) / static_cast<double>(bins - 1), 2.0) * 511.0));
        const float v = processor.spectrumAt(sourceBin);
        const float x = r.getX() + r.getWidth() * (float)i / (float)(bins - 1);
        const float y = r.getBottom() - v * r.getHeight() * 0.88f;
        if (i == 0) { line.startNewSubPath(x, y); fill.startNewSubPath(x, r.getBottom()); fill.lineTo(x, y); }
        else { line.lineTo(x, y); fill.lineTo(x, y); }
    }
    fill.lineTo(r.getRight(), r.getBottom()); fill.closeSubPath();
    g.setColour(HFXColours::orange.withAlpha(0.17f)); g.fillPath(fill);
    g.setColour(HFXColours::white.withAlpha(0.95f));
    g.strokePath(line, juce::PathStrokeType(1.5f));
}

HFXAudioProcessorEditor::HFXAudioProcessorEditor(HFXAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), spectrumView(p)
{
    setLookAndFeel(&laf);
    setSize(1120, 690);
    logo.setText("HFX", juce::dontSendNotification);
    logo.setFont(juce::Font(38.0f, juce::Font::bold));
    logo.setColour(juce::Label::textColourId, HFXColours::orange);
    addAndMakeVisible(logo);

    trackMode.addItem("MAIN VOCAL", 1);
    trackMode.addItem("BEAT", 2);
    addAndMakeVisible(trackMode);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.state(), "trackMode", trackMode);

    presetBox.addItem("FACTORY PRESETS", 1);
    presetBox.addItem("Clean Vocal", 2);
    presetBox.addItem("Melodic Rap", 3);
    presetBox.addItem("Trap Lead", 4);
    presetBox.addItem("Modern Drill Beat", 5);
    presetBox.addItem("Punchy 808 Beat", 6);
    presetBox.onChange = [this] { if (presetBox.getSelectedId() > 1) applyPreset(presetBox.getSelectedId()); };
    addAndMakeVisible(presetBox);

    autoMix.onClick = [this] { processor.requestAutoMix(); status.setText("ANALYZING INPUT — PLAY AUDIO FOR 4 SECONDS", juce::dontSendNotification); };
    addAndMakeVisible(autoMix);
    bypass.setClickingTogglesState(true);
    addAndMakeVisible(bypass);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.state(), "bypass", bypass);

    const juce::String tabNames[] { "AUTO-EQ", "COMPRESSOR", "SATURATOR", "PAN & WIDTH", "LIMITER" };
    for (int i = 0; i < 5; ++i)
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
        auto& s = knobs[(size_t)i];
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
        s.setLookAndFeel(&laf);
        addAndMakeVisible(s);
        knobLabels[(size_t)i].setJustificationType(juce::Justification::centred);
        knobLabels[(size_t)i].setColour(juce::Label::textColourId, HFXColours::muted);
        knobLabels[(size_t)i].setFont(juce::Font(11.0f, juce::Font::bold));
        addAndMakeVisible(knobLabels[(size_t)i]);
    }
    status.setColour(juce::Label::textColourId, HFXColours::muted);
    addAndMakeVisible(status);
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
    for (auto& s : knobs) s.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void HFXAudioProcessorEditor::selectTab(int index)
{
    activeTab = index;
    for (int i = 0; i < 5; ++i)
        tabs[(size_t)i].setToggleState(i == index, juce::dontSendNotification);
    rebuildAttachments();
    repaint();
}

void HFXAudioProcessorEditor::rebuildAttachments()
{
    for (auto& a : attachments) a.reset();
    for (int i = 0; i < 4; ++i)
    {
        knobLabels[(size_t)i].setText(names[(size_t)activeTab][(size_t)i], juce::dontSendNotification);
        attachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state(), ids[(size_t)activeTab][(size_t)i], knobs[(size_t)i]);
    }
}

void HFXAudioProcessorEditor::applyPreset(int id)
{
    auto set = [this](const char* key, float value)
    {
        if (auto* p = processor.state().getParameter(key))
            p->setValueNotifyingHost(p->convertTo0to1(value));
    };
    switch (id)
    {
        case 2: // Clean Vocal
            set("eqFreq", 320.0f); set("eqGain", -1.5f); set("eqQ", 1.1f);
            set("threshold", -20.0f); set("ratio", 3.0f); set("attack", 15.0f);
            set("release", 100.0f); set("drive", 1.0f); set("satMix", 0.15f);
            set("width", 1.02f); set("ceiling", -1.0f); break;
        case 3: // Melodic Rap
            set("eqFreq", 280.0f); set("eqGain", -1.0f); set("threshold", -22.0f);
            set("ratio", 4.0f); set("attack", 8.0f); set("release", 85.0f);
            set("drive", 2.0f); set("satMix", 0.25f); set("width", 1.08f); break;
        case 4: // Trap Lead
            set("eqFreq", 350.0f); set("eqGain", -1.2f); set("threshold", -21.0f);
            set("ratio", 4.5f); set("attack", 6.0f); set("release", 75.0f);
            set("drive", 3.0f); set("satMix", 0.30f); set("width", 1.05f); break;
        case 5: // Modern Drill Beat
            set("eqFreq", 250.0f); set("eqGain", -1.0f); set("threshold", -16.0f);
            set("ratio", 3.0f); set("attack", 25.0f); set("release", 110.0f);
            set("drive", 2.5f); set("satMix", 0.25f); set("width", 1.12f); break;
        case 6: // Punchy 808 Beat
            set("eqFreq", 180.0f); set("eqGain", 1.0f); set("threshold", -14.0f);
            set("ratio", 3.0f); set("attack", 30.0f); set("release", 120.0f);
            set("drive", 2.0f); set("satMix", 0.20f); set("width", 1.05f); break;
        default: break;
    }
    status.setText("FACTORY PRESET LOADED — ADJUST TO TASTE", juce::dontSendNotification);
}

void HFXAudioProcessorEditor::timerCallback()
{
    spectrumView.repaint();
    leftMeterText.setText("L PEAK  " + juce::String(processor.leftPeakDb(), 1) + " dBFS", juce::dontSendNotification);
    rightMeterText.setText("R PEAK  " + juce::String(processor.rightPeakDb(), 1) + " dBFS", juce::dontSendNotification);
    rmsText.setText("OUTPUT RMS  " + juce::String(processor.outputRmsDb(), 1) + " dBFS", juce::dontSendNotification);

    repaint();
    if (processor.autoMixReady())
    {
        const auto r = processor.takeAutoMixResult();
        auto set = [this](const char* key, float v)
        {
            if (auto* p = processor.state().getParameter(key))
                p->setValueNotifyingHost(p->convertTo0to1(v));
        };
        set("eqGain", r.eqGain); set("threshold", r.threshold);
        set("drive", r.drive); set("width", r.width); set("inputGain", r.inputGain);
        status.setText("AUTO-MIX SUGGESTIONS APPLIED  |  INPUT RMS " +
                       juce::String(r.rmsDb, 1) + " dBFS  |  CREST " +
                       juce::String(r.crestDb, 1) + " dB", juce::dontSendNotification);
    }
}

void HFXAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(HFXColours::bg);
    auto main = getLocalBounds().toFloat().reduced(16.0f);
    g.setColour(HFXColours::panel);
    g.fillRoundedRectangle(main.withTrimmedTop(90.0f), 12.0f);
    g.setColour(HFXColours::grid);
    g.drawRoundedRectangle(main.withTrimmedTop(90.0f), 12.0f, 1.0f);

    g.setColour(HFXColours::orange);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    const juce::String titles[] { "AUTO-EQ / LIVE SPECTRUM", "COMPRESSOR / DYNAMICS",
        "SATURATOR / HARMONICS", "PAN & WIDTH / STEREO FIELD", "LIMITER / OUTPUT CONTROL" };
    g.drawText(titles[activeTab], 32, 155, 500, 22, juce::Justification::left);

    // Master meter wells at right.
    const int mx = getWidth() - 76;
    g.setColour(HFXColours::panel2);
    g.fillRoundedRectangle((float)mx, 205.0f, 18.0f, 280.0f, 5.0f);
    g.fillRoundedRectangle((float)mx + 26.0f, 205.0f, 18.0f, 280.0f, 5.0f);
    const float l = juce::jlimit(0.0f, 1.0f, (processor.leftPeakDb() + 60.0f) / 60.0f);
    const float r = juce::jlimit(0.0f, 1.0f, (processor.rightPeakDb() + 60.0f) / 60.0f);
    g.setColour(HFXColours::orange);
    g.fillRoundedRectangle((float)mx + 2.0f, 483.0f - l * 276.0f, 14.0f, l * 276.0f, 3.0f);
    g.fillRoundedRectangle((float)mx + 28.0f, 483.0f - r * 276.0f, 14.0f, r * 276.0f, 3.0f);
    g.setColour(HFXColours::muted);
    g.setFont(10.0f);
    g.drawText("L", mx, 490, 18, 14, juce::Justification::centred);
    g.drawText("R", mx + 26, 490, 18, 14, juce::Justification::centred);
    g.setColour(HFXColours::orange.withAlpha(0.9f));
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("HF", 28, getHeight() - 105, 54, 42, juce::Justification::centred);
}

void HFXAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    logo.setBounds(4, 2, 160, 65);
    trackMode.setBounds(getWidth() - 510, 18, 165, 34);
    presetBox.setBounds(getWidth() - 335, 18, 175, 34);
    autoMix.setBounds(getWidth() - 148, 18, 128, 36);

    const int tabY = 91, tabH = 38, tabGap = 5;
    const int tabW = (getWidth() - 40 - tabGap * 4) / 5;
    for (int i = 0; i < 5; ++i)
        tabs[(size_t)i].setBounds(20 + i * (tabW + tabGap), tabY, tabW, tabH);

    spectrumView.setBounds(30, 185, getWidth() - 130, 290);
    bypass.setBounds(30, 495, 90, 28);
    status.setBounds(135, 498, getWidth() - 245, 24);
    leftMeterText.setBounds(getWidth() - 110, 520, 100, 18);
    rightMeterText.setBounds(getWidth() - 110, 540, 100, 18);
    rmsText.setBounds(getWidth() - 110, 560, 100, 18);

    const int deckY = 525;
    const int labelW = 135;
    const int startX = 110;
    for (int i = 0; i < 4; ++i)
    {
        const int x = startX + i * labelW;
        knobLabels[(size_t)i].setBounds(x, deckY - 8, labelW - 8, 18);
        knobs[(size_t)i].setBounds(x + 12, deckY + 12, 100, 105);
    }
}
