#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class HFXAudioProcessor final : public juce::AudioProcessor
{
public:
    HFXAudioProcessor();
    ~HFXAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState& state() noexcept { return apvts; }
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();

    void requestAutoMix() noexcept;
    bool autoMixReady() const noexcept { return resultReady.load(std::memory_order_acquire); }
    struct MixResult { float eqGain, threshold, drive, width, inputGain, rmsDb, crestDb; float eqLow, eqLowMid, eqMid, eqHighMid, eqHigh; };
    MixResult takeAutoMixResult() noexcept;

    float spectrumAt(int index) const noexcept;
    float spectrumAtFrequency(float hz) const noexcept;
    void setEqBandFromGraph(int band, float frequencyHz, float gainDb);
    float eqBandFrequency(int band) const noexcept;
    float eqBandGain(int band) const noexcept;
    float waveformAt(int index) const noexcept;
    float scopeXAt(int index) const noexcept;
    float scopeYAt(int index) const noexcept;
    float compressorThresholdDb() const noexcept { return pThreshold->load(); }
    float compressorRatio() const noexcept { return pRatio->load(); }
    float saturationDriveDb() const noexcept { return pDrive->load(); }
    float limiterCeilingDb() const noexcept { return pCeiling->load(); }
    float leftPeakDb() const noexcept { return leftPeak.load(); }
    float rightPeakDb() const noexcept { return rightPeak.load(); }
    float outputRmsDb() const noexcept { return outputRms.load(); }

private:
    juce::AudioProcessorValueTreeState apvts;

    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
        void setPeak(double sr, float freq, float q, float gainDb) noexcept;
        void setFilter(double sr, float freq, float q, float gainDb, int type) noexcept;
        float process(float x) noexcept;
        void reset() noexcept { z1 = z2 = 0.0f; }
    };
    std::array<std::array<std::array<Biquad, 4>, 5>, 2> eq;
    std::array<std::atomic<float>*, 5> pEqBandFreq {};
    std::array<std::atomic<float>*, 5> pEqBandGain {};
    std::array<std::atomic<float>*, 5> pEqBandType {};
    std::array<float, 2> compEnvelope { 0.0f, 0.0f };
    std::array<float, 2> limiterGain { 1.0f, 1.0f };
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0;
    std::array<float, 2> lowState { 0.0f, 0.0f };
    double sampleRateHz = 44100.0;

    std::atomic<float>* pEqFreq = nullptr;
    std::atomic<float>* pEqGain = nullptr;
    std::atomic<float>* pEqQ = nullptr;
    std::atomic<float>* pThreshold = nullptr;
    std::atomic<float>* pRatio = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pWarmth = nullptr;
    std::atomic<float>* pSatMix = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pPan = nullptr; // legacy global pan
    std::atomic<float>* pLeftPan = nullptr;
    std::atomic<float>* pRightPan = nullptr;
    std::atomic<float>* pMonoFreq = nullptr;
    std::atomic<float>* pHaasMs = nullptr;
    std::atomic<float>* pInputGain = nullptr;
    std::atomic<float>* pCeiling = nullptr;
    std::atomic<float>* pLimitRelease = nullptr;
    std::atomic<float>* pTrackMode = nullptr;
    std::atomic<float>* pStemRole = nullptr;
    std::atomic<float>* pBypass = nullptr;

    std::atomic<float> leftPeak { -90.0f }, rightPeak { -90.0f }, outputRms { -90.0f };

    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, fftSize * 2> fftData {};
    int fftWrite = 0;
    std::array<std::atomic<float>, fftSize / 2> spectrum {};
    std::atomic<int> spectrumVersion { 0 };
    static constexpr int visualSize = 512;
    std::array<std::atomic<float>, visualSize> waveformHistory {};
    std::array<std::atomic<float>, visualSize> scopeXHistory {};
    std::array<std::atomic<float>, visualSize> scopeYHistory {};
    std::atomic<int> visualWrite { 0 };

    std::atomic<bool> autoMixRequested { false };
    std::atomic<bool> autoMixResetPending { false };
    std::atomic<bool> resultReady { false };
    double analysisCount = 0.0;
    double analysisSquares = 0.0;
    double analysisLowSquares = 0.0, analysisMidSquares = 0.0, analysisHighSquares = 0.0;
    std::array<float, 2> analysisLowState { 0.0f, 0.0f }, analysisHighState { 0.0f, 0.0f };
    float analysisPeak = 0.0f;
    std::atomic<float> resultEq { 0.0f }, resultThreshold { -18.0f };
    std::atomic<float> resultDrive { 2.0f }, resultWidth { 1.0f };
    std::atomic<float> resultInput { 0.0f }, resultRms { -24.0f }, resultCrest { 12.0f };
    std::array<std::atomic<float>, 5> resultEqBands { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    void updateParameters();
    void pushSpectrum(const juce::AudioBuffer<float>& buffer);
    void analyzeAutoMix(const juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HFXAudioProcessor)
};
