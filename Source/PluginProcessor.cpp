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



void HFXAudioProcessor::Biquad::setFilter(double sr, float freq, float q, float gainDb, int type) noexcept
{
    const float fs = (float)juce::jmax(1000.0, sr);
    const float f = juce::jlimit(10.0f, fs * 0.45f, freq);
    const float Q = juce::jmax(0.1f, q);
    const float w = 2.0f * juce::MathConstants<float>::pi * f / fs;
    const float cs = std::cos(w), sn = std::sin(w);
    const float alpha = sn / (2.0f * Q);
    const float A = std::pow(10.0f, gainDb / 40.0f);
    float nb0=1.0f, nb1=0.0f, nb2=0.0f, na0=1.0f, na1=0.0f, na2=0.0f;
    switch (type)
    {
        case 0: // 48 dB/oct low cut: four cascaded 12 dB stages
        case 1: // 12 dB/oct low cut
        case 6: // 12 dB/oct high cut
        case 7: // 48 dB/oct high cut
        {
            const bool highpass = (type == 0 || type == 1);
            nb0 = highpass ? (1.0f + cs) * 0.5f : (1.0f - cs) * 0.5f;
            nb1 = highpass ? -(1.0f + cs) : (1.0f - cs);
            nb2 = nb0;
            na0 = 1.0f + alpha; na1 = -2.0f * cs; na2 = 1.0f - alpha;
            break;
        }
        case 2: // low shelf
        case 5: // high shelf
        {
            const float beta = 2.0f * std::sqrt(A) * alpha;
            if (type == 2)
            {
                nb0=A*((A+1)-(A-1)*cs+beta); nb1=2*A*((A-1)-(A+1)*cs); nb2=A*((A+1)-(A-1)*cs-beta);
                na0=(A+1)+(A-1)*cs+beta; na1=-2*((A-1)+(A+1)*cs); na2=(A+1)+(A-1)*cs-beta;
            }
            else
            {
                nb0=A*((A+1)+(A-1)*cs+beta); nb1=-2*A*((A-1)+(A+1)*cs); nb2=A*((A+1)+(A-1)*cs-beta);
                na0=(A+1)-(A-1)*cs+beta; na1=2*((A-1)-(A+1)*cs); na2=(A+1)-(A-1)*cs-beta;
            }
            break;
        }
        case 4: // notch
            nb0=1.0f; nb1=-2.0f*cs; nb2=1.0f; na0=1.0f+alpha; na1=-2.0f*cs; na2=1.0f-alpha;
            break;
        case 3: // bell
        default:
            nb0=1.0f+alpha*A; nb1=-2.0f*cs; nb2=1.0f-alpha*A;
            na0=1.0f+alpha/A; na1=-2.0f*cs; na2=1.0f-alpha/A;
            break;
    }
    if (std::abs(na0) < 1.0e-9f) return;
    b0=nb0/na0; b1=nb1/na0; b2=nb2/na0; a1=na1/na0; a2=na2/na0;
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
    for (int band = 0; band < 8; ++band)
    {
        pEqBandFreq[(size_t)band] = apvts.getRawParameterValue("eqBandFreq" + juce::String(band));
        pEqBandGain[(size_t)band] = apvts.getRawParameterValue("eqBandGain" + juce::String(band));
        pEqBandType[(size_t)band] = apvts.getRawParameterValue("eqBandType" + juce::String(band));
        pEqBandQ[(size_t)band] = apvts.getRawParameterValue("eqBandQ" + juce::String(band));
    }
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
    pLeftPan = apvts.getRawParameterValue("leftPan");
    pRightPan = apvts.getRawParameterValue("rightPan");
    pMonoFreq = apvts.getRawParameterValue("monoFreq");
    pHaasMs = apvts.getRawParameterValue("haasMs");
    pInputGain = apvts.getRawParameterValue("inputGain");
    pCeiling = apvts.getRawParameterValue("ceiling");
    pLimitRelease = apvts.getRawParameterValue("limitRelease");
    pTrackMode = apvts.getRawParameterValue("trackMode");
    pStemRole = apvts.getRawParameterValue("stemRole");
    pBypass = apvts.getRawParameterValue("bypass");
}

juce::AudioProcessorValueTreeState::ParameterLayout HFXAudioProcessor::makeParameters()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    auto f = [&params](String id, String name, NormalisableRange<float> range,
                       float initial, const char* unit = "")
    {
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, name, range, initial,
            AudioParameterFloatAttributes().withLabel(unit)));
    };

    f("eqFreq", "EQ Frequency", { 30.0f, 18000.0f, 1.0f, 0.35f }, 250.0f, "Hz");
    f("eqGain", "EQ Gain", { -18.0f, 18.0f, 0.1f }, 0.0f, "dB");
    f("eqQ", "EQ Q", { 0.2f, 8.0f, 0.01f }, 1.0f);
    const float bandHz[8] { 35.0f, 90.0f, 180.0f, 400.0f, 1000.0f, 3000.0f, 8000.0f, 15000.0f };
    const char* bandNames[8] { "Band 1", "Band 2", "Band 3", "Band 4", "Band 5", "Band 6", "Band 7", "Band 8" };
    for (int band = 0; band < 8; ++band)
    {
        const auto suffix = String(band);
        f("eqBandFreq" + suffix, String(bandNames[band]) + " Frequency", { 20.0f, 20000.0f, 1.0f, 0.35f }, bandHz[band], "Hz");
        f("eqBandGain" + suffix, String(bandNames[band]) + " Gain", { -18.0f, 18.0f, 0.1f }, 0.0f, "dB");
        f("eqBandQ" + suffix, String(bandNames[band]) + " Q", { 0.1f, 18.0f, 0.01f, 0.35f }, 0.7071f);
        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID { "eqBandType" + suffix, 1 },
            String(bandNames[band]) + " Filter Type",
            StringArray { "48 dB/oct Low Cut", "12 dB/oct Low Cut", "Low Shelf",
                          "Bell", "Notch", "High Shelf", "12 dB/oct High Cut",
                          "48 dB/oct High Cut" },
            band == 0 ? 0 : (band == 7 ? 7 : (band == 1 ? 2 : 3))));
    }
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
    f("leftPan", "Left Channel Pan", { -1.0f, 1.0f, 0.01f }, -1.0f);
    f("rightPan", "Right Channel Pan", { -1.0f, 1.0f, 0.01f }, 1.0f);
    f("monoFreq", "Mono Maker", { 20.0f, 300.0f, 1.0f }, 120.0f, "Hz");
    f("haasMs", "Haas Delay", { 0.0f, 30.0f, 0.1f }, 0.0f, "ms");
    f("inputGain", "Input Gain", { -12.0f, 18.0f, 0.1f }, 0.0f, "dB");
    f("ceiling", "Ceiling", { -12.0f, 0.0f, 0.1f }, -1.0f, "dB");
    f("limitRelease", "Limiter Release", { 10.0f, 500.0f, 1.0f }, 80.0f, "ms");
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { "trackMode", 1 }, "Track Mode",
        StringArray { "Vocal", "Beat" }, 0));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { "stemRole", 1 }, "Beat Stem Role",
        StringArray { "Kick", "Snare", "Percussion", "Closed Hat", "Open Hat",
                      "EFX", "808 / Bass", "Drum Filler", "Other" }, 0));
    params.push_back(std::make_unique<AudioParameterBool>(
        ParameterID { "bypass", 1 }, "Bypass", false));
    return { params.begin(), params.end() };
}

void HFXAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRateHz = sr;
    for (auto& channel : eq)
        for (auto& band : channel)
            for (auto& filter : band) filter.reset();
    compEnvelope = { 0.0f, 0.0f };
    limiterGain = { 1.0f, 1.0f };
    lowState = { 0.0f, 0.0f };
    const int delayCapacity = juce::jmax(8192, (int)std::ceil(sr * 0.04) + 8192);
    delayBuffer.setSize(2, delayCapacity, false, true, true);
    delayBuffer.clear();
    delayWrite = 0;
    for (auto& bin : spectrum) bin.store(0.0f);
    for (auto& v : waveformHistory) v.store(0.0f);
    for (auto& v : scopeXHistory) v.store(0.0f);
    for (auto& v : scopeYHistory) v.store(0.0f);
    visualWrite.store(0);
    fftWrite = 0;
    updateParameters();
}

void HFXAudioProcessor::releaseResources()
{
    for (auto& channel : eq)
        for (auto& band : channel)
            for (auto& filter : band) filter.reset();
}

bool HFXAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void HFXAudioProcessor::updateParameters()
{
    for (int ch = 0; ch < 2; ++ch)
        for (int band = 0; band < 8; ++band)
        {
            auto* typeParam = pEqBandType[(size_t)band];
            auto* freqParam = pEqBandFreq[(size_t)band];
            auto* gainParam = pEqBandGain[(size_t)band];
            auto* qParam = pEqBandQ[(size_t)band];
            if (typeParam == nullptr || freqParam == nullptr || gainParam == nullptr || qParam == nullptr)
                continue;

            const int type = (int)typeParam->load(std::memory_order_relaxed);
            const int stages = (type == 0 || type == 7) ? 4 : 1;
            for (int stage = 0; stage < 4; ++stage)
            {
                auto& filter = eq[(size_t)ch][(size_t)band][(size_t)stage];
                if (stage < stages)
                    filter.setFilter(sampleRateHz,
                                     freqParam->load(std::memory_order_relaxed),
                                     qParam->load(std::memory_order_relaxed),
                                     gainParam->load(std::memory_order_relaxed),
                                     type == 0 ? 1 : (type == 7 ? 6 : type));
                else
                    filter.setPeak(sampleRateHz, 1000.0f, 0.7071f, 0.0f);
            }
        }
}

void HFXAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midi);

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, buffer.getNumSamples());

    pushSpectrum(buffer);
    analyzeAutoMix(buffer);
    // Capture the incoming waveform and stereo image for the editor.
    {
        const int nSamples = buffer.getNumSamples();
        const auto* inL = buffer.getReadPointer(0);
        const auto* inR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : inL;
        int write = visualWrite.load(std::memory_order_relaxed);
        for (int n = 0; n < nSamples; ++n)
        {
            const int slot = write++ % visualSize;
            const float l = inL[n], r = inR[n];
            waveformHistory[(size_t)slot].store(0.5f * (l + r), std::memory_order_relaxed);
        }
        visualWrite.store(write, std::memory_order_release);
    }

    auto captureStereoScope = [this, &buffer]() noexcept
    {
        const int nSamples = buffer.getNumSamples();
        const auto* inL = buffer.getReadPointer(0);
        const auto* inR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : inL;
        int write = visualWrite.load(std::memory_order_relaxed);
        for (int n = 0; n < nSamples; ++n)
        {
            const int slot = ((write - nSamples + n) % visualSize + visualSize) % visualSize;
            const float l = inL[n], r = inR[n];
            scopeXHistory[(size_t)slot].store(0.5f * (l - r), std::memory_order_relaxed);
            scopeYHistory[(size_t)slot].store(0.5f * (l + r), std::memory_order_relaxed);
        }
    };
    if (pBypass->load() > 0.5f)
    {
        captureStereoScope();
        return;
    }

    // Defensive guard: never dereference APVTS raw parameter pointers unless all
    // required parameters were created successfully by this processor instance.
    if (pInputGain == nullptr || pDrive == nullptr || pSatMix == nullptr ||
        pTone == nullptr || pWidth == nullptr || pLeftPan == nullptr ||
        pRightPan == nullptr || pAttack == nullptr || pRelease == nullptr ||
        pThreshold == nullptr || pRatio == nullptr || pCeiling == nullptr ||
        pLimitRelease == nullptr)
        return;

    for (int band = 0; band < 8; ++band)
        if (pEqBandFreq[(size_t) band] == nullptr ||
            pEqBandGain[(size_t) band] == nullptr ||
            pEqBandType[(size_t) band] == nullptr ||
            pEqBandQ[(size_t) band] == nullptr)
            return;

    updateParameters();
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float inGain = juce::Decibels::decibelsToGain(pInputGain->load());
    const float drive = juce::Decibels::decibelsToGain(pDrive->load());
    const float satMix = pSatMix->load();
    const float tone = pTone->load();
    const float width = pWidth->load();
    const float leftPan = juce::jlimit(-1.0f, 1.0f, pLeftPan->load());
    const float rightPan = juce::jlimit(-1.0f, 1.0f, pRightPan->load());
    const float leftToL = std::cos((leftPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
    const float leftToR = std::sin((leftPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
    const float rightToL = std::cos((rightPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
    const float rightToR = std::sin((rightPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
    const float warmth = pWarmth->load();
    const float attackMs = pAttack->load();
    const float releaseMs = pRelease->load();
    const float attackCoeff = std::exp(-1.0f / (0.001f * attackMs * (float)sampleRateHz));
    const float releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * (float)sampleRateHz));
    const float thresholdDb = pThreshold->load();
    const float ratio = juce::jmax(1.0f, pRatio->load());
    constexpr float kneeDb = 6.0f;

    float peakL = 0.0f, peakR = 0.0f;
    double sumSq = 0.0;
    for (int c = 0; c < channels; ++c)
    {
        auto* d = buffer.getWritePointer(c);
        for (int n = 0; n < samples; ++n)
        {
            float x = d[n] * inGain;
            for (int band = 0; band < 8; ++band)
                for (int stage = 0; stage < 4; ++stage)
                    x = eq[(size_t)c][(size_t)band][(size_t)stage].process(x);

            // RMS-style detector with a soft knee and logarithmic gain computer.
            const float detectorPower = x * x;
            const float coeff = detectorPower > compEnvelope[(size_t)c] ? attackCoeff : releaseCoeff;
            compEnvelope[(size_t)c] = coeff * compEnvelope[(size_t)c] + (1.0f - coeff) * detectorPower;
            const float levelDb = juce::Decibels::gainToDecibels(std::sqrt(juce::jmax(1.0e-12f, compEnvelope[(size_t)c])), -120.0f);
            const float overDb = levelDb - thresholdDb;
            float reductionDb = 0.0f;
            if (kneeDb > 0.0f && overDb > -0.5f * kneeDb && overDb < 0.5f * kneeDb)
            {
                const float softened = overDb + 0.5f * kneeDb;
                reductionDb = (1.0f / ratio - 1.0f) * softened * softened / (2.0f * kneeDb);
            }
            else if (overDb >= 0.5f * kneeDb)
                reductionDb = (1.0f / ratio - 1.0f) * overDb;
            x *= juce::Decibels::decibelsToGain(reductionDb);

            const float tilt = juce::Decibels::decibelsToGain(tone * 3.0f);
            const float bias = warmth * 0.12f;
            const float saturated = 0.5f * (std::tanh(x * drive * tilt + bias)
                                             + std::tanh(x * drive * tilt - bias));
            x = x * (1.0f - satMix) + saturated * satMix;

            // Fast-attack, release-smoothed sample-peak gain control.
            const float ceiling = juce::Decibels::decibelsToGain(pCeiling->load());
            const float targetGain = std::abs(x) > ceiling ? ceiling / juce::jmax(0.000001f, std::abs(x)) : 1.0f;
            const float limitReleaseMs = pLimitRelease->load();
            const float limiterReleaseCoeff = std::exp(-1.0f / (0.001f * limitReleaseMs * (float)sampleRateHz));
            if (targetGain < limiterGain[(size_t)c])
                limiterGain[(size_t)c] = targetGain;
            else
                limiterGain[(size_t)c] = limiterReleaseCoeff * limiterGain[(size_t)c]
                                       + (1.0f - limiterReleaseCoeff) * targetGain;
            x *= limiterGain[(size_t)c];
            d[n] = x;
        }
    }

    if (channels == 2)
    {
        auto* l = buffer.getWritePointer(0);
        auto* r = buffer.getWritePointer(1);
        for (int n = 0; n < samples; ++n)
        {
            const float mid = 0.5f * (l[n] + r[n]);
            const float side = 0.5f * (l[n] - r[n]) * width;
            const float widenedL = mid + side;
            const float widenedR = mid - side;
            l[n] = widenedL * leftToL + widenedR * rightToL;
            r[n] = widenedL * leftToR + widenedR * rightToR;
        }
    }

    captureStereoScope();

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
                spectrum[(size_t)i].store(juce::jlimit(0.0f, 1.0f, fftData[(size_t)i] / 480.0f),
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

float HFXAudioProcessor::spectrumAtFrequency(float hz) const noexcept
{
    const float safeHz = juce::jlimit(20.0f, (float)(sampleRateHz * 0.49), hz);
    const int bin = juce::jlimit(0, fftSize / 2 - 1,
        (int)std::round(safeHz * (float)fftSize / (float)sampleRateHz));
    return spectrumAt(bin);
}

void HFXAudioProcessor::setEqBandFromGraph(int band, float frequencyHz, float gainDb)
{
    if (band < 0 || band >= 8) return;
    auto set = [this](const juce::String& id, float value)
    {
        if (auto* parameter = apvts.getParameter(id))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
            parameter->endChangeGesture();
        }
    };
    set("eqBandFreq" + juce::String(band), juce::jlimit(20.0f, 20000.0f, frequencyHz));
    set("eqBandGain" + juce::String(band), juce::jlimit(-18.0f, 18.0f, gainDb));
}

float HFXAudioProcessor::eqBandFrequency(int band) const noexcept
{
    return (band >= 0 && band < 8 && pEqBandFreq[(size_t)band] != nullptr)
        ? pEqBandFreq[(size_t)band]->load(std::memory_order_relaxed) : 1000.0f;
}

float HFXAudioProcessor::eqBandQ(int band) const noexcept
{
    return (band >= 0 && band < 8 && pEqBandQ[(size_t)band] != nullptr)
        ? pEqBandQ[(size_t)band]->load(std::memory_order_relaxed) : 0.7071f;
}

float HFXAudioProcessor::eqBandGain(int band) const noexcept
{
    return (band >= 0 && band < 8 && pEqBandGain[(size_t)band] != nullptr)
        ? pEqBandGain[(size_t)band]->load(std::memory_order_relaxed) : 0.0f;
}

float HFXAudioProcessor::waveformAt(int index) const noexcept
{
    const int write = visualWrite.load(std::memory_order_acquire);
    const int slot = (write - visualSize + index + visualSize * 2) % visualSize;
    return waveformHistory[(size_t)slot].load(std::memory_order_relaxed);
}

float HFXAudioProcessor::scopeXAt(int index) const noexcept
{
    const int write = visualWrite.load(std::memory_order_acquire);
    const int slot = (write - visualSize + index + visualSize * 2) % visualSize;
    return scopeXHistory[(size_t)slot].load(std::memory_order_relaxed);
}

float HFXAudioProcessor::scopeYAt(int index) const noexcept
{
    const int write = visualWrite.load(std::memory_order_acquire);
    const int slot = (write - visualSize + index + visualSize * 2) % visualSize;
    return scopeYHistory[(size_t)slot].load(std::memory_order_relaxed);
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
        analysisLowSquares = analysisMidSquares = analysisHighSquares = 0.0;
        analysisLowState = { 0.0f, 0.0f };
        analysisHighState = { 0.0f, 0.0f };
        analysisPeak = 0.0f;
    }

    const float lowAlpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 180.0f / (float)sampleRateHz);
    const float highAlpha = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 6500.0f / (float)sampleRateHz);
    for (int c = 0; c < b.getNumChannels(); ++c)
    {
        const auto* d = b.getReadPointer(c);
        const int stateChannel = juce::jmin(c, 1);
        for (int n = 0; n < b.getNumSamples(); ++n)
        {
            const float x = d[n];
            analysisSquares += (double)x * (double)x;
            analysisPeak = juce::jmax(analysisPeak, std::abs(x));
            analysisLowState[(size_t)stateChannel] += lowAlpha * (x - analysisLowState[(size_t)stateChannel]);
            analysisHighState[(size_t)stateChannel] += highAlpha * (x - analysisHighState[(size_t)stateChannel]);
            const float low = analysisLowState[(size_t)stateChannel];
            const float high = x - analysisHighState[(size_t)stateChannel];
            const float mid = x - low - high;
            analysisLowSquares += (double)low * low;
            analysisMidSquares += (double)mid * mid;
            analysisHighSquares += (double)high * high;
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

        const float bandCount = juce::jmax(1.0f, (float)analysisCount);
        const float lowDb = juce::Decibels::gainToDecibels(std::sqrt((float)(analysisLowSquares / bandCount)), -90.0f);
        const float midDb = juce::Decibels::gainToDecibels(std::sqrt((float)(analysisMidSquares / bandCount)), -90.0f);
        const float highDb = juce::Decibels::gainToDecibels(std::sqrt((float)(analysisHighSquares / bandCount)), -90.0f);
        // Adaptive tonal-balance suggestions: restrained corrections, never an automatic mastering target.
        const float lowCorrection = juce::jlimit(-3.0f, 3.0f, (midDb - lowDb) * 0.18f);
        const float midCorrection = juce::jlimit(-2.0f, 2.0f, ((lowDb + highDb) * 0.5f - midDb) * 0.12f);
        const float highCorrection = juce::jlimit(-3.0f, 3.0f, (midDb - highDb) * 0.16f);
        resultEqBands[0].store(lowCorrection);
        resultEqBands[1].store(lowCorrection * 0.35f);
        resultEqBands[2].store(midCorrection);
        resultEqBands[3].store(highCorrection * 0.55f);
        resultEqBands[4].store(highCorrection);
        resultEq.store(lowCorrection);
        resultThreshold.store(juce::jlimit(-32.0f, -8.0f, rmsDb + (beat ? 9.0f : 6.0f)));
        resultDrive.store(juce::jlimit(0.5f, 5.0f, 0.8f + juce::jmax(0.0f, crest - 8.0f) * 0.12f));
        resultWidth.store(beat ? 1.05f : 1.02f);
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
                  resultWidth.load(), resultInput.load(), resultRms.load(), resultCrest.load(),
                  resultEqBands[0].load(), resultEqBands[1].load(), resultEqBands[2].load(),
                  resultEqBands[3].load(), resultEqBands[4].load() };
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


juce::AudioProcessorEditor* HFXAudioProcessor::createEditor()
{
    return new HFXAudioProcessorEditor(*this);
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HFXAudioProcessor();
}
