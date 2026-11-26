#include "PluginEditor.h"

namespace
{
    constexpr int panelModeCount = 5;
    const juce::StringArray vocalPresets = []
    {
        juce::StringArray a;
        const char* groups[4][10] = {
            {"Rap - Clean Punch", "Rap - Forward Lead", "Rap - Tight Modern", "Rap - Clear Articulation", "Rap - Low-Mid Control", "Rap - Bright Presence", "Rap - Smooth Top", "Rap - Dense Lead", "Rap - Natural Dynamic", "Rap - Studio Lead"},
            {"Melodic - Clean Lead", "Melodic - Smooth Tune", "Melodic - Bright Tune", "Melodic - Soft Presence", "Melodic - Tight Dynamics", "Melodic - Air Control", "Melodic - Warm Lead", "Melodic - Clear Hook", "Melodic - Balanced Lead", "Melodic - Modern Lead"},
            {"R&B - Silky Clean", "R&B - Warm Intimate", "R&B - Smooth Presence", "R&B - Breath Detail", "R&B - Rich Chest", "R&B - Soft Air", "R&B - Controlled Peaks", "R&B - Clean Studio", "R&B - Balanced Lead", "R&B - Polished Lead"},
            {"Pop - Radio Clean", "Pop - Clear Center", "Pop - Bright Presence", "Pop - Smooth Top", "Pop - Tight Lead", "Pop - Natural Body", "Pop - Clean Air", "Pop - Balanced Lead", "Pop - Dense Lead", "Pop - Modern Studio"}
        };
        for (const auto& group : groups) for (auto* name : group) a.add(name);
        return a;
    }();
    const juce::StringArray beatPresets = []
    {
        juce::StringArray a;
        const char* groups[7][10] = {
            {"Kick - Deep Trap", "Kick - Punch Attack", "Kick - Tight Short", "Kick - Warm Round", "Kick - Sub Focus", "Kick - Modern Knock", "Kick - Dry Studio", "Kick - Heavy Impact", "Kick - Clean Transient", "Kick - Balanced"},
            {"Snare - Clean Crack", "Snare - Tight Pop", "Snare - Body Punch", "Snare - Dark Punch", "Snare - Bright Snap", "Snare - Trap Rim", "Snare - Dry Knock", "Snare - Modern Snap", "Snare - Clean Studio", "Snare - Balanced"},
            {"Perc - Clean Groove", "Perc - Bright Tick", "Perc - Warm Hand", "Perc - Tight Transient", "Perc - Organic", "Perc - Afro Groove", "Perc - Dark Texture", "Perc - Punch Layer", "Perc - Clean Detail", "Perc - Balanced"},
            {"Hi-Hat - Crisp Trap", "Hi-Hat - Smooth Top", "Hi-Hat - Tight Dry", "Hi-Hat - Bright Roll", "Hi-Hat - Dark Control", "Hi-Hat - Open Air", "Hi-Hat - Soft Tick", "Hi-Hat - Modern Drill", "Hi-Hat - Clean Detail", "Hi-Hat - Balanced"},
            {"808 - Clean Sub", "808 - Tight Mono", "808 - Deep Round", "808 - Controlled Harmonics", "808 - Punch Bass", "808 - Dark Heavy", "808 - Smooth Sustain", "808 - Clean Drill", "808 - Balanced Low End", "808 - Studio Sub"},
            {"EFX - Riser Clean", "EFX - Riser Bright", "EFX - Uplifter Tight", "EFX - Downlifter", "EFX - Impact Punch", "EFX - Sweep Wide", "EFX - Transition Clean", "EFX - Reverse FX", "EFX - Noise Sweep", "EFX - Filler Control"},
            {"Melody - Clean Keys", "Melody - Piano Clear", "Melody - Synth Smooth", "Melody - Pluck Detail", "Melody - Pad Control", "Melody - Bell Bright", "Melody - Guitar Clean", "Melody - Wide Synth", "Melody - Warm Keys", "Melody - Balanced Music"}
        };
        for (const auto& group : groups) for (auto* name : group) a.add(name);
        return a;
    }();
}

void HFXSpectrumView::mouseDown(const juce::MouseEvent& e)
{
    if (displayMode != 0) return;
    const auto r = getLocalBounds().toFloat();
    float best = 1.0e9f;
    draggedBand = -1;
    for (int band = 0; band < 5; ++band)
    {
        const float t = std::log(processor.eqBandFrequency(band) / 20.0f) / std::log(20000.0f / 20.0f);
        const float x = r.getX() + t * r.getWidth();
        const float y = r.getCentreY() - processor.eqBandGain(band) * (r.getHeight() / 40.0f);
        const float d = e.position.getDistanceFrom({ x, y });
        if (d < best && d < 28.0f) { best = d; draggedBand = band; }
    }
    if (draggedBand >= 0)
    {
        if (onBandSelected) onBandSelected(draggedBand);
        updateEqFromPoint(e.position);
    }
}

void HFXSpectrumView::mouseDrag(const juce::MouseEvent& e)
{
    if (displayMode == 0 && draggedBand >= 0) updateEqFromPoint(e.position);
}

void HFXSpectrumView::updateEqFromPoint(juce::Point<float> point)
{
    const auto r = getLocalBounds().toFloat();
    const float t = juce::jlimit(0.0f, 1.0f, (point.x - r.getX()) / juce::jmax(1.0f, r.getWidth()));
    const float hz = 20.0f * std::pow(20000.0f / 20.0f, t);
    const float gain = juce::jlimit(-18.0f, 18.0f, (r.getCentreY() - point.y) * 40.0f / r.getHeight());
    processor.setEqBandFromGraph(draggedBand, hz, gain);
    repaint();
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
            const float t = (float)i / (float)(bins - 1);
            const float hz = 20.0f * std::pow(20000.0f / 20.0f, t);
            const float v = processor.spectrumAtFrequency(hz);
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
        constexpr float minHz = 20.0f, maxHz = 20000.0f;
        for (int i = 0; i < 240; ++i)
        {
            const float t = (float)i / 239.0f;
            const float hz = minHz * std::pow(maxHz / minHz, t);
            const float db = processor.eqResponseDbAt(hz);
            const float x = r.getX() + t * r.getWidth();
            const float y = r.getCentreY() - juce::jlimit(-18.0f, 18.0f, db) * (r.getHeight() / 40.0f);
            if (i == 0) response.startNewSubPath(x, y); else response.lineTo(x, y);
        }
        g.setColour(HFXColours::orange.withAlpha(0.95f));
        g.strokePath(response, juce::PathStrokeType(2.2f));
        for (int band = 0; band < 5; ++band)
        {
            const float t = std::log(processor.eqBandFrequency(band) / minHz) / std::log(maxHz / minHz);
            const float x = r.getX() + t * r.getWidth();
            const float y = r.getCentreY() - processor.eqBandGain(band) * (r.getHeight() / 40.0f);
            const bool selected = band == draggedBand;
            g.setColour(HFXColours::orange.withAlpha(selected ? 0.35f : 0.22f));
            g.fillEllipse(x - (selected ? 12.0f : 9.0f), y - (selected ? 12.0f : 9.0f),
                          selected ? 24.0f : 18.0f, selected ? 24.0f : 18.0f);
            g.setColour(HFXColours::orange); g.fillEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        }
    }
    else if (displayMode == 1) // Compressor: input/output transfer curve
    {
        const auto c = r.getCentre();
        juce::Path diagonal; diagonal.startNewSubPath(r.getX() + 16, r.getBottom() - 16); diagonal.lineTo(r.getRight() - 16, r.getY() + 16);
        g.setColour(HFXColours::muted.withAlpha(0.65f)); g.strokePath(diagonal, juce::PathStrokeType(1.2f));
        const float threshold = processor.compressorThresholdDb();
        const float ratio = juce::jmax(1.0f, processor.compressorRatio());
        juce::Path transfer;
        for (int i = 0; i <= 120; ++i)
        {
            const float inputDb = -60.0f + 60.0f * (float)i / 120.0f;
            const float over = inputDb - threshold;
            float outputDb = inputDb;
            if (over > 3.0f) outputDb = threshold + over / ratio;
            else if (over > -3.0f)
            {
                const float compressedOver = over + (1.0f / ratio - 1.0f) * (over + 3.0f) * (over + 3.0f) / 12.0f;
                outputDb = threshold + compressedOver;
            }
            const float x = r.getX() + (inputDb + 60.0f) / 60.0f * r.getWidth();
            const float y = r.getBottom() - ((juce::jlimit(-60.0f, 0.0f, outputDb) + 60.0f) / 60.0f) * r.getHeight();
            if (i == 0) transfer.startNewSubPath(x, y); else transfer.lineTo(x, y);
        }
        g.setColour(HFXColours::orange.withAlpha(0.9f));
        g.strokePath(transfer, juce::PathStrokeType(2.0f));
        const float thresholdNorm = juce::jlimit(0.0f, 1.0f, (threshold + 60.0f) / 60.0f);
        const float thresholdY = r.getBottom() - thresholdNorm * r.getHeight();
        g.setColour(HFXColours::orange.withAlpha(0.8f));
        g.drawHorizontalLine((int)thresholdY, r.getX() + 8.0f, r.getRight() - 8.0f);
        juce::Path liveWave;
        for (int i = 0; i < 256; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float)i / 255.0f;
            const float sample = juce::jlimit(-1.0f, 1.0f, processor.waveformAt(i));
            const float y = r.getCentreY() - sample * r.getHeight() * 0.43f;
            if (i == 0) liveWave.startNewSubPath(x, y); else liveWave.lineTo(x, y);
        }
        g.setColour(HFXColours::white.withAlpha(0.92f));
        g.strokePath(liveWave, juce::PathStrokeType(1.5f));
        g.setColour(HFXColours::orange); g.setFont(12.0f);
        g.drawText("LIVE INPUT  |  THRESHOLD " + juce::String(threshold, 1) + " dB  |  RATIO " + juce::String(processor.compressorRatio(), 1) + ":1  |  GR " + juce::String(processor.compressorGainReductionDb(), 1) + " dB", r.toNearestInt().reduced(12, 8), juce::Justification::topLeft);
    }
    else if (displayMode == 2) // Saturator: transfer curve
    {
        juce::Path curve;
        const float mid = r.getCentreY();
        for (int i = 0; i <= 120; ++i)
        {
            const float xNorm = (float)i / 120.0f * 2.0f - 1.0f;
            const float yNorm = processor.saturationTransfer(xNorm);
            const float x = r.getCentreX() + xNorm * r.getWidth() * 0.46f;
            const float y = mid - yNorm * r.getHeight() * 0.43f;
            if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
        }
        g.setColour(HFXColours::orange.withAlpha(0.2f)); g.strokePath(curve, juce::PathStrokeType(9.0f));
        g.setColour(HFXColours::orange); g.strokePath(curve, juce::PathStrokeType(2.5f));
        g.setColour(HFXColours::muted); g.drawText("SOFT CLIP / HARMONIC TRANSFER", r.toNearestInt().reduced(12, 8), juce::Justification::topLeft);
    }
    else if (displayMode == 3) // Polar stereo placement display
    {
        // Polar display: left/right occupy the lower arc; centre/mono is at the top.
        // The radius is derived from the measured Mid/Side sample magnitude.
        const float radius = juce::jmin(r.getWidth() * 0.43f, r.getHeight() * 0.78f);
        const float cx = r.getCentreX();
        const float baseY = r.getBottom() - 18.0f;
        const float pi = juce::MathConstants<float>::pi;

        g.setColour(HFXColours::muted);
        g.drawText("POLAR LEVEL", r.toNearestInt().withHeight(18).reduced(8, 0),
                   juce::Justification::centredTop);

        // Semi-circular polar grid.
        g.setColour(HFXColours::grid);
        for (float fraction : { 0.25f, 0.50f, 0.75f, 1.0f })
        {
            const float rr = radius * fraction;
            juce::Path arc;
            arc.addCentredArc(cx, baseY, rr, rr, 0.0f, pi, 2.0f * pi, true);
            g.strokePath(arc, juce::PathStrokeType(1.0f));
        }

        // Radial guides from the centre/top toward the left and right edges.
        for (int degrees : { 0, 30, 60, 90, 120, 150, 180 })
        {
            const float a = juce::degreesToRadians((float)degrees);
            const float x = cx + std::cos(a) * radius;
            const float y = baseY - std::sin(a) * radius;
            g.drawLine(cx, baseY, x, y, 0.8f);
        }

        g.setColour(HFXColours::muted);
        g.drawText("L", (int)(cx - radius - 16.0f), (int)(baseY - 3.0f), 18, 16,
                   juce::Justification::centredLeft);
        g.drawText("R", (int)(cx + radius - 2.0f), (int)(baseY - 3.0f), 18, 16,
                   juce::Justification::centredRight);

        // Frequency-dependent polar trace. Each FFT bin is assigned to its measured L/R position.
        juce::Path polarTrace, polarFill;
        bool traceStarted = false;
        for (int bin = 0; bin <= 180; ++bin)
        {
            const float level = processor.polarLevelAt(bin);
            const float angle = juce::MathConstants<float>::pi * (1.0f - (float)bin / 180.0f);
            const float x = cx + std::cos(angle) * level * radius;
            const float y = baseY - std::sin(angle) * level * radius;
            if (!traceStarted)
            {
                polarTrace.startNewSubPath(x, y);
                polarFill.startNewSubPath(cx, baseY);
                polarFill.lineTo(x, y);
                traceStarted = true;
            }
            else { polarTrace.lineTo(x, y); polarFill.lineTo(x, y); }
        }
        if (traceStarted)
        {
            polarFill.lineTo(cx, baseY);
            polarFill.closeSubPath();
            g.setColour(HFXColours::orange.withAlpha(0.16f)); g.fillPath(polarFill);
            g.setColour(HFXColours::orange.withAlpha(0.30f));
            g.strokePath(polarTrace, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(HFXColours::orange);
            g.strokePath(polarTrace, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float corr = processor.stereoCorrelation();
        g.setColour(HFXColours::muted);
        g.drawText("CORRELATION " + juce::String(corr, 2),
                   r.toNearestInt().reduced(10, 22).withHeight(16),
                   juce::Justification::topRight);
    }
    else // Limiter: peak history and ceiling line
    {
        juce::Path trace;
        for (int i = 0; i < 256; ++i)
        {
            const float x = r.getX() + 10.0f + (r.getWidth() - 20.0f) * (float)i / 255.0f;
            const float sample = juce::jlimit(-1.0f, 1.0f, processor.waveformAt(i));
            const float y = r.getCentreY() - sample * (r.getHeight() - 24.0f) * 0.48f;
            if (i == 0) trace.startNewSubPath(x, y); else trace.lineTo(x, y);
        }
        g.setColour(HFXColours::white); g.strokePath(trace, juce::PathStrokeType(1.5f));
        const float ceiling = juce::Decibels::decibelsToGain(processor.limiterCeilingDb());
        const float ceilingY = r.getCentreY() - ceiling * (r.getHeight() - 24.0f) * 0.48f;
        g.setColour(HFXColours::orange); g.drawHorizontalLine((int)ceilingY, r.getX() + 8, r.getRight() - 8);
        g.setColour(HFXColours::orange); g.drawText("LIVE OUTPUT  |  CEILING " + juce::String(processor.limiterCeilingDb(), 1) + " dB  |  GR " + juce::String(processor.limiterGainReductionDb(), 1) + " dB", r.toNearestInt().reduced(12, 8), juce::Justification::topLeft);
    }
}

HFXAudioProcessorEditor::HFXAudioProcessorEditor(HFXAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), spectrumView(p)
{
    setLookAndFeel(&laf);
    setSize(1120, 800);
    logo.setText("HFX", juce::dontSendNotification);
    logo.setFont(juce::Font(42.0f, juce::Font::bold));
    logo.setColour(juce::Label::textColourId, HFXColours::orange);
    addAndMakeVisible(logo);
    logo.setVisible(false); // logo is rendered with the multi-pass glow in paint()

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
    stemRole.addItem("MELODY", 8);
    addAndMakeVisible(stemRole);
    stemRoleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state(), "stemRole", stemRole);
    stemRole.onChange = [this] { refreshPresetChoices(); };
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

    autoMix.onClick = [this]
    {
        autoMixWaitTicks = 80;
        processor.requestAutoMix();
        status.setText("AUTO-MIX ANALYSIS STARTED — PLAY AUDIO FOR 4 SECONDS", juce::dontSendNotification);
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "HFX AUTO-MIX",
            "Analyzing your incoming audio. Keep playback running for 4 seconds. HFX will then prepare a suggested starting mix.");
    };
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
    eqSoloButton.setClickingTogglesState(true);
    eqSoloButton.onClick = [this]
    {
        processor.setEqSolo(eqSoloButton.getToggleState(), selectedEqBand);
        eqSoloButton.setButtonText(eqSoloButton.getToggleState() ? "SOLO ON" : "SOLO SELECTED BAND");
    };
    eqSoloButton.setColour(juce::TextButton::buttonOnColourId, HFXColours::orange);
    addAndMakeVisible(eqSoloButton);
    spectrumView.onBandSelected = [this](int band)
    {
        selectedEqBand = juce::jlimit(0, 4, band);
        if (eqSoloButton.getToggleState()) processor.setEqSolo(true, selectedEqBand);
        rebuildAttachments();
        resized();
    };
    for (int i = 0; i < 10; ++i)
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
    for (auto* meter : { &leftMeterText, &rightMeterText, &rmsText })
    {
        meter->setFont(juce::Font(9.5f, juce::Font::bold));
        meter->setJustificationType(juce::Justification::centredRight);
    }
    addAndMakeVisible(leftMeterText); addAndMakeVisible(rightMeterText); addAndMakeVisible(rmsText);
    rebuildAttachments();
    startTimerHz(30);
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
    if (beat)
    {
        const int category = juce::jlimit(0, 6, stemRole.getSelectedId() - 1);
        const int first = category * 10;
        for (int i = 0; i < 10; ++i)
            presetBox.addItem(choices[first + i], first + i + 1);
        presetBox.setText(stemRole.getText() + " MIX PRESETS", juce::dontSendNotification);
    }
    else
    {
        for (int i = 0; i < choices.size(); ++i) presetBox.addItem(choices[i], i + 1);
        presetBox.setText("VOCAL MIX PRESETS", juce::dontSendNotification);
    }
    presetBox.onChange = [this] { if (presetBox.getSelectedId() > 0) applyPreset(presetBox.getSelectedId()); };
}

void HFXAudioProcessorEditor::selectTab(int index)
{
    activeTab = juce::jlimit(0, 4, index);
    if (activeTab != 0 && eqSoloButton.getToggleState())
    {
        eqSoloButton.setToggleState(false, juce::dontSendNotification);
        eqSoloButton.setButtonText("SOLO SELECTED BAND");
        processor.setEqSolo(false, selectedEqBand);
    }
    for (int i = 0; i < panelModeCount; ++i)
        tabs[(size_t)i].setToggleState(i == activeTab, juce::dontSendNotification);
    spectrumView.setDisplayMode(activeTab);
    eqSoloButton.setVisible(activeTab == 0);
    rebuildAttachments();
    resized();
    repaint();
}

void HFXAudioProcessorEditor::rebuildAttachments()
{
    for (auto& attachment : attachments) attachment.reset();
    for (int i = 0; i < 10; ++i)
    {
        const int visibleCount = (activeTab == 2 || activeTab == 3) ? 3 : 4;
        const bool used = i < visibleCount;
        knobLabels[(size_t)i].setVisible(used);
        knobs[(size_t)i].setVisible(used);
        if (!used) continue;

        juce::String parameterId;
        if (activeTab == 0)
        {
            const juce::String suffix = juce::String(selectedEqBand);
            if (i == 0) parameterId = "eqBandFreq" + suffix;
            else if (i == 1) parameterId = "eqBandGain" + suffix;
            else if (i == 2) parameterId = "eqBandQ" + suffix;
            else parameterId = "eqBandDynamic" + suffix;
        }
        else
            parameterId = ids[(size_t)activeTab][(size_t)i];

        knobLabels[(size_t)i].setText(names[(size_t)activeTab][(size_t)i],
                                       juce::dontSendNotification);
        attachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state(), parameterId, knobs[(size_t)i]);
        if (activeTab == 3)
        {
            knobs[(size_t)i].textFromValueFunction = [](double value) { return juce::String((int)std::round(value * 100.0)) + "%"; };
            knobs[(size_t)i].valueFromTextFunction = [](const juce::String& text) { return text.getDoubleValue() / 100.0; };
        }
    }
}

void HFXAudioProcessorEditor::applyPreset(int id)
{
    // HFX curated starting profiles. These are starting points, not a promise
    // of a finished mix: source level, mic, room, arrangement and performance vary.
    struct Profile
    {
        float freq[5];
        float gain[5];
        float q[5];
        float threshold, ratio, attack, release;
        float drive, tone, satMix, width;
        float input, ceiling, limiterRelease, transient;
    };

    const bool beat = trackMode.getSelectedId() == 2;
    const int safeId = juce::jlimit(1, beat ? 70 : 40, id);
    const int category = (safeId - 1) / 10;
    const int variation = (safeId - 1) % 10;

    // Vocal groups: Rap, Melodic, R&B, Pop, Creative FX.
    // Beat groups: Kick, Snare, Percussion, Hi-hat, 808.
    static const Profile vocal[5] =
    {
        // Rap: controlled low-mid, intelligibility, firm but not crushed dynamics.
        {{85, 250, 1100, 4200, 12500}, {-1.0f, -1.8f, 0.4f, 1.2f, 0.5f}, {0.80f, 1.10f, 0.90f, 0.85f, 0.70f},
         -22.0f, 3.0f, 18.0f, 105.0f, 1.2f, 0.10f, 0.16f, 1.02f, -1.0f, -1.2f, 85.0f, 0.05f},
        // Melodic: smoother upper mids, controlled peaks, restrained saturation.
        {{90, 280, 1800, 5200, 14500}, {-0.8f, -1.0f, -0.5f, 0.8f, 0.8f}, {0.75f, 0.90f, 0.80f, 0.75f, 0.65f},
         -24.0f, 2.6f, 24.0f, 135.0f, 0.8f, 0.18f, 0.12f, 1.06f, -1.0f, -1.3f, 100.0f, 0.02f},
        // R&B: gentle body and air, slower compression.
        {{75, 220, 900, 3800, 15000}, {-0.5f, -0.8f, 0.2f, 0.5f, 1.0f}, {0.70f, 0.85f, 0.80f, 0.75f, 0.65f},
         -25.0f, 2.2f, 28.0f, 155.0f, 0.6f, 0.12f, 0.10f, 1.08f, -1.5f, -1.3f, 120.0f, 0.00f},
        // Pop: clean presence and controlled top end.
        {{100, 300, 1600, 4500, 13500}, {-1.2f, -1.0f, 0.2f, 1.0f, 0.8f}, {0.80f, 0.95f, 0.90f, 0.80f, 0.70f},
         -23.0f, 2.8f, 16.0f, 115.0f, 0.9f, 0.08f, 0.13f, 1.04f, -1.0f, -1.2f, 90.0f, 0.03f},
        // Clean vocal: neutral, controlled dynamics and restrained processing.
        {{90, 260, 1200, 4200, 13500}, {-0.8f, -1.0f, 0.1f, 0.7f, 0.5f}, {0.75f, 0.90f, 0.85f, 0.80f, 0.70f},
         -24.0f, 2.5f, 22.0f, 125.0f, 0.5f, 0.05f, 0.10f, 1.00f, -1.5f, -1.2f, 100.0f, 0.02f}
    };

    static const Profile beatProfiles[7] =
    {
        // Kick: sub foundation, controlled click and transient.
        {{45, 90, 250, 2500, 9000}, {1.0f, -0.8f, -1.5f, 0.7f, -0.5f}, {0.75f, 0.90f, 1.00f, 0.85f, 0.70f},
         -15.0f, 3.0f, 28.0f, 95.0f, 1.8f, -0.05f, 0.18f, 1.00f, -1.0f, -1.0f, 75.0f, 0.12f},
        // Snare: body and crack without extreme top boost.
        {{120, 220, 700, 4200, 11000}, {-0.8f, 0.8f, -0.5f, 1.2f, 0.4f}, {0.80f, 0.90f, 0.90f, 0.80f, 0.75f},
         -17.0f, 3.2f, 12.0f, 110.0f, 1.6f, 0.08f, 0.18f, 1.08f, -1.0f, -1.0f, 90.0f, 0.15f},
        // Percussion: transient detail and modest width.
        {{160, 350, 1200, 6500, 14000}, {-0.5f, -0.5f, 0.3f, 0.8f, 0.5f}, {0.75f, 0.90f, 0.85f, 0.75f, 0.70f},
         -20.0f, 2.4f, 8.0f, 85.0f, 0.8f, 0.12f, 0.10f, 1.12f, -1.0f, -1.0f, 85.0f, 0.10f},
        // Hi-hat: high-pass cannot be configured in this five-bell EQ; conservative high-band shaping.
        {{250, 700, 2500, 7500, 15000}, {-1.0f, -0.8f, 0.0f, 0.8f, -0.3f}, {0.75f, 0.85f, 0.85f, 0.75f, 0.70f},
         -24.0f, 2.0f, 2.0f, 65.0f, 0.5f, -0.08f, 0.08f, 1.10f, -1.0f, -1.0f, 65.0f, 0.05f},
        // 808: mono-safe low end and restrained drive.
        {{35, 65, 140, 700, 4500}, {0.8f, 0.3f, -1.0f, 0.5f, -0.5f}, {0.70f, 0.85f, 0.95f, 0.90f, 0.75f},
         -18.0f, 2.6f, 35.0f, 145.0f, 2.0f, -0.12f, 0.20f, 0.92f, -1.0f, -1.0f, 110.0f, 0.04f},
        // EFX: conservative tonal and peak control for risers, sweeps and impacts.
        {{120, 350, 1200, 5000, 12000}, {-1.0f, -0.5f, 0.0f, 0.5f, -0.5f}, {0.80f, 0.90f, 0.90f, 0.85f, 0.75f},
         -20.0f, 2.0f, 8.0f, 100.0f, 0.6f, 0.0f, 0.08f, 1.10f, -2.0f, -1.5f, 100.0f, 0.08f},
        // Melody: balanced harmonic content, gentle dynamics and modest width.
        {{100, 280, 1400, 4800, 14000}, {-0.8f, -0.8f, 0.0f, 0.6f, 0.4f}, {0.75f, 0.90f, 0.85f, 0.80f, 0.70f},
         -22.0f, 2.2f, 25.0f, 140.0f, 0.5f, 0.08f, 0.10f, 1.08f, -1.5f, -1.3f, 110.0f, 0.02f}
    };

    const auto& p = beat ? beatProfiles[category] : vocal[category];
    const float v = (float)variation / 9.0f;

    auto set = [this](const juce::String& key, float value)
    {
        if (auto* parameter = processor.state().getParameter(key))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };

    // Each named preset is a variation of its instrument/voice-family base profile.
    // Variation is deliberately restrained to avoid extreme, source-dependent settings.
    const float eqVariation = (v - 0.5f) * (beat ? 0.65f : 0.55f);
    for (int band = 0; band < 5; ++band)
    {
        const float freqScale = 1.0f + (v - 0.5f) * (band == 0 ? -0.08f : band == 4 ? 0.10f : 0.04f);
        const float bandVariation = (band == 0 ? -0.25f : band == 4 ? 0.25f : 0.10f) * eqVariation;
        set("eqBandFreq" + juce::String(band), juce::jlimit(20.0f, 20000.0f, p.freq[band] * freqScale));
        set("eqBandGain" + juce::String(band), juce::jlimit(-6.0f, 6.0f, p.gain[band] + bandVariation));
        set("eqBandQ" + juce::String(band), p.q[band]);
        set("eqBandDynamic" + juce::String(band), 0.0f);
    }

    set("threshold", p.threshold + (v - 0.5f) * (beat ? 2.0f : 1.5f));
    set("ratio", juce::jlimit(1.0f, 8.0f, p.ratio + (v - 0.5f) * 0.5f));
    set("attack", p.attack * (0.90f + 0.20f * v));
    set("release", p.release * (0.90f + 0.20f * v));
    set("drive", juce::jlimit(0.0f, 4.0f, p.drive * (0.85f + 0.30f * v)));
    set("tone", juce::jlimit(-1.0f, 1.0f, p.tone + (v - 0.5f) * 0.12f));
    set("satMix", juce::jlimit(0.0f, 0.30f, p.satMix * (0.85f + 0.30f * v)));
    set("width", juce::jlimit(0.0f, 1.35f, p.width + (v - 0.5f) * 0.08f));
    set("inputGain", juce::jlimit(-3.0f, 0.0f, p.input + (0.5f - v) * 0.25f));
    set("ceiling", juce::jlimit(-2.0f, -1.0f, p.ceiling));
    set("limitRelease", p.limiterRelease);
    set("transient", p.transient);

    // Preserve compatibility parameters, but neutralize unused legacy pan/mono/Haas controls.
    set("pan", 0.0f);
    set("leftPan", 0.0f);
    set("rightPan", 0.0f);
    set("monoFreq", 120.0f);
    set("haasMs", 0.0f);

    const auto& names = beat ? beatPresets : vocalPresets;
    const auto presetName = names[safeId - 1];
    status.setText(presetName + " LOADED — CLEAN STARTING MIX; CHECK LEVELS",
                   juce::dontSendNotification);
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
    rmsText.setText("OUT RMS  " + juce::String(processor.outputRmsDb(), 1) + " dBFS", juce::dontSendNotification);
    if (autoMixWaitTicks > 0 && !processor.autoMixReady())
    {
        --autoMixWaitTicks;
        const int secondsLeft = (autoMixWaitTicks + 19) / 20;
        status.setText("ANALYZING AUDIO — " + juce::String(secondsLeft) + " SEC", juce::dontSendNotification);
    }
    repaint();
    if (processor.autoMixReady())
    {
        const auto result = processor.takeAutoMixResult();
        auto set = [this](const char* key, float value)
        {
            if (auto* parameter = processor.state().getParameter(key)) parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        set("eqBandGain0", result.eqLow); set("eqBandGain1", result.eqLowMid);
        set("eqBandGain2", result.eqMid); set("eqBandGain3", result.eqHighMid); set("eqBandGain4", result.eqHigh);
        set("threshold", result.threshold); set("drive", result.drive); set("width", result.width); set("inputGain", result.inputGain);
        status.setText("AUTO-MIX SUGGESTIONS APPLIED  |  LOW " + juce::String(result.eqLow, 1) + " dB  |  MID " + juce::String(result.eqMid, 1) + " dB  |  HIGH " + juce::String(result.eqHigh, 1) + " dB", juce::dontSendNotification);
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
    for (int pass = 12; pass >= 1; --pass)
    {
        g.setColour(HFXColours::orange.withAlpha(0.035f * (float)(13 - pass)));
        g.drawText("HFX", 22 - pass, 13 - pass, 120 + pass * 2, 58 + pass * 2, juce::Justification::left);
    }
    for (int pass = 12; pass >= 1; --pass)
    { g.setColour(HFXColours::orange.withAlpha(0.035f * (float)(13 - pass))); g.setFont(juce::Font(34.0f, juce::Font::bold)); g.drawText("HF", 28 - pass, getHeight() - 105 - pass, 62 + pass * 2, 48 + pass * 2, juce::Justification::centred); }
    g.setColour(HFXColours::orange); g.setFont(juce::Font(34.0f, juce::Font::bold)); g.drawText("HF", 28, getHeight() - 105, 62, 48, juce::Justification::centred);

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
    for (int i = 0; i < panelModeCount; ++i)
        tabs[(size_t)i].setBounds(20 + i * (tabWidth + tabGap), tabY, tabWidth, tabHeight);

    const int graphHeight = 320;
    spectrumView.setBounds(30, 185, getWidth() - 130, graphHeight);
    const int graphBottom = 185 + graphHeight;
    const int utilityY = graphBottom + 8;
    bypass.setBounds(30, utilityY, 82, 22);
    eqSoloButton.setBounds(120, utilityY, 142, 22);
    eqSoloButton.setVisible(activeTab == 0);
    status.setBounds(275, utilityY, juce::jmax(180, getWidth() - 565), 22);
    const int meterX = getWidth() - 282;
    leftMeterText.setBounds(meterX, utilityY, 252, 14);
    rightMeterText.setBounds(meterX, utilityY + 14, 252, 14);
    rmsText.setBounds(meterX, utilityY + 28, 252, 14);

    const int available = getWidth() - 210;
    const int cell = available / 4;
    const int startX = 105;
    for (int i = 0; i < 10; ++i)
    {
        const bool used = i < 4;
        if (!used)
        {
            knobLabels[(size_t)i].setBounds(0, 0, 0, 0);
            knobs[(size_t)i].setBounds(0, 0, 0, 0);
            continue;
        }
        const int x = startX + i * cell;
        const int y = graphBottom + 48;
        knobLabels[(size_t)i].setBounds(x, y, cell - 8, 16);
        knobs[(size_t)i].setBounds(x + (cell - 84) / 2, y + 14, 84, 84);
    }
}
