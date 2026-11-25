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
    std::array<juce::TextButton, 5> tabs;
    std::array<juce::Slider, 10> knobs;
    std::array<juce::Label, 10> knobLabels;
    std::array<juce::ComboBox, 8> eqFilterType;
    std::array<juce::Slider, 24> eqKnobs;
    std::array<juce::Label, 24> eqKnobLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 24> eqAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 8> eqFilterAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 10> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> stemRoleAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    HFXSpectrumView spectrumView;
    int activeTab = 0;
    int lastPresetMode = -1;
    int autoMixWaitTicks = 0;

    const std::array<std::array<const char*, 10>, 5> ids {{
        {{ "eqBandFreq0", "eqBandFreq1", "eqBandFreq2", "eqBandFreq3", "eqBandFreq4", "eqBandGain0", "eqBandGain1", "eqBandGain2", "eqBandGain3", "eqBandGain4" }},
        {{ "threshold", "ratio", "attack", "release", "threshold", "threshold", "ratio", "attack", "release", "threshold" }},
        {{ "drive", "tone", "warmth", "satMix", "drive", "drive", "tone", "warmth", "satMix", "drive" }},
        {{ "width", "leftPan", "rightPan", "width", "width", "width", "leftPan", "rightPan", "width", "width" }},
        {{ "inputGain", "ceiling", "limitRelease", "inputGain", "ceiling", "inputGain", "ceiling", "limitRelease", "inputGain", "ceiling" }}
    }};
    const std::array<std::array<const char*, 10>, 5> names {{
        {{ "LOW FREQ", "LOW-MID FREQ", "MID FREQ", "HIGH-MID FREQ", "HIGH FREQ", "LOW GAIN", "LOW-MID GAIN", "MID GAIN", "HIGH-MID GAIN", "HIGH GAIN" }},
        {{ "THRESHOLD", "RATIO", "ATTACK", "RELEASE", "KNEE", "THRESHOLD", "RATIO", "ATTACK", "RELEASE", "KNEE" }},
        {{ "DRIVE", "TONE", "WARMTH", "MIX", "OUTPUT", "DRIVE", "TONE", "WARMTH", "MIX", "OUTPUT" }},
        {{ "WIDTH", "LEFT PAN", "RIGHT PAN", "—", "—", "WIDTH", "LEFT PAN", "RIGHT PAN", "—", "—" }},
        {{ "INPUT GAIN", "CEILING", "RELEASE", "TRANSIENT", "OUTPUT", "INPUT GAIN", "CEILING", "RELEASE", "TRANSIENT", "OUTPUT" }}
    }};

    void selectTab(int);
    void rebuildAttachments();
    void timerCallback() override;
    void applyPreset(int);
    void refreshPresetChoices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HFXAudioProcessorEditor)
};
