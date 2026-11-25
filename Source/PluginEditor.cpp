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
        g.setColour(HFXColours::grid);
        g.drawRoundedRectangle(r.reduced(1.0f), 6.0f, 1.0f);
        const juce::String titles[] {
            "EQ", "COMPRESSOR", "SATURATOR", "PAN & WIDTH", "LIMITER"
        };
        const juce::String descriptions[] {
            "", "THRESHOLD  •  RATIO  •  ATTACK  •  RELEASE",
            "DRIVE  •  TONE  •  WARMTH  •  MIX",
            "STEREO WIDTH  •  LEFT PAN  •  RIGHT PAN",
            "INPUT GAIN  •  CEILING  •  RELEASE"
        };
        g.setColour(HFXColours::orange);
        g.setFont(juce::Font(20.0f, juce::Font::bold));
        g.drawText(titles[juce::jlimit(0, 4, displayMode)],
                   r.removeFromTop(r.getHeight() * 0.48f).toNearestInt(),
                   juce::Justification::centred);
        g.setColour(HFXColours::muted);
        g.setFont(12.0f);
        g.drawText(descriptions[juce::jlimit(0, 4, displayMode)],
                   r.toNearestInt(), juce::Justification::centred);
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
    const juce::StringArray filterNames {
        "48 dB/oct Low Cut", "12 dB/oct Low Cut", "Low Shelf", "Bell",
        "Notch", "High Shelf", "12 dB/oct High Cut", "48 dB/oct High Cut"
    };
    for (int band = 0; band < 8; ++band)
    {
        auto& selector = eqFilterType[(size_t)band];
        selector.addItemList(filterNames, 1);
        addAndMakeVisible(selector);
        eqFilterAttachments[(size_t)band] =
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                processor.state(), "eqBandType" + juce::String(band), selector);
    }
    const juce::String eqControlNames[3] { "FREQ", "GAIN", "Q" };
    for (int band = 0; band < 8; ++band)
    {
        for (int control = 0; control < 3; ++control)
        {
            const int index = band * 3 + control;
            auto& slider = eqKnobs[(size_t)index];
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 17);
                addAndMakeVisible(slider);
            eqKnobLabels[(size_t)index].setText(eqControlNames[control], juce::dontSendNotification);
            eqKnobLabels[(size_t)index].setJustificationType(juce::Justification::centred);
            eqKnobLabels[(size_t)index].setColour(juce::Label::textColourId, HFXColours::muted);
            eqKnobLabels[(size_t)index].setFont(juce::Font(9.5f, juce::Font::bold));
            addAndMakeVisible(eqKnobLabels[(size_t)index]);
        }
    }
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
    for (auto& attachment : eqAttachments) attachment.reset();
    for (int i = 0; i < 24; ++i)
    {
        const int band = i / 3, control = i % 3;
        const juce::String suffix = juce::String(band);
        const juce::String parameter = control == 0 ? "eqBandFreq" + suffix : (control == 1 ? "eqBandGain" + suffix : "eqBandQ" + suffix);
        eqKnobs[(size_t)i].setVisible(activeTab == 0);
        eqKnobLabels[(size_t)i].setVisible(activeTab == 0);
        if (activeTab == 0)
            eqAttachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state(), parameter, eqKnobs[(size_t)i]);
    }
    for (int i = 0; i < 10; ++i)
    {
        const bool used = (activeTab != 0) && ((activeTab == 3 || activeTab == 4) ? i < 3 : i < 4);
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
    auto set = [this](const juce::String& key, float value)
    {
        if (auto* parameter = processor.state().getParameter(key))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };
    const bool beat = trackMode.getSelectedId() == 2;
    // All 50 presets, including the original first eight, now populate the live five-band EQ.
    {
        const int categoryForEq = juce::jlimit(0, 4, (id - 1) / 10);
        const int variationForEq = (id - 1) % 10;
        const float variationAmount = (float)variationForEq / 9.0f;
        const float root = beat ? (categoryForEq == 4 ? 48.0f : 90.0f + categoryForEq * 80.0f)
                                : 100.0f + categoryForEq * 110.0f;
        for (int band = 0; band < 5; ++band)
        {
            const float freq = juce::jlimit(20.0f, 20000.0f, root * std::pow(2.0f, (float)band * (beat ? 1.55f : 1.7f)));
            const float curve = (band == 0 ? -0.3f : band == 4 ? 0.25f : 0.1f) * (float)(categoryForEq + 1);
            set("eqBandFreq" + juce::String(band), freq);
            set("eqBandGain" + juce::String(band), juce::jlimit(-6.0f, 6.0f, (variationAmount - 0.5f) * 3.0f + curve));
        }
    }
    if (id > 8)
    {
        // The additional entries are complete parameter snapshots, not labels-only presets.
        const int category = (id - 1) / 10;
        const int variation = (id - 1) % 10;
        const float v = (float)variation / 9.0f;
        const float baseFreq = beat ? (category == 4 ? 55.0f : 110.0f + category * 95.0f) : 120.0f + category * 90.0f;
        const float baseGain = (v - 0.5f) * (beat ? 4.0f : 3.0f);
        for (int band = 0; band < 5; ++band)
        {
            const float freq = juce::jlimit(20.0f, 20000.0f, baseFreq * std::pow(2.0f, (float)band * (beat ? 1.45f : 1.65f)));
            const float shape = (band == 0 ? -0.25f : band == 4 ? 0.30f : 0.12f) * (float)(category + 1);
            set("eqBandFreq" + juce::String(band), freq);
            set("eqBandGain" + juce::String(band), juce::jlimit(-6.0f, 6.0f, baseGain + shape));
        }
        set("threshold", beat ? -16.0f - v * 8.0f : -19.0f - v * 7.0f);
        set("ratio", beat ? 2.0f + category * 0.45f + v : 2.0f + category * 0.35f + v * 1.4f);
        set("attack", beat ? 3.0f + v * 32.0f : 5.0f + v * 24.0f);
        set("release", beat ? 45.0f + v * 130.0f : 65.0f + v * 115.0f);
        set("drive", beat ? 1.0f + category * 0.55f + v * 2.0f : 0.5f + category * 0.45f + v * 1.5f);
        set("tone", (v - 0.5f) * 0.5f); set("warmth", 0.1f + v * 0.35f);
        set("satMix", 0.12f + v * 0.28f); set("width", beat ? 0.92f + v * 0.25f : 1.0f + v * 0.20f);
        set("ceiling", -1.0f - v * 1.5f); set("inputGain", (0.5f - v) * 2.0f);
        status.setText((beat ? "BEAT" : "VOCAL") + juce::String(" PRESET LOADED — CATEGORY ") + juce::String(category + 1) + " / VARIATION " + juce::String(variation + 1), juce::dontSendNotification);
        return;
    }
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
    // Repaint only the graph; FFT calculation remains on the audio thread.
    if (activeTab == 0)
        spectrumView.repaint();
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
    const int graphHeight = activeTab == 0 ? 320 : 350;
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
        const int groupWidth = (getWidth() - 70) / 4;
        for (int band = 0; band < 8; ++band)
        {
            const int row = band / 4, col = band % 4;
            const int x = 35 + col * groupWidth;
            const int y = graphBottom + 8 + row * 145;
            eqFilterType[(size_t)band].setBounds(x + 4, y, groupWidth - 12, 24);
            for (int control = 0; control < 3; ++control)
            {
                const int index = band * 3 + control;
                const int knobX = x + 5 + control * ((groupWidth - 15) / 3);
                eqKnobLabels[(size_t)index].setBounds(knobX, y + 27, (groupWidth - 15) / 3, 13);
                eqKnobs[(size_t)index].setBounds(knobX, y + 39, (groupWidth - 15) / 3, 90);
            }
        }
    }
    else for (auto& selector : eqFilterType) selector.setBounds(0, 0, 0, 0);
    const int available = getWidth() - 210;
    const int count = activeTab == 0 ? 5 : ((activeTab == 3 || activeTab == 4) ? 3 : 4);
    const int cell = available / count;
    const int startX = 105;
    for (int i = 0; i < 10; ++i)
    {
        const bool used = (activeTab != 0) && ((activeTab == 3 || activeTab == 4) ? i < 3 : i < 4);
        if (!used) { knobLabels[(size_t)i].setBounds(0, 0, 0, 0); knobs[(size_t)i].setBounds(0, 0, 0, 0); continue; }
        const int row = activeTab == 0 ? i / 5 : 0;
        const int col = activeTab == 0 ? i % 5 : i;
        const int x = startX + col * cell;
        const int y = activeTab == 0 ? graphBottom + 72 + row * 98 : graphBottom + 54;
        knobLabels[(size_t)i].setBounds(x, y, cell - 8, 16);
        knobs[(size_t)i].setBounds(x + (cell - 76) / 2, y + 14, 76, 76);
    }
}
