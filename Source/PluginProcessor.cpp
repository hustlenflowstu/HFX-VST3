#include "PluginProcessor.h"
#include "PluginEditor.h"


void HFXAudioProcessor::Biquad::setPeak(double sr, float freq, float q, float gainDb) noexcept
{
    const double f = juce::jlimit(10.0, sr * 0.45, (double)freq);
    const double quality = juce::jmax(0.1, (double)q);
    const double A = std::pow(10.0, (double)gainDb / 40.0);
    const double w0 = 2.0 * juce::MathConstants<double>::pi * f / sr;
    const double cw = std::cos(w0);
    const double sw = std::sin(w0);
    const double alpha = sw / (2.0 * quality);
    const double a0 = 1.0 + alpha / A;
    b0 = (float)((1.0 + alpha * A) / a0);
    b1 = (float)((-2.0 * cw) / a0);
    b2 = (float)((1.0 - alpha * A) / a0);
    a1 = (float)((-2.0 * cw) / a0);
    a2 = (float)((1.0 - alpha / A) / a0);
}

float HFXAudioProcessor::Biquad::process(float x) noexcept
{
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

HFXAudioProcessor::HFXAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "HFX_STATE", makeParameters())
{
    pEqFreq = apvts.getRawParameterValue("eqFreq");
    pEqGain = apvts.getRawParameterValue("eqGain");
    pEqQ = apvts.getRawParameterValue("eqQ");
    pThreshold = apvts.getRawParameterValue("threshold");
    pRatio = apvts.getRawParameterValue("ratio");
    pAttack = apvts.getRawParameterValue("attack");
    pRelease = apvts.getRawParameterValue("release");
    pDrive = apvts.getRawParameterValue("drive");
    pTone = apvts.getRawParameterValue("tone");
    pWarmth = apvts.getRawParameterValue("warmth");
    pSatMix = apvts.getRawParameterValue("satMix");
    pWidth = apvts.getRawParameterValue("width");
    pPan = apvts.getRawParameterValue("pan");
    pMonoFreq = apvts.getRawParameterValue("monoFreq");
    pHaasMs = apvts.getRawParameterValue("haasMs");
    pInputGain = apvts.getRawParameterValue("inputGain");
    pCeiling = apvts.getRawParameterValue("ceiling");
    pLimitRelease = apvts.getRawParameterValue("limitRelease");
    pTrackMode = apvts.getRawParameterValue("trackMode");
    pBypass = apvts.getRawParameterValue("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout HFXAudioProcessor::makeParameters()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    auto f = [&params](const char* id, const char* name, NormalisableRange<float> range,
                       float initial, const char* unit = "")
    {
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, range, initial,
            AudioParameterFloatAttributes().withLabel(unit)));
    };

    f("eqFreq", "EQ Frequency", { 30.0f, 18000.0f, 1.0f, 0.35f }, 250.0f, "Hz");
    f("eqGain", "EQ Gain", { -18.0f, 18.0f, 0.1f }, 0.0f, "dB");
    f("eqQ", "EQ Q", { 0.2f, 8.0f, 0.01f }, 1.0f);
    f("threshold", "Threshold", { -60.0f, 0.0f, 0.1f }, -18.0f, "dB");
    f("ratio", "Ratio", { 1.0f, 20.0f, 0.1f }, 4.0f, ":1");
    f("attack", "Attack", { 0.1f, 100.0f, 0.1f }, 12.0f, "ms");
    f("release", "Release", { 5.0f, 500.0f, 1.0f }, 90.0f, "ms");
    f("drive", "Drive", { 0.0f, 24.0f, 0.1f }, 3.0f, "dB");
    f("tone", "Tone", { -1.0f, 1.0f, 0.01f }, 0.0f);
    f("warmth", "Warmth", { 0.0f, 1.0f, 0.01f }, 0.0f);
    f("satMix", "Saturation Mix", { 0.0f, 1.0f, 0.01f }, 0.35f);
    f("width", "Width", { 0.0f, 2.0f, 0.01f }, 1.0f);
    f("pan", "Pan", { -1.0f, 1.0f, 0.01f }, 0.0f);
    f("monoFreq", "Mono Maker", { 20.0f, 300.0f, 1.0f }, 120.0f, "Hz");
    f("haasMs", "Haas Delay", { 0.0f, 30.0f, 0.1f }, 0.0f, "ms");
    f("inputGain", "Input Gain", { -12.0f, 18.0f, 0.1f }, 0.0f, "dB");
    f("ceiling", "Ceiling", { -12.0f, 0.0f, 0.1f }, -1.0f, "dB");
    f("limitRelease", "Limiter Release", { 10.0f, 500.0f, 1.0f }, 80.0f, "ms");
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { "trackMode", 1 }, "Track Mode",
        StringArray { "Vocal", "Beat" }, 0));
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { "bypass", 1 }, "Bypass", false));
    return { params.begin(), params.end() };
}

void HFXAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRateHz = sr;
    for (int c = 0; c < 2; ++c)
        eq[(size_t)c].reset();
    compEnvelope = { 0.0f, 0.0f };
    limiterGain = { 1.0f, 1.0f };
    lowState = { 0.0f, 0.0f };
    const int delayCapacity = juce::jmax(8192, (int)std::ceil(sr * 0.04) + 8192);
    delayBuffer.setSize(2, delayCapacity, false, true, true);
    delayBuffer.clear();
    delayWrite = 0;
    for (auto& bin : spectrum) bin.store(0.0f);
    fftWrite = 0;
    updateParameters();
}

void HFXAudioProcessor::releaseResources()
{
    for (auto& filter : eq) filter.reset();
}

bool HFXAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void HFXAudioProcessor::updateParameters()
{
    for (int c = 0; c < 2; ++c)
        eq[(size_t)c].setPeak(sampleRateHz, pEqFreq->load(), pEqQ->load(), pEqGain->load());
}

void HFXAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midi);

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());

    pushSpectrum(buffer);
    analyzeAutoMix(buffer);

    if (pBypass->load() > 0.5f)
        return;

    updateParameters();
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float inGain = juce::Decibels::decibelsToGain(pInputGain->load());
    const float drive = juce::Decibels::decibelsToGain(pDrive->load());
    const float satMix = pSatMix->load();
    const float tone = pTone->load();
    const float width = pWidth->load();
    const float pan = pPan->load();
    const float monoHz = pMonoFreq->load();
    const float haasMs = pHaasMs->load();
    const int delaySamples = juce::jlimit(0, delayBuffer.getNumSamples() - 1,
                                          (int)std::round(haasMs * 0.001f * (float)sampleRateHz));
    const float warmth = pWarmth->load();
    const float alpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * monoHz / (float)sampleRateHz);

    float peakL = 0.0f, peakR = 0.0f;
    double sumSq = 0.0;
    for (int c = 0; c < channels; ++c)
    {
        auto* d = buffer.getWritePointer(c);
        for (int n = 0; n < samples; ++n)
        {
            float x = d[n] * inGain;
            x = eq[(size_t)c].process(x);

            // Smooth peak-envelope compressor. Attack/release coefficients are
            // calculated from the controls and sample rate without allocating.
            const float attackMs = pAttack->load();
            const float releaseMs = pRelease->load();
            const float attackCoeff = std::exp(-1.0f / (0.001f * attackMs * (float)sampleRateHz));
            const float releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * (float)sampleRateHz));
            const float detector = std::abs(x);
            const float coeff = detector > compEnvelope[(size_t)c] ? attackCoeff : releaseCoeff;
            compEnvelope[(size_t)c] = coeff * compEnvelope[(size_t)c] + (1.0f - coeff) * detector;
            const float threshold = juce::Decibels::decibelsToGain(pThreshold->load());
            const float ratio = juce::jmax(1.0f, pRatio->load());
            float compGain = 1.0f;
            if (compEnvelope[(size_t)c] > threshold && threshold > 0.000001f)
                compGain = std::pow(compEnvelope[(size_t)c] / threshold, (1.0f / ratio) - 1.0f);
            x *= compGain;

            const float tilt = juce::Decibels::decibelsToGain(tone * 3.0f);
            const float bias = warmth * 0.12f;
            const float saturated = 0.5f * (std::tanh(x * drive * tilt + bias)
                                             + std::tanh(x * drive * tilt - bias));
            x = x * (1.0f - satMix) + saturated * satMix;

            // Fast-attack, release-smoothed sample-peak gain control.
            const float ceiling = juce::Decibels::decibelsToGain(pCeiling->load());
            const float targetGain = std::abs(x) > ceiling ? ceiling / juce::jmax(0.000001f, std::abs(x)) : 1.0f;
            const float limitReleaseMs = pLimitRelease->load();
            const float releaseCoeff = std::exp(-1.0f / (0.001f * limitReleaseMs * (float)sampleRateHz));
            if (targetGain < limiterGain[(size_t)c])
                limiterGain[(size_t)c] = targetGain;
            else
                limiterGain[(size_t)c] = releaseCoeff * limiterGain[(size_t)c] + (1.0f - releaseCoeff) * targetGain;
            x *= limiterGain[(size_t)c];
            d[n] = x;
        }
    }

    if (channels == 2)
    {
        auto* l = buffer.getWritePointer(0);
        auto* r = buffer.getWritePointer(1);
        const float gainL = std::sqrt(0.5f * (1.0f - pan)) * 1.41421356f;
        const float gainR = std::sqrt(0.5f * (1.0f + pan)) * 1.41421356f;
        for (int n = 0; n < samples; ++n)
        {
            const float lowL = lowState[0] + alpha * (l[n] - lowState[0]);
            const float lowR = lowState[1] + alpha * (r[n] - lowState[1]);
            lowState[0] = lowL;
            lowState[1] = lowR;
            const float monoLow = 0.5f * (lowL + lowR);
            const float highL = l[n] - lowL;
            const float highR = r[n] - lowR;
            const float outL = (monoLow + highL * width) * gainL;
            const float outR = (monoLow + highR * width) * gainR;

            // Delay the right channel by the selected Haas interval. The
            // delay ring is allocated in prepareToPlay, never in processBlock.
            delayBuffer.setSample(0, delayWrite, outL);
            delayBuffer.setSample(1, delayWrite, outR);
            const int readPos = (delayWrite - delaySamples + delayBuffer.getNumSamples())
                                % delayBuffer.getNumSamples();
            l[n] = delayBuffer.getSample(0, delayWrite);
            r[n] = delayBuffer.getSample(1, readPos);
            if (++delayWrite >= delayBuffer.getNumSamples()) delayWrite = 0;
        }
    }

    for (int c = 0; c < channels; ++c)
    {
        const auto* d = buffer.getReadPointer(c);
        for (int n = 0; n < samples; ++n)
        {
            const float v = d[n];
            if (c == 0) peakL = juce::jmax(peakL, std::abs(v));
            if (c == 1) peakR = juce::jmax(peakR, std::abs(v));
            sumSq += (double)v * (double)v;
        }
    }
    leftPeak.store(juce::Decibels::gainToDecibels(peakL, -90.0f));
    rightPeak.store(juce::Decibels::gainToDecibels(channels > 1 ? peakR : peakL, -90.0f));
    const double count = (double)juce::jmax(1, samples * channels);
    outputRms.store(juce::Decibels::gainToDecibels((float)std::sqrt(sumSq / count), -90.0f));
}

void HFXAudioProcessor::pushSpectrum(const juce::AudioBuffer<float>& b)
{
    if (b.getNumChannels() == 0) return;
    const auto* l = b.getReadPointer(0);
    const auto* r = b.getNumChannels() > 1 ? b.getReadPointer(1) : nullptr;
    for (int n = 0; n < b.getNumSamples(); ++n)
    {
        fftData[(size_t)fftWrite] = r != nullptr ? 0.5f * (l[n] + r[n]) : l[n];
        if (++fftWrite == fftSize)
        {
            std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
            window.multiplyWithWindowingTable(fftData.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform(fftData.data());
            for (int i = 0; i < fftSize / 2; ++i)
                spectrum[(size_t)i].store(juce::jlimit(0.0f, 1.0f, fftData[(size_t)i] / 120.0f),
                                          std::memory_order_relaxed);
            spectrumVersion.fetch_add(1, std::memory_order_release);
            fftWrite = 0;
        }
    }
}

float HFXAudioProcessor::spectrumAt(int i) const noexcept
{
    return (i >= 0 && i < fftSize / 2) ? spectrum[(size_t)i].load(std::memory_order_relaxed) : 0.0f;
}

void HFXAudioProcessor::requestAutoMix() noexcept
{
    resultReady.store(false, std::memory_order_release);
    autoMixResetPending.store(true, std::memory_order_release);
    autoMixRequested.store(true, std::memory_order_release);
}

void HFXAudioProcessor::analyzeAutoMix(const juce::AudioBuffer<float>& b)
{
    if (!autoMixRequested.load(std::memory_order_acquire)) return;
    if (autoMixResetPending.exchange(false, std::memory_order_acq_rel))
    {
        analysisCount = 0.0;
        analysisSquares = 0.0;
        analysisPeak = 0.0f;
    }

    for (int c = 0; c < b.getNumChannels(); ++c)
    {
        const auto* d = b.getReadPointer(c);
        for (int n = 0; n < b.getNumSamples(); ++n)
        {
            const float x = d[n];
            analysisSquares += (double)x * (double)x;
            analysisPeak = juce::jmax(analysisPeak, std::abs(x));
            analysisCount += 1.0;
        }
    }

    const double target = sampleRateHz * 4.0 * (double)juce::jmax(1, b.getNumChannels());
    if (analysisCount >= target)
    {
        const float rms = (float)std::sqrt(analysisSquares / juce::jmax(1.0, analysisCount));
        const float rmsDb = juce::Decibels::gainToDecibels(rms, -90.0f);
        const float peakDb = juce::Decibels::gainToDecibels(analysisPeak, -90.0f);
        const float crest = peakDb - rmsDb;
        const bool beat = pTrackMode->load() > 0.5f;

        resultEq.store(beat ? 0.4f : -0.6f);
        resultThreshold.store(juce::jlimit(-32.0f, -8.0f, rmsDb + (beat ? 9.0f : 6.0f)));
        resultDrive.store(juce::jlimit(0.5f, 5.0f, crest > 14.0f ? 1.8f : 1.1f));
        resultWidth.store(beat ? 1.08f : 1.03f);
        resultInput.store(juce::jlimit(-3.0f, 6.0f, -10.0f - peakDb));
        resultRms.store(rmsDb);
        resultCrest.store(crest);
        autoMixRequested.store(false, std::memory_order_release);
        resultReady.store(true, std::memory_order_release);
    }
}

HFXAudioProcessor::MixResult HFXAudioProcessor::takeAutoMixResult() noexcept
{
    MixResult r { resultEq.load(), resultThreshold.load(), resultDrive.load(),
                  resultWidth.load(), resultInput.load(), resultRms.load(), resultCrest.load() };
    resultReady.store(false, std::memory_order_release);
    return r;
}

void HFXAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto copy = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(copy.createXml());
    copyXmlToBinary(*xml, dest);
}

void HFXAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HFXAudioProcessor();
}
