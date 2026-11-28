#include "PluginEditor.h"

namespace
{
    constexpr int panelModeCount = 7;
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
        const char* groups[8][10] = {
            {"Kick - Deep Trap", "Kick - Punch Attack", "Kick - Tight Short", "Kick - Warm Round", "Kick - Sub Focus", "Kick - Modern Knock", "Kick - Dry Studio", "Kick - Heavy Impact", "Kick - Clean Transient", "Kick - Balanced"},
            {"Snare - Clean Crack", "Snare - Tight Pop", "Snare - Body Punch", "Snare - Dark Punch", "Snare - Bright Snap", "Snare - Trap Rim", "Snare - Dry Knock", "Snare - Modern Snap", "Snare - Clean Studio", "Snare - Balanced"},
            {"Perc - Clean Groove", "Perc - Bright Tick", "Perc - Warm Hand", "Perc - Tight Transient", "Perc - Organic", "Perc - Afro Groove", "Perc - Dark Texture", "Perc - Punch Layer", "Perc - Clean Detail", "Perc - Balanced"},
            {"Closed Hat - Crisp Trap", "Closed Hat - Smooth Top", "Closed Hat - Tight Dry", "Closed Hat - Bright Roll", "Closed Hat - Dark Control", "Closed Hat - Soft Tick", "Closed Hat - Modern Drill", "Closed Hat - Clean Detail", "Closed Hat - Balanced", "Closed Hat - Studio"},
            {"Open Hat - Bright Air", "Open Hat - Smooth", "Open Hat - Wide Lift", "Open Hat - Tight", "Open Hat - Dark Control", "Open Hat - Clean Tail", "Open Hat - Modern Trap", "Open Hat - Soft Air", "Open Hat - Crisp", "Open Hat - Balanced"},
            {"EFX - Riser Clean", "EFX - Riser Bright", "EFX - Uplifter Tight", "EFX - Downlifter", "EFX - Impact Punch", "EFX - Sweep Wide", "EFX - Transition Clean", "EFX - Reverse FX", "EFX - Noise Sweep", "EFX - Filler Control"},
            {"808 - Clean Sub", "808 - Tight Mono", "808 - Deep Round", "808 - Controlled Harmonics", "808 - Punch Bass", "808 - Dark Heavy", "808 - Smooth Sustain", "808 - Clean Drill", "808 - Balanced Low End", "808 - Studio Sub"},
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

    if (displayMode == 0)
    {
        g.setColour(HFXColours::muted.withAlpha(0.78f));
        g.setFont(10.0f);
        const char* freqLabels[] { "20", "50", "100", "500", "1k", "5k", "10k", "20k" };
        const float freqValues[] { 20.0f, 50.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f };
        for (int i = 0; i < 8; ++i)
        {
            const float t = std::log(freqValues[i] / 20.0f) / std::log(20000.0f / 20.0f);
            const float x = r.getX() + r.getWidth() * t;
            g.drawText(freqLabels[i], (int)x - 18, (int)r.getBottom() - 16, 36, 12, juce::Justification::centred);
        }
        g.drawText("+18", 4, (int)r.getY() + 2, 28, 12, juce::Justification::left);
        g.drawText("0", 4, (int)r.getCentreY() - 6, 28, 12, juce::Justification::left);
        g.drawText("-18", 4, (int)r.getBottom() - 18, 28, 12, juce::Justification::left);
    }

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
    stemRole.onChange = [this] { refreshPresetChoices(); updateTabVisibility(); };
    stemRole.setVisible(trackMode.getSelectedId() == 2);
    trackMode.onChange = [this]
    {
        const bool beatMode = trackMode.getSelectedId() == 2;
        stemRole.setVisible(beatMode);
        trackMode.setVisible(true);
        presetBox.setVisible(true);
        autoMix.setVisible(true);
        bypass.setVisible(true);
        status.setVisible(true);
        leftMeterText.setVisible(true);
        rightMeterText.setVisible(true);
        rmsText.setVisible(true);
        refreshPresetChoices();
        updateTabVisibility();
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

    sidechainButton.setClickingTogglesState(true);
    sidechainButton.setColour(juce::TextButton::buttonOnColourId, HFXColours::orange);
    sidechainButton.onClick = [this]
    {
        sidechainButton.setButtonText(sidechainButton.getToggleState() ? "SIDECHAIN ON" : "SIDECHAIN OFF");
    };
    addAndMakeVisible(sidechainButton);
    sidechainButton.setVisible(false);
    sidechainButton.setTooltip("Enable external sidechain detection. Select the source track in Ableton Live's Sidechain routing menu.");
    sidechainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.state(), "sidechainEnabled", sidechainButton);

    const juce::String tabNames[] { "EQ", "COMPRESSOR", "SATURATOR", "PAN & WIDTH", "LIMITER", "UTILITY", "DRUM BUS" };
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
    updateTabVisibility();
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
        const int category = juce::jlimit(0, 7, stemRole.getSelectedId() - 1);
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

bool HFXAudioProcessorEditor::drumBusEligible() const noexcept
{
    return trackMode.getSelectedId() == 2 && stemRole.getSelectedId() >= 1
        && stemRole.getSelectedId() <= 6;
}

void HFXAudioProcessorEditor::updateTabVisibility()
{
    const bool drumOnly = drumBusEligible();
    const bool bassMelodyOnly = trackMode.getSelectedId() == 2
        && (stemRole.getSelectedId() == 7 || stemRole.getSelectedId() == 8);
    for (int i = 0; i < panelModeCount; ++i)
    {
        const bool visible = drumOnly ? (i == 0 || i == 3 || i == 6)
            : (bassMelodyOnly ? (i >= 0 && i <= 3) : (i < 6));
        tabs[(size_t)i].setVisible(visible);
    }
    if (drumOnly && activeTab != 0 && activeTab != 3 && activeTab != 6)
        activeTab = 0;
    if (bassMelodyOnly && activeTab > 3)
        activeTab = 0;
    if (!drumOnly && activeTab == 6)
        activeTab = 0;
    for (int i = 0; i < panelModeCount; ++i)
        tabs[(size_t)i].setToggleState(i == activeTab, juce::dontSendNotification);
    spectrumView.setVisible(activeTab != 5 && activeTab != 6);
    eqSoloButton.setVisible(activeTab == 0);
    sidechainButton.setVisible(activeTab == 1 && !drumOnly);
    rebuildAttachments();
}

void HFXAudioProcessorEditor::selectTab(int index)
{
    const bool drumOnly = drumBusEligible();
    const bool bassMelodyOnly = trackMode.getSelectedId() == 2
        && (stemRole.getSelectedId() == 7 || stemRole.getSelectedId() == 8);
    const int maxTab = drumOnly ? 6 : (bassMelodyOnly ? 3 : 5);
    activeTab = juce::jlimit(0, maxTab, index);
    if (drumOnly && activeTab != 0 && activeTab != 3 && activeTab != 6)
        activeTab = 0;
    if (activeTab != 0 && eqSoloButton.getToggleState())
    {
        eqSoloButton.setToggleState(false, juce::dontSendNotification);
        eqSoloButton.setButtonText("SOLO SELECTED BAND");
        processor.setEqSolo(false, selectedEqBand);
    }
    for (int i = 0; i < panelModeCount; ++i)
        tabs[(size_t)i].setToggleState(i == activeTab, juce::dontSendNotification);
    spectrumView.setDisplayMode(activeTab);
    spectrumView.setVisible(activeTab != 5 && activeTab != 6);
    trackMode.setVisible(true);
    stemRole.setVisible(trackMode.getSelectedId() == 2);
    presetBox.setVisible(true);
    autoMix.setVisible(true);
    bypass.setVisible(true);
    status.setVisible(true);
    leftMeterText.setVisible(true);
    rightMeterText.setVisible(true);
    rmsText.setVisible(true);
    eqSoloButton.setVisible(activeTab == 0);
    sidechainButton.setVisible(activeTab == 1 && !drumBusEligible());
    rebuildAttachments();
    resized();
    repaint();
}

void HFXAudioProcessorEditor::rebuildAttachments()
{
    for (auto& attachment : attachments) attachment.reset();
    for (int i = 0; i < 10; ++i)
    {
        const int visibleCount = activeTab == 5 ? 1 : ((activeTab == 2 || activeTab == 3) ? 3 : 4);
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
    const bool beat = trackMode.getSelectedId() == 2;
    const int safeId = juce::jlimit(1, beat ? 80 : 40, id);
    const int category = (safeId - 1) / 10;
    const int variation = (safeId - 1) % 10;
    const float v = (float)variation / 9.0f;
    const float centered = v - 0.5f;
    auto set = [this](const juce::String& key, float value)
    {
        if (auto* parameter = processor.state().getParameter(key))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };

    float freq[5] { 90.0f, 260.0f, 1100.0f, 4300.0f, 13500.0f };
    float gain[5] { 0.0f, -2.0f, -0.5f, 2.0f, 1.4f };
    float q[5] { 0.707f, 1.0f, 1.15f, 0.85f, 0.707f };
    int type[5] { 1, 3, 3, 3, 5 };

    float threshold = -29.8f, ratio = 4.0f, attack = 5.0f, release = 60.0f;
    float drive = 2.0f, tone = 0.0f, satMix = 0.25f, width = 1.0f;
    float input = 11.0f, ceiling = -1.0f, limiterRelease = 300.0f, transient = 0.0f;
    float panL = 0.0f, panR = 0.0f;
    float utility = 6.0f;
    float drumDrive = 3.0f, drumDamp = 9200.0f, drumBoom = 0.0f;

    if (!beat)
    {
        // Vocal guide: controlled low end and low-mid, forward presence and a restrained air shelf.
        const float familyTilt = (category == 1 ? 0.6f : category == 2 ? -0.6f : category == 3 ? 0.3f : 0.0f);
        freq[0] = category == 2 ? 75.0f : 90.0f;
        freq[1] = 260.0f; freq[2] = 1100.0f; freq[3] = 4300.0f; freq[4] = 13500.0f;
        gain[0] = 0.0f; gain[1] = -2.2f; gain[2] = -0.5f + familyTilt;
        gain[3] = 2.0f; gain[4] = 1.4f;
        type[0] = 1; type[1] = 3; type[2] = 3; type[3] = 3; type[4] = 5;
        threshold = -29.8f; ratio = 4.0f; attack = 5.0f; release = 60.0f;
        drive = 2.0f; satMix = 0.25f; width = 1.0f;
        input = 11.0f; ceiling = -1.0f; limiterRelease = 300.0f; utility = 6.0f;
    }
    else if (category <= 5)
    {
        // Percussive stems use EQ + Pan/Width + Drum Bus only. The audio engine
        // bypasses Compressor, Saturator, Utility and Limiter for these six roles.
        threshold = 0.0f; ratio = 1.0f; attack = 10.0f; release = 100.0f;
        drive = 0.0f; satMix = 0.0f; input = 0.0f; utility = 0.0f;
        ceiling = 0.0f; limiterRelease = 80.0f;

        switch (category)
        {
            case 0: // Kick: sub weight, focused punch, remove unnecessary upper range.
                freq[0]=25; freq[1]=75; freq[2]=260; freq[3]=3500; freq[4]=12500;
                gain[0]=0; gain[1]=1.8f; gain[2]=-3.2f; gain[3]=1.2f; gain[4]=-1.0f;
                type[0]=0; type[1]=2; type[2]=3; type[3]=3; type[4]=6;
                q[2]=1.25f; width=1.0f;
                break;
            case 1: // Snare: body, box control and crack; guide width 40%.
                freq[0]=206; freq[1]=300; freq[2]=850; freq[3]=4200; freq[4]=14500;
                gain[0]=0; gain[1]=1.8f; gain[2]=-2.4f; gain[3]=2.4f; gain[4]=0.8f;
                type[0]=1; type[1]=3; type[2]=3; type[3]=3; type[4]=5;
                width=0.40f;
                break;
            case 2: // Percussion: remove lows, retain transient detail; alternating L/R.
                freq[0]=417; freq[1]=700; freq[2]=2200; freq[3]=6200; freq[4]=15500;
                gain[0]=0; gain[1]=-2.8f; gain[2]=0.8f; gain[3]=2.0f; gain[4]=1.0f;
                type[0]=1; type[1]=3; type[2]=3; type[3]=3; type[4]=5;
                width=0.10f; if (variation % 2 == 0) panL=-0.20f; else panR=0.20f;
                break;
            case 3: // Closed hat: high-pass and high detail; 20% left, 10% width.
                freq[0]=300; freq[1]=1500; freq[2]=4300; freq[3]=9000; freq[4]=17500;
                gain[0]=0; gain[1]=-1.8f; gain[2]=0.5f; gain[3]=2.2f; gain[4]=1.0f;
                type[0]=0; type[1]=3; type[2]=3; type[3]=3; type[4]=5;
                width=0.10f; panL=-0.20f;
                break;
            case 4: // Open hat: brighter top and controlled body; 20% right, 10% width.
                freq[0]=979; freq[1]=1800; freq[2]=4200; freq[3]=8500; freq[4]=16500;
                gain[0]=0; gain[1]=-1.5f; gain[2]=0.4f; gain[3]=1.8f; gain[4]=2.0f;
                type[0]=0; type[1]=3; type[2]=3; type[3]=3; type[4]=5;
                width=0.10f; panR=0.20f;
                break;
            default: // EFX: remove rumble, shape mid/high transition detail, wide alternating pan.
                freq[0]=140; freq[1]=420; freq[2]=1500; freq[3]=5800; freq[4]=16500;
                gain[0]=0; gain[1]=-2.2f; gain[2]=0.7f; gain[3]=2.0f; gain[4]=1.3f;
                type[0]=1; type[1]=3; type[2]=3; type[3]=3; type[4]=5;
                width=1.0f; if (variation % 2 == 0) panL=-0.40f; else panR=0.40f;
                break;
        }

        // Ten deliberately audible voicings per drum role: wider frequency,
        // EQ gain, Q and Drum Bus variation than the earlier micro-adjustments.
        const float freqScale[10] { 0.72f, 0.80f, 0.88f, 0.94f, 1.0f, 1.07f, 1.15f, 1.25f, 1.38f, 1.52f };
        const float gainShift[10] { -2.4f, -1.8f, -1.2f, -0.6f, 0.0f, 0.6f, 1.2f, 1.8f, 2.4f, 3.0f };
        const float driveValues[6][10] {
            { 5, 3, 7, 9, 2, 11, 6, 13, 4, 8 },       // Kick
            { 3, 5, 7, 2, 9, 4, 11, 6, 13, 8 },       // Snare
            { 3, 4, 6, 2, 8, 5, 10, 7, 12, 1 },       // Percussion
            { 2, 3, 4, 1, 5, 2, 6, 3, 8, 1 },         // Closed hat
            { 3, 4, 5, 2, 6, 3, 8, 4, 10, 1 },       // Open hat
            { 3, 5, 7, 2, 9, 4, 11, 6, 13, 8 }        // EFX
        };
        const float dampValues[6][10] {
            { 7180, 6000, 8500, 10000, 5000, 12000, 9200, 4000, 15000, 7800 },
            { 9200, 11000, 7500, 14000, 6000, 10000, 5000, 16000, 8200, 12500 },
            { 9200, 12000, 7000, 15000, 5500, 10000, 6500, 18000, 8000, 13000 },
            { 9200, 12000, 7500, 15000, 6000, 10500, 5000, 17000, 8000, 13000 },
            { 9200, 13000, 7500, 16000, 6000, 11000, 5000, 18000, 8500, 14000 },
            { 9200, 12000, 7000, 15000, 5500, 10000, 6500, 18000, 8000, 13000 }
        };
        const float boomValues[6][10] {
            { 10, 0, 15, 5, 20, 8, 12, 3, 25, 10 },
            { 0, 5, 0, 8, 0, 12, 3, 0, 15, 5 },
            { 0, 3, 0, 5, 0, 8, 2, 0, 10, 4 },
            { 0, 0, 2, 0, 4, 0, 1, 0, 5, 0 },
            { 0, 2, 0, 4, 0, 6, 1, 0, 8, 0 },
            { 0, 5, 0, 8, 0, 12, 3, 0, 15, 5 }
        };
        const float boomFreqValues[6][10] {
            { 50, 45, 55, 60, 40, 70, 80, 35, 90, 65 },
            { 50, 55, 45, 60, 40, 70, 80, 35, 90, 65 },
            { 50, 55, 45, 60, 40, 70, 80, 35, 90, 65 },
            { 50, 55, 45, 60, 40, 70, 80, 35, 90, 65 },
            { 50, 55, 45, 60, 40, 70, 80, 35, 90, 65 },
            { 50, 55, 45, 60, 40, 70, 80, 35, 90, 65 }
        };
        for (int band = 0; band < 5; ++band)
        {
            const float bandScale = band == 0 ? freqScale[variation] : std::pow(freqScale[variation], band == 4 ? 0.65f : 0.85f);
            freq[band] *= bandScale;
            if (band >= 1 && band <= 3)
                gain[band] += gainShift[variation] * (band == 2 ? -0.75f : 0.75f);
            else if (band == 4)
                gain[band] += gainShift[variation] * 0.45f;
            q[band] *= (variation % 2 == 0 ? 0.82f : 1.22f);
        }
        drumDrive = driveValues[category][variation];
        drumDamp = dampValues[category][variation];
        drumBoom = boomValues[category][variation];

    }
    else
    {
        // 808/Bass and Melody retain the full dynamics, saturation and output chain.
        threshold = -29.8f; ratio = 4.0f; attack = 5.0f; release = 60.0f;
        input = 11.0f; ceiling = -1.0f; limiterRelease = 300.0f; utility = 6.0f;
        if (category == 6) // 808 / Bass
        {
            freq[0]=25; freq[1]=58; freq[2]=160; freq[3]=650; freq[4]=5200;
            gain[0]=0; gain[1]=1.8f; gain[2]=-2.2f; gain[3]=-1.8f; gain[4]=0;
            type[0]=0; type[1]=2; type[2]=3; type[3]=3; type[4]=6;
            width=1.0f; drive=8.0f; satMix=1.0f;
        }
        else // Melody: mid/high clarity and three guide-based stereo profiles.
        {
            freq[0]=120; freq[1]=340; freq[2]=1400; freq[3]=4800; freq[4]=15500;
            gain[0]=0; gain[1]=-2.4f; gain[2]=0.8f; gain[3]=2.0f; gain[4]=1.3f;
            type[0]=1; type[1]=3; type[2]=3; type[3]=3; type[4]=5;
            drive=2.0f; satMix=1.0f;
            if (variation % 3 == 0) width=0.30f;
            else if (variation % 3 == 1) { width=0.45f; panL=-0.30f; panR=0.20f; }
            else { width=0.55f; if (variation % 2 == 0) panL=-0.40f; else panR=0.40f; }
        }
        const float freqSpread[10] { 0.70f, 0.78f, 0.86f, 0.93f, 1.0f, 1.08f, 1.17f, 1.28f, 1.40f, 1.55f };
        const float eqSpread[10] { -2.5f, -2.0f, -1.4f, -0.8f, 0.0f, 0.8f, 1.4f, 2.0f, 2.6f, 3.2f };
        for (int band = 0; band < 5; ++band)
        {
            freq[band] *= std::pow(freqSpread[variation], band == 0 ? 0.5f : (band == 4 ? 0.7f : 1.0f));
            if (band > 0) gain[band] += eqSpread[variation] * (band == 2 ? -0.55f : 0.65f);
        }
        threshold += centered * 2.0f;
        attack *= (variation % 2 == 0 ? 0.8f : 1.25f);
        release *= (variation % 2 == 0 ? 0.85f : 1.20f);
        drive += centered * (category == 6 ? 1.0f : 0.5f);
    }

    for (int band = 0; band < 5; ++band)
    {
        set("eqBandFreq" + juce::String(band), freq[band]);
        set("eqBandGain" + juce::String(band), gain[band]);
        set("eqBandQ" + juce::String(band), q[band]);
        set("eqBandType" + juce::String(band), (float)type[band]);
        set("eqBandDynamic" + juce::String(band), 0.0f);
    }

    set("threshold", threshold); set("ratio", ratio);
    set("attack", attack); set("release", release);
    set("drive", drive); set("tone", tone); set("satMix", satMix);
    set("width", width); set("leftPan", panL); set("rightPan", panR);
    set("inputGain", input); set("ceiling", ceiling);
    set("limitRelease", limiterRelease); set("transient", transient);
    set("utilityGain", utility);
    set("drumDrive", drumDrive); set("drumDamp", drumDamp);
    set("drumBoom", drumBoom);
    set("pan", 0.0f); set("monoFreq", 120.0f); set("haasMs", 0.0f);

    const auto& names = beat ? beatPresets : vocalPresets;
    status.setText(names[safeId-1] + " LOADED — LISTEN & ADJUST TO SOURCE",
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
    rmsText.setText("OUTPUT  " + juce::String(processor.outputRmsDb(), 1) + " dBFS", juce::dontSendNotification);
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

    const juce::String titles[] { "EQ  /  PRECISION FREQUENCY SHAPING", "COMPRESSOR  /  DYNAMIC CONTROL", "SATURATOR  /  HARMONIC COLOR", "PAN & WIDTH  /  STEREO FIELD", "LIMITER  /  PEAK CONTROL", "UTILITY  /  OUTPUT GAIN", "DRUM BUS  /  DRIVE, DAMP & BOOM" };
    g.setColour(HFXColours::orange); g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(titles[activeTab], 32, 155, 760, 22, juce::Justification::left);

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
    const int tabWidth = (getWidth() - 40 - tabGap * 6) / 7;
    for (int i = 0; i < panelModeCount; ++i)
        tabs[(size_t)i].setBounds(20 + i * (tabWidth + tabGap), tabY, tabWidth, tabHeight);

    const int graphHeight = 320;
    spectrumView.setBounds(30, 185, getWidth() - 130, graphHeight);
    spectrumView.setVisible(activeTab != 5 && activeTab != 6);
    const int graphBottom = 185 + graphHeight;
    const int utilityY = (activeTab == 5 || activeTab == 6) ? getHeight() - 112 : graphBottom + 8;
    bypass.setBounds(30, utilityY, 82, 22);
    eqSoloButton.setBounds(120, utilityY, 142, 22);
    eqSoloButton.setVisible(activeTab == 0);
    sidechainButton.setBounds(getWidth() - 190, 151, 158, 26);
    sidechainButton.setVisible(activeTab == 1 && !drumBusEligible());
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
        const bool used = i < (activeTab == 5 ? 1 : 4);
        if (!used)
        {
            knobLabels[(size_t)i].setBounds(0, 0, 0, 0);
            knobs[(size_t)i].setBounds(0, 0, 0, 0);
            continue;
        }
        const int x = activeTab == 5 ? (getWidth() - 300) / 2 : startX + i * cell;
        const int y = activeTab == 5 ? 280 : (activeTab == 6 ? 350 : graphBottom + 48);
        knobLabels[(size_t)i].setBounds(x, y, activeTab == 5 ? 300 : cell - 8, activeTab == 5 ? 24 : 16);
        knobs[(size_t)i].setBounds(activeTab == 5 ? (getWidth() - 190) / 2 : x + (cell - 84) / 2,
                                   y + (activeTab == 5 ? 30 : 14),
                                   activeTab == 5 ? 190 : 84, activeTab == 5 ? 190 : 84);
    }
}
