#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

class HFXSpectrumView final : public juce::Component
{
public:
    explicit HFXSpectrumView(HFXAudioProcessor& p) : processor(p) {}
    void paint(juce::Graphics& g) override;
    void setDisplayMode(int mode) noexcept { displayMode = mode; repaint(); }
private:
    HFXAudioProcessor& processor;
    int displayMode = 0;
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
    std::array<juce::TextButton, 5> tabs;
    std::array<juce::Slider, 4> knobs;
    std::array<juce::Label, 4> knobLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stemRoleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    HFXSpectrumView spectrumView;
    int activeTab = 0;
    int lastPresetMode = -1;

    const std::array<std::array<const char*, 4>, 5> ids {{
        {{ "eqFreq", "eqGain", "eqQ", "eqFreq" }},
        {{ "threshold", "ratio", "attack", "release" }},
        {{ "drive", "tone", "warmth", "satMix" }},
        {{ "width", "pan", "monoFreq", "haasMs" }},
        {{ "inputGain", "ceiling", "limitRelease", "inputGain" }}
    }};
    const std::array<std::array<const char*, 4>, 5> names {{
        {{ "FREQ", "GAIN", "Q", "DYNAMIC" }},
        {{ "THRESHOLD", "RATIO", "ATTACK", "RELEASE" }},
        {{ "DRIVE", "TONE", "WARMTH", "MIX" }},
        {{ "WIDTH", "PAN", "MONO MAKER", "HAAS / DELAY" }},
        {{ "INPUT GAIN", "CEILING", "RELEASE", "TRANSIENT" }}
    }};

    void selectTab(int);
    void rebuildAttachments();
    void timerCallback() override;
    void applyPreset(int);
    void refreshPresetChoices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HFXAudioProcessorEditor)
};
