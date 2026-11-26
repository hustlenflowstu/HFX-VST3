#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

class HFXSpectrumView final : public juce::Component
{
public:
    explicit HFXSpectrumView(HFXAudioProcessor& p) : processor(p) {}
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void setDisplayMode(int mode) noexcept { displayMode = mode; repaint(); }
    std::function<void(int)> onBandSelected;
private:
    HFXAudioProcessor& processor;
    int displayMode = 0;
    int draggedBand = -1;
    void updateEqFromPoint(juce::Point<float> point);
};

class HFXAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HFXAudioProcessorEditor(HFXAudioProcessor&);
    ~HFXAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HFXAudioProcessor& processor;
    HFXLookAndFeel laf;
    juce::Label logo, moduleTitle, status, leftMeterText, rightMeterText, rmsText;
    juce::ComboBox trackMode, stemRole, presetBox;
    juce::TextButton autoMix { "AUTO-MIX" }, bypass { "BYPASS" };
    juce::TextButton eqSoloButton { "SOLO SELECTED BAND" };
    std::array<juce::TextButton, 5> tabs;
    std::array<juce::Slider, 10> knobs;
    std::array<juce::Label, 10> knobLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 10> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stemRoleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    HFXSpectrumView spectrumView;
    int activeTab = 0;
    int selectedEqBand = 2;
    int lastPresetMode = -1;
    int autoMixWaitTicks = 0;

    const std::array<std::array<const char*, 4>, 5> ids {{
        {{ "eqBandFreq0", "eqBandGain0", "eqBandQ0", "eqBandDynamic0" }},
        {{ "threshold", "ratio", "attack", "release" }},
        {{ "drive", "tone", "satMix", "satMix" }},
        {{ "width", "leftPan", "rightPan", "rightPan" }},
        {{ "inputGain", "ceiling", "limitRelease", "transient" }}
    }};
    const std::array<std::array<const char*, 4>, 5> names {{
        {{ "FREQ", "GAIN", "Q", "DYNAMIC" }},
        {{ "THRESHOLD", "RATIO", "ATTACK", "RELEASE" }},
        {{ "DRIVE", "TONE", "MIX", "" }},
        {{ "WIDTH", "PAN L", "PAN R", "" }},
        {{ "INPUT GAIN", "CEILING", "RELEASE", "TRANSIENT" }}
    }};

    void selectTab(int);
    void rebuildAttachments();
    void timerCallback() override;
    void applyPreset(int);
    void refreshPresetChoices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HFXAudioProcessorEditor)
};
