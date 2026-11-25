#include "PluginEditor.h"

namespace
{
    constexpr int panelModeCount = 5;
    const juce::StringArray vocalPresets = []
    {
        juce::StringArray a;
        const char* groups[5][10] = {
            {"Rap - Clean Punch", "Rap - Forward Lead", "Rap - Aggressive Bite", "Rap - Dry Close Mic", "Rap - Dark Chest", "Rap - Bright Cut", "Rap - Thick Doubled", "Rap - Tight Modern", "Rap - Grit Texture", "Rap - Radio Lead"},
            {"Melody - Smooth Tune", "Melody - Hard Tune", "Melody - Airy Tune", "Melody - Emotional Lead", "Melody - Wide Hook", "Melody - Bright Top", "Melody - Soft Tune", "Melody - Dark Tune", "Melody - Layer Stack", "Melody - Modern Pop Rap"},
            {"R&B - Silky Lead", "R&B - Warm Intimate", "R&B - Breath Control", "R&B - Velvet Smooth", "R&B - Wide Harmony", "R&B - Soft Air", "R&B - Rich Chest", "R&B - Late Night", "R&B - Clean Studio", "R&B - Gloss Lead"},
            {"Pop - Radio Bright", "Pop - Clean Center", "Pop - Airy Lead", "Pop - Punchy Lead", "Pop - Soft Focus", "Pop - Wide Chorus", "Pop - Intimate Verse", "Pop - Gloss Vocal", "Pop - Modern Stack", "Pop - Anthem Lead"},
            {"FX - Telephone", "FX - Megaphone", "FX - Lo-Fi Vocal", "FX - Radio Distort", "FX - Whisper Air", "FX - Formant Color", "FX - Wide Doubler", "FX - Throw Delay", "FX - Dark Reverb", "FX - Creative Crush"}
        };
        for (const auto& group : groups) for (auto* name : group) a.add(name);
        return a;
    }();
    const juce::StringArray beatPresets = []
    {
        juce::StringArray a;
        const char* groups[5][10] = {
            {"Kick - Deep Trap", "Kick - Punch Attack", "Kick - Tight Short", "Kick - Warm Round", "Kick - Hard Clip", "Kick - Sub Focus", "Kick - Modern Knock", "Kick - Dry Studio", "Kick - Heavy Impact", "Kick - Soft Vintage"},
            {"Snare - Crack", "Snare - Tight Pop", "Snare - Wide Layer", "Snare - Dark Punch", "Snare - Bright Snap", "Snare - Room Body", "Snare - Trap Rim", "Snare - Dry Knock", "Snare - Big Plate", "Snare - Lo-Fi"},
            {"Perc - Clean Groove", "Perc - Bright Tick", "Perc - Warm Hand", "Perc - Wide Perc", "Perc - Tight Transient", "Perc - Organic", "Perc - Afro Groove", "Perc - Dark Texture", "Perc - Punch Layer", "Perc - Soft Space"},
            {"Hi-Hat - Crisp Trap", "Hi-Hat - Smooth Top", "Hi-Hat - Tight Dry", "Hi-Hat - Bright Roll", "Hi-Hat - Dark Control", "Hi-Hat - Open Air", "Hi-Hat - Wide Pattern", "Hi-Hat - Soft Tick", "Hi-Hat - Modern Drill", "Hi-Hat - Lo-Fi Dust"},
            {"808 - Clean Sub", "808 - Hard Clip", "808 - Long Glide", "808 - Tight Mono", "808 - Deep Round", "808 - Grit Distort", "808 - Punch Bass", "808 - Dark Heavy", "808 - Smooth Sustain", "808 - Drill Growl"}
        };
        for (const auto& group : groups) for (auto* name : group) a.add(name);
        return a;
    }();
}

void HFXSpectrumView::mouseDown(const juce::MouseEvent&) {}
void HFXSpectrumView::mouseDrag(const juce::MouseEvent&) {}
void HFXSpectrumView::updateEqFromPoint(juce::Point<float>) {}

void HFXSpectrumView::paint(juce::Graphics& g)
{
    // Deliberately simple, static EQ response display for the stability build.
    // No FFT/spectrum reads, animated paths, or custom graph effects.
    auto r = getLocalBounds().toFloat();
    g.setColour(HFXColours::panel2);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(HFXColours::grid);
    for (int i = 1; i < 5; ++i)
        g.drawHorizontalLine((int)(r.getY() + r.getHeight() * i / 5.0f), r.getX(), r.getRight());
    for (int i = 1; i < 9; ++i)
        g.drawVerticalLine((int)(r.getX() + r.getWidth() * i / 9.0f), r.getY(), r.getBottom());

    if (displayMode != 0)
    {
        const int mode = juce::jlimit(1, 4, displayMode);
        const auto plot = r.reduced(22.0f, 24.0f);
        g.setColour(HFXColours::grid);
        g.drawRoundedRectangle(r.reduced(1.0f), 6.0f, 1.0f);
        g.setColour(HFXColours::muted);
        g.setFont(11.0f);
        const juce::String titles[] { "", "COMPRESSOR", "SATURATOR", "STEREO FIELD", "LIMITER" };
        g.drawText(titles[mode], (int)r.getX() + 12, (int)r.getY() + 7, 220, 18, juce::Justification::left);

        if (mode == 1) // Compressor transfer curve + incoming waveform
        {
            const float threshold = processor.compressorThresholdDb();
            const float ratio = juce::jmax(1.0f, processor.compressorRatio());
            auto mapDb = [&plot](float db, bool yAxis)
            {
                const float norm = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);
                return yAxis ? plot.getBottom() - norm * plot.getHeight()
                             : plot.getX() + norm * plot.getWidth();
            };
            juce::Path transfer;
            for (int i = 0; i <= 120; ++i)
            {
                const float inDb = -60.0f + (float)i;
                const float outDb = inDb <= threshold ? inDb : threshold + (inDb - threshold) / ratio;
                const float x = mapDb(inDb, false), y = mapDb(outDb, true);
                if (i == 0) transfer.startNewSubPath(x, y); else transfer.lineTo(x, y);
            }
            g.setColour(HFXColours::muted.withAlpha(0.7f));
            g.drawLine(plot.getX(), plot.getBottom(), plot.getRight(), plot.getY(), 1.0f);
            g.setColour(HFXColours::orange);
            g.strokePath(transfer, juce::PathStrokeType(2.4f));
            const float tx = mapDb(threshold, false);
            g.setColour(HFXColours::orange.withAlpha(0.55f));
            g.drawVerticalLine((int)tx, plot.getY(), plot.getBottom());
            g.setColour(HFXColours::muted);
            g.drawText("INPUT LEVEL", (int)plot.getX(), (int)plot.getBottom() + 5, (int)plot.getWidth(), 16, juce::Justification::centred);
        }
        else if (mode == 2) // Saturator waveshaping transfer curve
        {
            const float driveDb = processor.saturationDriveDb();
            const float drive = juce::Decibels::decibelsToGain(driveDb);
            juce::Path curve;
            for (int i = 0; i <= 160; ++i)
            {
                const float input = -1.0f + 2.0f * (float)i / 160.0f;
                const float output = std::tanh(input * drive);
                const float x = plot.getX() + (input + 1.0f) * 0.5f * plot.getWidth();
                const float y = plot.getCentreY() - output * plot.getHeight() * 0.46f;
                if (i == 0) curve.startNewSubPath(x, y); else curve.lineTo(x, y);
            }
            g.setColour(HFXColours::muted.withAlpha(0.7f));
            g.drawLine(plot.getX(), plot.getCentreY(), plot.getRight(), plot.getCentreY(), 1.0f);
            g.drawLine(plot.getCentreX(), plot.getY(), plot.getCentreX(), plot.getBottom(), 1.0f);
            g.setColour(HFXColours::orange);
            g.strokePath(curve, juce::PathStrokeType(2.5f));
            g.setColour(HFXColours::muted);
            g.drawText("INPUT", (int)plot.getX(), (int)plot.getBottom() + 5, (int)plot.getWidth(), 16, juce::Justification::centred);
        }
        else if (mode == 3) // Ozone-Imager-inspired stereo vectorscope
        {
            const auto scope = plot.withSizeKeepingCentre(juce::jmin(plot.getWidth(), plot.getHeight()),
                                                           juce::jmin(plot.getWidth(), plot.getHeight()));
            g.setColour(HFXColours::grid);
            g.drawEllipse(scope);
            g.drawLine(scope.getCentreX(), scope.getY(), scope.getCentreX(), scope.getBottom(), 1.0f);
            g.drawLine(scope.getX(), scope.getCentreY(), scope.getRight(), scope.getCentreY(), 1.0f);
            g.drawLine(scope.getX() + scope.getWidth() * 0.15f, scope.getBottom() - scope.getHeight() * 0.15f,
                       scope.getRight() - scope.getWidth() * 0.15f, scope.getY() + scope.getHeight() * 0.15f, 1.0f);
            juce::Path cloud;
            constexpr int points = 160;
            for (int i = 0; i < points; ++i)
            {
                const float sx = juce::jlimit(-1.0f, 1.0f, processor.scopeXAt(i));
                const float sy = juce::jlimit(-1.0f, 1.0f, processor.scopeYAt(i));
                const float x = scope.getCentreX() + sx * scope.getWidth() * 0.46f;
                const float y = scope.getCentreY() - sy * scope.getHeight() * 0.46f;
                if (i == 0) cloud.startNewSubPath(x, y); else cloud.lineTo(x, y);
            }
            g.setColour(HFXColours::orange.withAlpha(0.82f));
            g.strokePath(cloud, juce::PathStrokeType(1.4f));
            g.setColour(HFXColours::muted);
            g.drawText("LEFT / RIGHT STEREO IMAGE", (int)r.getX() + 12, (int)r.getBottom() - 20, (int)r.getWidth() - 24, 14, juce::Justification::centred);
        }
        else if (mode == 4) // Limiter ceiling + output waveform
        {
            const float ceiling = processor.limiterCeilingDb();
            const float ceilingNorm = juce::jlimit(0.0f, 1.0f, juce::Decibels::decibelsToGain(ceiling));
            const float ceilingY = plot.getCentreY() - ceilingNorm * plot.getHeight() * 0.46f;
            g.setColour(HFXColours::orange.withAlpha(0.7f));
            g.drawHorizontalLine((int)ceilingY, plot.getX(), plot.getRight());
            g.drawHorizontalLine((int)(plot.getCentreY() * 2.0f - ceilingY), plot.getX(), plot.getRight());
            juce::Path wave;
            constexpr int points = 256;
            for (int i = 0; i < points; ++i)
            {
                const float sample = juce::jlimit(-1.0f, 1.0f, processor.waveformAt(i));
                const float x = plot.getX() + (float)i / (float)(points - 1) * plot.getWidth();
                const float y = plot.getCentreY() - sample * plot.getHeight() * 0.46f;
                if (i == 0) wave.startNewSubPath(x, y); else wave.lineTo(x, y);
            }
            g.setColour(HFXColours::muted);
            g.drawLine(plot.getX(), plot.getCentreY(), plot.getRight(), plot.getCentreY(), 1.0f);
            g.setColour(HFXColours::orange);
            g.strokePath(wave, juce::PathStrokeType(1.5f));
            g.setColour(HFXColours::muted);
            g.drawText("OUTPUT PEAK / CEILING", (int)plot.getX(), (int)plot.getBottom() + 5, (int)plot.getWidth(), 16, juce::Justification::centred);
        }
        return;
    }

    // Live input spectrum: read-only atomic FFT data from the audio processor.
    // The editor samples it at a modest UI rate; no FFT work happens in paint().
    juce::Path liveSpectrum;
    constexpr int spectrumPoints = 128;
    constexpr float minHz = 20.0f, maxHz = 20000.0f;
    for (int i = 0; i < spectrumPoints; ++i)
    {
        const float t = (float)i / (float)(spectrumPoints - 1);
        const float hz = minHz * std::pow(maxHz / minHz, t);
        const float raw = juce::jlimit(0.0f, 1.0f, processor.spectrumAtFrequency(hz));
        const float target = std::sqrt(raw);
        spectrumSmooth[(size_t)i] = 0.72f * spectrumSmooth[(size_t)i] + 0.28f * target;
        const float x = r.getX() + t * r.getWidth();
        const float y = r.getBottom() - 3.0f - spectrumSmooth[(size_t)i] * r.getHeight() * 0.82f;
        if (i == 0) liveSpectrum.startNewSubPath(x, y); else liveSpectrum.lineTo(x, y);
    }
    juce::Path spectrumFill = liveSpectrum;
    spectrumFill.lineTo(r.getRight(), r.getBottom());
    spectrumFill.lineTo(r.getX(), r.getBottom());
    spectrumFill.closeSubPath();
    g.setColour(HFXColours::orange.withAlpha(0.12f));
    g.fillPath(spectrumFill);
    g.setColour(HFXColours::orange.withAlpha(0.48f));
    g.strokePath(liveSpectrum, juce::PathStrokeType(1.35f));

    juce::Path response;
    for (int i = 0; i < 200; ++i)
    {
        const float t = (float)i / 199.0f;
        const float hz = minHz * std::pow(maxHz / minHz, t);
        float db = 0.0f;
        for (int band = 0; band < 8; ++band)
        {
            const float centre = juce::jmax(20.0f, processor.eqBandFrequency(band));
            const float octaves = std::log2(hz / centre);
            db += processor.eqBandGain(band) * std::exp(-0.5f * octaves * octaves / 0.72f);
        }
        const float x = r.getX() + t * r.getWidth();
        const float y = r.getCentreY() - juce::jlimit(-18.0f, 18.0f, db) * (r.getHeight() / 40.0f);
        if (i == 0) response.startNewSubPath(x, y); else response.lineTo(x, y);
    }
    g.setColour(HFXColours::orange);
    g.strokePath(response, juce::PathStrokeType(2.0f));

    for (int band = 0; band < 8; ++band)
    {
        const float hz = juce::jlimit(minHz, maxHz, processor.eqBandFrequency(band));
        const float t = std::log(hz / minHz) / std::log(maxHz / minHz);
        const float x = r.getX() + t * r.getWidth();
        const float y = r.getCentreY() - processor.eqBandGain(band) * (r.getHeight() / 40.0f);
        g.setColour(HFXColours::orange);
        g.fillEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f);
    }

    if (displayMode == 0)
    {
        const auto plot = r.reduced(42.0f, 28.0f);
        for (int band = 0; band < 8; ++band)
        {
            const float freq = juce::jlimit(20.0f, 20000.0f, processor.eqBandFrequency(band));
            const float gain = juce::jlimit(-18.0f, 18.0f, processor.eqBandGain(band));
            const float xNorm = std::log(freq / 20.0f) / std::log(1000.0f);
            const float yNorm = 0.5f + gain / 36.0f;
            const float x = plot.getX() + xNorm * plot.getWidth();
            const float y = plot.getBottom() - yNorm * plot.getHeight();
            g.setColour(band == selectedBand ? HFXColours::orange : juce::Colours::white);
            const float d = band == selectedBand ? 12.0f : 8.0f;
            g.fillEllipse(x - d * 0.5f, y - d * 0.5f, d, d);
        }
        g.setColour(HFXColours::muted);
        g.setFont(11.0f);
        g.drawText("BAND " + juce::String(selectedBand + 1) + " SELECTED",
                   (int)r.getX() + 12, (int)r.getY() + 8, 180, 16,
                   juce::Justification::left);
    }
}

HFXAudioProcessorEditor::HFXAudioProcessorEditor(HFXAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), spectrumView(p)
{
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
    for (int i = 0; i < 10; ++i)
    {
        auto& slider = knobs[(size_t)i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
        addAndMakeVisible(slider);
        knobLabels[(size_t)i].setJustificationType(juce::Justification::centred);
        knobLabels[(size_t)i].setColour(juce::Label::textColourId, HFXColours::muted);
        knobLabels[(size_t)i].setFont(juce::Font(11.0f, juce::Font::bold));
        addAndMakeVisible(knobLabels[(size_t)i]);
    }
    spectrumView.onEqBandSelected = [this](int) { attachSelectedEqBand(); };
    status.setColour(juce::Label::textColourId, HFXColours::muted); addAndMakeVisible(status);
    leftMeterText.setColour(juce::Label::textColourId, HFXColours::orange);
    rightMeterText.setColour(juce::Label::textColourId, HFXColours::orange);
    rmsText.setColour(juce::Label::textColourId, HFXColours::muted);
    addAndMakeVisible(leftMeterText); addAndMakeVisible(rightMeterText); addAndMakeVisible(rmsText);
    rebuildAttachments();
    startTimerHz(30);
}

HFXAudioProcessorEditor::~HFXAudioProcessorEditor()
{
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
    for (int i = 0; i < 10; ++i)
    {
        const bool used = (activeTab == 0) ? (i < 4)
            : ((activeTab == 3 || activeTab == 4) ? i < 3 : i < 4);
        knobLabels[(size_t)i].setVisible(used);
        knobs[(size_t)i].setVisible(used);
        if (used && activeTab != 0)
        {
            knobLabels[(size_t)i].setText(names[(size_t)activeTab][(size_t)i], juce::dontSendNotification);
            attachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.state(), ids[(size_t)activeTab][(size_t)i], knobs[(size_t)i]);
        }
    }
    if (activeTab == 0)
        attachSelectedEqBand();
}

void HFXAudioProcessorEditor::attachSelectedEqBand()
{
    for (auto& attachment : attachments) attachment.reset();
    if (activeTab != 0) return;
    const int band = juce::jlimit(0, 7, spectrumView.getSelectedBand());
    const juce::String suffix = juce::String(band);
    const juce::String params[4] {
        "eqBandFreq" + suffix, "eqBandGain" + suffix,
        "eqBandQ" + suffix, "eqBandDynamic" + suffix
    };
    const juce::String labels[4] { "FREQ", "GAIN", "Q", "DYNAMIC" };
    for (int i = 0; i < 4; ++i)
    {
        knobLabels[(size_t)i].setText(labels[i], juce::dontSendNotification);
        knobLabels[(size_t)i].setVisible(true);
        knobs[(size_t)i].setVisible(true);
        attachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state(), params[i], knobs[(size_t)i]);
    }
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
    // Keep the EQ page close to the supplied HFX reference: large spectrum/response
    // graph, orange branding, and a compact control strip beneath it.
    const int graphHeight = activeTab == 0 ? 330 : 350;
    spectrumView.setBounds(30, 185, getWidth() - 130, graphHeight);
    const int graphBottom = 185 + graphHeight;
    const int utilityY = graphBottom + 8;
    bypass.setBounds(30, utilityY, 90, 24);
    status.setBounds(135, utilityY, getWidth() - 245, 24);
    leftMeterText.setBounds(getWidth() - 110, 555, 100, 18);
    rightMeterText.setBounds(getWidth() - 110, 575, 100, 18);
    rmsText.setBounds(getWidth() - 110, 595, 100, 18);
    if (activeTab == 0)
    {
        const int controlWidth = (getWidth() - 220) / 4;
        const int startX = 120;
        for (int i = 0; i < 4; ++i)
        {
            const int x = startX + i * controlWidth;
            const int y = graphBottom + 22;
            knobLabels[(size_t)i].setBounds(x, y, controlWidth - 12, 18);
            knobs[(size_t)i].setBounds(x + (controlWidth - 88) / 2, y + 18, 88, 88);
        }
    }
    const int available = getWidth() - 210;
    const int count = (activeTab == 3 || activeTab == 4) ? 3 : 4;
    const int cell = available / count;
    const int startX = 105;
    for (int i = 0; i < 10; ++i)
    {
        const bool used = activeTab != 0 && ((activeTab == 3 || activeTab == 4) ? i < 3 : i < 4);
        if (!used) { knobLabels[(size_t)i].setBounds(0, 0, 0, 0); knobs[(size_t)i].setBounds(0, 0, 0, 0); continue; }
        const int col = i;
        const int x = startX + col * cell;
        const int y = graphBottom + 54;
        knobLabels[(size_t)i].setBounds(x, y, cell - 8, 16);
        knobs[(size_t)i].setBounds(x + (cell - 76) / 2, y + 14, 76, 76);
    }
}
