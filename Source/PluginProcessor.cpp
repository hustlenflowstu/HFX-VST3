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



void HFXAudioProcessor::Biquad::setBandPass(double sr, float freq, float q) noexcept
{
    const double fs = juce::jmax(1000.0, sr);
    const double f = juce::jlimit(10.0, fs * 0.45, (double)freq);
    const double quality = juce::jlimit(0.2, 8.0, (double)q);
    const double w0 = 2.0 * juce::MathConstants<double>::pi * f / fs;
    const double alpha = std::sin(w0) / (2.0 * quality);
    const double a0 = 1.0 + alpha;
    b0 = (float)(alpha / a0);
    b1 = 0.0f;
    b2 = (float)(-alpha / a0);
    a1 = (float)(-2.0 * std::cos(w0) / a0);
    a2 = (float)((1.0 - alpha) / a0);
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

float HFXAudioProcessor::Biquad::magnitudeAt(double sr, float frequencyHz) const noexcept
{
    const double w = 2.0 * juce::MathConstants<double>::pi
                   * juce::jlimit(1.0, sr * 0.499, (double)frequencyHz) / sr;
    const double c1 = std::cos(w), s1 = std::sin(w);
    const double c2 = std::cos(2.0 * w), s2 = std::sin(2.0 * w);
    const double nr = (double)b0 + (double)b1 * c1 + (double)b2 * c2;
    const double ni = -(double)b1 * s1 - (double)b2 * s2;
    const double dr = 1.0 + (double)a1 * c1 + (double)a2 * c2;
    const double di = -(double)a1 * s1 - (double)a2 * s2;
    const double den = dr * dr + di * di;
    return den > 1.0e-18 ? (float)std::sqrt((nr * nr + ni * ni) / den) : 1.0f;
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
    for (int band = 0; band < 5; ++band)
    {
        pEqBandFreq[(size_t)band] = apvts.getRawParameterValue("eqBandFreq" + juce::String(band));
        pEqBandGain[(size_t)band] = apvts.getRawParameterValue("eqBandGain" + juce::String(band));
        pEqBandType[(size_t)band] = apvts.getRawParameterValue("eqBandType" + juce::String(band));
        pEqBandQ[(size_t)band] = apvts.getRawParameterValue("eqBandQ" + juce::String(band));
        pEqBandDynamic[(size_t)band] = apvts.getRawParameterValue("eqBandDynamic" + juce::String(band));
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
    pTransient = apvts.getRawParameterValue("transient");
    pInputGain = apvts.getRawParameterValue("inputGain");
    pUtilityGain = apvts.getRawParameterValue("utilityGain");
    pCeiling = apvts.getRawParameterValue("ceiling");
    pLimitRelease = apvts.getRawParameterValue("limitRelease");
    pTrackMode = apvts.getRawParameterValue("trackMode");
    pStemRole = apvts.getRawParameterValue("stemRole");
    pDrumDrive = apvts.getRawParameterValue("drumDrive");
    pDrumDamp = apvts.getRawParameterValue("drumDamp");
    pDrumBoom = apvts.getRawParameterValue("drumBoom");
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
    const float bandHz[5] { 80.0f, 250.0f, 1000.0f, 5000.0f, 12000.0f };
    const char* bandNames[5] { "Low", "Low Mid", "Mid", "High Mid", "High" };
    for (int band = 0; band < 5; ++band)
    {
        const auto suffix = String(band);
        f("eqBandFreq" + suffix, String(bandNames[band]) + " Frequency", { 20.0f, 20000.0f, 1.0f, 0.35f }, bandHz[band], "Hz");
        f("eqBandGain" + suffix, String(bandNames[band]) + " Gain", { -18.0f, 18.0f, 0.1f }, 0.0f, "dB");
        f("eqBandQ" + suffix, String(bandNames[band]) + " Q", { 0.2f, 8.0f, 0.01f }, 0.707f);
        f("eqBandDynamic" + suffix, String(bandNames[band]) + " Dynamic", { 0.0f, 12.0f, 0.1f }, 0.0f, "dB");
        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID { "eqBandType" + suffix, 1 },
            String(bandNames[band]) + " Filter Type",
            StringArray { "48 dB/oct Low Cut", "12 dB/oct Low Cut", "Low Shelf",
                          "Bell", "Notch", "High Shelf", "12 dB/oct High Cut",
                          "48 dB/oct High Cut" },
            band == 0 ? 2 : (band == 4 ? 5 : 3)));
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
    f("leftPan", "Left Channel Pan", { -1.0f, 0.0f, 0.01f }, 0.0f);
    f("rightPan", "Right Channel Pan", { 0.0f, 1.0f, 0.01f }, 0.0f);
    f("monoFreq", "Mono Maker", { 20.0f, 300.0f, 1.0f }, 120.0f, "Hz");
    f("haasMs", "Haas Delay", { 0.0f, 30.0f, 0.1f }, 0.0f, "ms");
    f("transient", "Transient", { 0.0f, 1.0f, 0.01f }, 0.0f);
    f("inputGain", "Input Gain", { -12.0f, 18.0f, 0.1f }, 0.0f, "dB");
    f("utilityGain", "Utility Gain", { -24.0f, 24.0f, 0.01f }, 0.0f, "dB");
    f("ceiling", "Ceiling", { -12.0f, 0.0f, 0.1f }, -1.0f, "dB");
    f("limitRelease", "Limiter Release", { 10.0f, 500.0f, 1.0f }, 80.0f, "ms");
    // Drum Bus controls are only active for Kick, Snare, Percussion, Hats and EFX.
    f("drumDrive", "Drum Bus Drive", { 0.0f, 20.0f, 0.1f }, 3.0f, "%");
    f("drumDamp", "Drum Bus Damp", { 1500.0f, 20000.0f, 10.0f, 0.35f }, 9200.0f, "Hz");
    f("drumBoom", "Drum Bus Boom", { 0.0f, 100.0f, 0.1f }, 0.0f, "%");
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { "trackMode", 1 }, "Track Mode",
        StringArray { "Vocal", "Beat" }, 0));
    params.push_back(std::make_unique<AudioParameterChoice>(
        ParameterID { "stemRole", 1 }, "Beat Stem Role",
        StringArray { "Kick", "Snare", "Percussion", "Closed Hat", "Open Hat",
                      "EFX", "808 / Bass", "Melody" }, 0));
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
    for (auto& filter : drumDampFilters) filter.reset();
    for (auto& filter : drumBoomFilters) filter.reset();
    compEnvelope = { 0.0f, 0.0f };
    limiterGain = { 1.0f, 1.0f };
    lowState = { 0.0f, 0.0f };
    transientEnvelope = { 0.0f, 0.0f };
    toneLowState = { 0.0f, 0.0f };
    eqDynamicDetector.store(0.0f, std::memory_order_relaxed);
    stereoSideLowState = 0.0f;
    compressorGR.store(0.0f);
    limiterGR.store(0.0f);
    const int delayCapacity = juce::jmax(8192, (int)std::ceil(sr * 0.04) + 8192);
    delayBuffer.setSize(2, delayCapacity, false, true, true);
    delayBuffer.clear();
    delayWrite = 0;
    for (auto& bin : spectrum) bin.store(0.0f);
    for (auto& level : polarLevels) level.store(0.0f, std::memory_order_relaxed);
    for (auto& v : waveformHistory) v.store(0.0f);
    for (auto& v : scopeXHistory) v.store(0.0f);
    for (auto& v : scopeYHistory) v.store(0.0f);
    visualWrite.store(0);
    fftWrite = 0;
    fftHopCounter = 0;
    fftLeftRing.fill(0.0f);
    fftRightRing.fill(0.0f);
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
    return in == out
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void HFXAudioProcessor::updateParameters()
{
    // Five parametric EQ bands. Factory presets use HP/LP, shelf and bell shapes
    // to match the supplied stem-mixing references.
    for (int ch = 0; ch < 2; ++ch)
        for (int band = 0; band < 5; ++band)
        {
            const float freq = pEqBandFreq[(size_t)band]->load(std::memory_order_relaxed);
            const float q = pEqBandQ[(size_t)band] != nullptr
                ? pEqBandQ[(size_t)band]->load(std::memory_order_relaxed) : 0.707f;
            const float baseGain = pEqBandGain[(size_t)band]->load(std::memory_order_relaxed);
            const float dynamicDb = pEqBandDynamic[(size_t)band] != nullptr
                ? pEqBandDynamic[(size_t)band]->load(std::memory_order_relaxed) * eqDynamicDetector.load(std::memory_order_relaxed) : 0.0f;
            const int type = pEqBandType[(size_t)band] != nullptr
                ? juce::jlimit(0, 7, (int)std::round(pEqBandType[(size_t)band]->load(std::memory_order_relaxed))) : 3;

            for (int stage = 0; stage < 4; ++stage)
            {
                auto& filter = eq[(size_t)ch][(size_t)band][(size_t)stage];
                const bool fourPole = (type == 0 || type == 7);
                const bool firstStageOnly = (type == 1 || type == 2 || type == 3 || type == 4 || type == 5 || type == 6);
                if ((fourPole || stage == 0) && !(firstStageOnly && stage > 0))
                    filter.setFilter(sampleRateHz, freq, q, baseGain - dynamicDb, type);
                else
                    filter.setPeak(sampleRateHz, 1000.0f, 0.707f, 0.0f);
            }
        }

    const int soloBand = eqSoloBand.load(std::memory_order_relaxed);
    if (soloBand >= 0)
    {
        const float soloFreq = pEqBandFreq[(size_t)soloBand]->load(std::memory_order_relaxed);
        const float soloQ = pEqBandQ[(size_t)soloBand]->load(std::memory_order_relaxed);
        for (auto& filter : eqSoloFilters) filter.setBandPass(sampleRateHz, soloFreq, soloQ);
    }
}

void HFXAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midi);

    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels == 0 || samples == 0) return;

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, samples);

    analyzeAutoMix(buffer);

    auto captureVisualsAndMeters = [this, &buffer, channels, samples]()
    {
        const auto* l = buffer.getReadPointer(0);
        const auto* r = channels > 1 ? buffer.getReadPointer(1) : l;
        int write = visualWrite.load(std::memory_order_relaxed);
        float peakL = 0.0f, peakR = 0.0f;
        double sumSq = 0.0, sumL2 = 0.0, sumR2 = 0.0, sumLR = 0.0;
        for (int n = 0; n < samples; ++n)
        {
            const int slot = write++ % visualSize;
            const float left = l[n];
            const float right = r[n];
            waveformHistory[(size_t)slot].store(0.5f * (left + right), std::memory_order_relaxed);
            scopeXHistory[(size_t)slot].store(0.5f * (left - right), std::memory_order_relaxed);
            scopeYHistory[(size_t)slot].store(0.5f * (left + right), std::memory_order_relaxed);
            peakL = juce::jmax(peakL, std::abs(left));
            peakR = juce::jmax(peakR, std::abs(right));
            sumSq += (double)left * left;
            sumL2 += (double)left * left;
            sumR2 += (double)right * right;
            sumLR += (double)left * right;
            if (channels > 1) sumSq += (double)right * right;
        }
        visualWrite.store(write, std::memory_order_release);
        leftPeak.store(juce::Decibels::gainToDecibels(peakL, -90.0f), std::memory_order_relaxed);
        rightPeak.store(juce::Decibels::gainToDecibels(channels > 1 ? peakR : peakL, -90.0f), std::memory_order_relaxed);
        const double count = (double)juce::jmax(1, samples * channels);
        outputRms.store(juce::Decibels::gainToDecibels((float)std::sqrt(sumSq / count), -90.0f), std::memory_order_relaxed);
        const double corrDenom = std::sqrt(sumL2 * sumR2);
        const float corr = (channels > 1 && corrDenom > 1.0e-20) ? (float)(sumLR / corrDenom) : 1.0f;
        stereoCorrelationValue.store(juce::jlimit(-1.0f, 1.0f, corr), std::memory_order_relaxed);
    };

    if (pBypass->load(std::memory_order_relaxed) > 0.5f)
    {
        compressorGR.store(0.0f, std::memory_order_relaxed);
        limiterGR.store(0.0f, std::memory_order_relaxed);
        captureVisualsAndMeters();
        return;
    }

    // Broadband detector controls the optional dynamic reduction of each EQ band.
    double inputSquares = 0.0;
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int n = 0; n < samples; ++n)
            inputSquares += (double)data[n] * data[n];
    }
    const float inputRms = (float)std::sqrt(inputSquares / (double)juce::jmax(1, channels * samples));
    const float inputDb = juce::Decibels::gainToDecibels(inputRms, -90.0f);
    const float dynamicTarget = juce::jlimit(0.0f, 1.0f, (inputDb + 30.0f) / 30.0f);
    const float previousDynamic = eqDynamicDetector.load(std::memory_order_relaxed);
    eqDynamicDetector.store(previousDynamic + 0.15f * (dynamicTarget - previousDynamic),
                            std::memory_order_relaxed);
    updateParameters();

    auto* left = buffer.getWritePointer(0);
    auto* right = channels > 1 ? buffer.getWritePointer(1) : nullptr;
    const float inputGain = juce::Decibels::decibelsToGain(pInputGain->load(std::memory_order_relaxed));
    const float drive = juce::Decibels::decibelsToGain(pDrive->load(std::memory_order_relaxed));
    const float satMix = pSatMix->load(std::memory_order_relaxed);
    const float tone = pTone->load(std::memory_order_relaxed);
    const float thresholdDb = pThreshold->load(std::memory_order_relaxed);
    const float ratio = juce::jmax(1.0f, pRatio->load(std::memory_order_relaxed));
    const float attackCoeff = std::exp(-1.0f / (0.001f * pAttack->load(std::memory_order_relaxed) * (float)sampleRateHz));
    const float releaseCoeff = std::exp(-1.0f / (0.001f * pRelease->load(std::memory_order_relaxed) * (float)sampleRateHz));
    const float limiterReleaseCoeff = std::exp(-1.0f / (0.001f * pLimitRelease->load(std::memory_order_relaxed) * (float)sampleRateHz));
    const float ceiling = juce::Decibels::decibelsToGain(pCeiling->load(std::memory_order_relaxed));
    const float transientAmount = pTransient != nullptr ? pTransient->load(std::memory_order_relaxed) : 0.0f;
    const float transientCoeff = std::exp(-1.0f / (0.002f * (float)sampleRateHz));
    const float toneCoeff = std::exp(-1.0f / (0.001f * 2.0f * (float)sampleRateHz));
    const float width = pWidth->load(std::memory_order_relaxed);
    // One-direction controls combine into a constant-power stereo balance.
    const float panL = pLeftPan != nullptr ? juce::jlimit(-1.0f, 0.0f, pLeftPan->load(std::memory_order_relaxed)) : 0.0f;
    const float panR = pRightPan != nullptr ? juce::jlimit(0.0f, 1.0f, pRightPan->load(std::memory_order_relaxed)) : 0.0f;
    const float balance = juce::jlimit(-1.0f, 1.0f, panL + panR);
    const float panLeft = std::sqrt(2.0f) * std::cos((balance + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
    const float panRight = std::sqrt(2.0f) * std::sin((balance + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

    // Drum Bus mode is deliberately limited to the six percussive stem roles.
    // 808/Bass (role 6) and Melody (role 7) retain the full HFX dynamics chain.
    const int currentStemRole = (int)std::round(pStemRole->load(std::memory_order_relaxed));
    const bool beatMode = pTrackMode->load(std::memory_order_relaxed) >= 0.5f;
    const bool drumBusActive = beatMode && currentStemRole < 6;
    const bool bassOrMelodyMode = beatMode && (currentStemRole == 6 || currentStemRole == 7);
    if (drumBusActive)
    {
        const float dampHz = pDrumDamp != nullptr ? pDrumDamp->load(std::memory_order_relaxed) : 9200.0f;
        const float boomPct = pDrumBoom != nullptr ? pDrumBoom->load(std::memory_order_relaxed) : 0.0f;
        const float boomDb = boomPct * 0.10f;
        const float boomHz = 50.0f;
        for (int ch = 0; ch < 2; ++ch)
        {
            drumDampFilters[(size_t)ch].setFilter(sampleRateHz, dampHz, 0.707f, 0.0f, 6);
            drumBoomFilters[(size_t)ch].setFilter(sampleRateHz, boomHz, 0.90f, boomDb, 3);
        }
    }

    float maxCompReduction = 0.0f;
    float maxLimitReduction = 0.0f;

    for (int n = 0; n < samples; ++n)
    {
        float xL = left[n] * inputGain;
        float xR = right != nullptr ? right[n] * inputGain : xL;

        const int soloBandNow = eqSoloBand.load(std::memory_order_relaxed);
        if (soloBandNow >= 0)
        {
            // Band-solo audition: isolate the selected frequency region without
            // changing the saved EQ gain or other band settings.
            xL = eqSoloFilters[0].process(xL);
            if (right != nullptr) xR = eqSoloFilters[1].process(xR);
        }
        else
        {
            for (int band = 0; band < 5; ++band)
            {
                xL = eq[0][(size_t)band][0].process(xL);
                if (right != nullptr)
                    xR = eq[1][(size_t)band][0].process(xR);
            }
        }

        if (drumBusActive)
        {
            // Drum Bus: soft drive, high-frequency damping and a tunable low-frequency boom shelf.
            // No compressor, conventional saturator, Utility gain or limiter is inserted here.
            const float drivePct = pDrumDrive != nullptr ? pDrumDrive->load(std::memory_order_relaxed) : 3.0f;
            const float driveAmount = 1.0f + drivePct * 0.045f;
            const float driveNorm = juce::jmax(0.001f, std::tanh(driveAmount));
            xL = std::tanh(xL * driveAmount) / driveNorm;
            if (right != nullptr) xR = std::tanh(xR * driveAmount) / driveNorm;
            xL = drumDampFilters[0].process(xL);
            if (right != nullptr) xR = drumDampFilters[1].process(xR);
            xL = drumBoomFilters[0].process(xL);
            if (right != nullptr) xR = drumBoomFilters[1].process(xR);
        }
        else
        {
            // Full vocal / 808 / melody chain: linked compression, transient control,
            // saturation, Utility gain and peak limiting.
            float detectorPower = right != nullptr
                ? 0.5f * (xL * xL + xR * xR) : xL * xL;
            const float compCoeff = detectorPower > compEnvelope[0] ? attackCoeff : releaseCoeff;
            compEnvelope[0] = compCoeff * compEnvelope[0] + (1.0f - compCoeff) * detectorPower;
            compEnvelope[1] = compEnvelope[0];
            const float levelDb = juce::Decibels::gainToDecibels(std::sqrt(juce::jmax(1.0e-12f, compEnvelope[0])), -120.0f);
            const float overDb = levelDb - thresholdDb;
            constexpr float kneeDb = 6.0f;
            float reductionDb = 0.0f;
            if (overDb > -0.5f * kneeDb && overDb < 0.5f * kneeDb)
            {
                const float softened = overDb + 0.5f * kneeDb;
                reductionDb = (1.0f / ratio - 1.0f) * softened * softened / (2.0f * kneeDb);
            }
            else if (overDb >= 0.5f * kneeDb)
                reductionDb = (1.0f / ratio - 1.0f) * overDb;
            const float compGain = juce::Decibels::decibelsToGain(reductionDb);
            maxCompReduction = juce::jmax(maxCompReduction, -reductionDb);
            xL *= compGain;
            if (right != nullptr) xR *= compGain;

            transientEnvelope[0] = transientCoeff * transientEnvelope[0] + (1.0f - transientCoeff) * xL;
            xL += transientAmount * (xL - transientEnvelope[0]);
            if (right != nullptr)
            {
                transientEnvelope[1] = transientCoeff * transientEnvelope[1] + (1.0f - transientCoeff) * xR;
                xR += transientAmount * (xR - transientEnvelope[1]);
            }

            auto saturate = [&](float x, int channel)
            {
                toneLowState[(size_t)channel] = toneCoeff * toneLowState[(size_t)channel] + (1.0f - toneCoeff) * x;
                const float low = toneLowState[(size_t)channel];
                const float high = x - low;
                const float tilted = low * (1.0f - tone * 0.35f) + high * (1.0f + tone * 0.35f);
                const float norm = juce::jmax(0.001f, std::tanh(drive));
                const float wet = std::tanh(tilted * drive) / norm;
                return x * (1.0f - satMix) + wet * satMix;
            };
            xL = saturate(xL, 0);
            if (right != nullptr) xR = saturate(xR, 1);

            if (!bassOrMelodyMode)
            {
                const float utilityGainDb = pUtilityGain != nullptr ? pUtilityGain->load(std::memory_order_relaxed) : 0.0f;
                const float utilityLinear = juce::Decibels::decibelsToGain(utilityGainDb);
                xL *= utilityLinear;
                if (right != nullptr) xR *= utilityLinear;

                const float peak = right != nullptr ? juce::jmax(std::abs(xL), std::abs(xR)) : std::abs(xL);
                const float targetGain = peak > ceiling ? ceiling / juce::jmax(1.0e-9f, peak) : 1.0f;
                if (targetGain < limiterGain[0]) limiterGain[0] = targetGain;
                else limiterGain[0] = limiterReleaseCoeff * limiterGain[0] + (1.0f - limiterReleaseCoeff) * targetGain;
                limiterGain[1] = limiterGain[0];
                const float limitGR = -juce::Decibels::gainToDecibels(limiterGain[0], -90.0f);
                maxLimitReduction = juce::jmax(maxLimitReduction, limitGR);
                xL *= limiterGain[0];
                if (right != nullptr) xR *= limiterGain[0];
            }
        }

        if (right != nullptr)
        {
            // Mid/side width followed by independent constant-power left/right pan.
            const float mid = 0.5f * (xL + xR);
            const float side = 0.5f * (xL - xR);
            const float widenedSide = side * width;
            xL = (mid + widenedSide) * panLeft;
            xR = (mid - widenedSide) * panRight;
        }

        left[n] = xL;
        if (right != nullptr) right[n] = xR;
    }

    compressorGR.store(drumBusActive ? 0.0f : maxCompReduction, std::memory_order_relaxed);
    limiterGR.store(drumBusActive ? 0.0f : maxLimitReduction, std::memory_order_relaxed);
    captureVisualsAndMeters();
    pushSpectrum(buffer); // Analyze processed output so Width and Pan changes are visible.
}

void HFXAudioProcessor::pushSpectrum(const juce::AudioBuffer<float>& b)
{
    if (b.getNumChannels() == 0) return;
    const auto* l = b.getReadPointer(0);
    const auto* r = b.getNumChannels() > 1 ? b.getReadPointer(1) : l;
    constexpr int hopSize = fftSize / 4;
    for (int n = 0; n < b.getNumSamples(); ++n)
    {
        fftLeftRing[(size_t)fftWrite] = l[n];
        fftRightRing[(size_t)fftWrite] = r[n];
        if (++fftWrite == fftSize) fftWrite = 0;
        if (++fftHopCounter < hopSize) continue;
        fftHopCounter = 0;

        // Copy the most recent window in chronological order. A 75% overlap
        // gives a responsive 4x update rate while retaining 4096-point resolution.
        for (int i = 0; i < fftSize; ++i)
        {
            const int source = (fftWrite + i) % fftSize;
            const float leftSample = fftLeftRing[(size_t)source];
            const float rightSample = fftRightRing[(size_t)source];
            fftData[(size_t)i] = 0.5f * (leftSample + rightSample);
            fftLeftData[(size_t)i] = leftSample;
            fftRightData[(size_t)i] = rightSample;
        }
        std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
        std::fill(fftLeftData.begin() + fftSize, fftLeftData.end(), 0.0f);
        std::fill(fftRightData.begin() + fftSize, fftRightData.end(), 0.0f);
        window.multiplyWithWindowingTable(fftData.data(), fftSize);
        window.multiplyWithWindowingTable(fftLeftData.data(), fftSize);
        window.multiplyWithWindowingTable(fftRightData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform(fftData.data());
        fft.performFrequencyOnlyForwardTransform(fftLeftData.data());
        fft.performFrequencyOnlyForwardTransform(fftRightData.data());

        std::array<float, polarSize> sectorPeak {};
        constexpr float coherentGainScale = 1.0f / (float)(fftSize * 0.25);
        for (int i = 1; i < fftSize / 2; ++i)
        {
            const float hz = (float)i * (float)sampleRateHz / (float)fftSize;
            if (hz < 25.0f || hz > 18000.0f || hz > (float)sampleRateHz * 0.48f) continue;
            const float magL = fftLeftData[(size_t)i] * coherentGainScale;
            const float magR = fftRightData[(size_t)i] * coherentGainScale;
            const float sum = magL + magR;
            if (sum < 1.0e-7f) continue;
            const float pan = magR / sum;
            const int sector = juce::jlimit(0, polarSize - 1, (int)std::round(pan * (float)(polarSize - 1)));
            const float combined = std::sqrt(0.5f * (magL * magL + magR * magR));
            sectorPeak[(size_t)sector] = juce::jmax(sectorPeak[(size_t)sector], combined);
        }
        for (int i = 0; i < fftSize / 2; ++i)
        {
            const float magnitude = fftData[(size_t)i] * coherentGainScale;
            const float db = juce::Decibels::gainToDecibels(magnitude, -90.0f);
            spectrum[(size_t)i].store(juce::jlimit(0.0f, 1.0f, (db + 90.0f) / 90.0f), std::memory_order_relaxed);
        }
        for (int sector = 0; sector < polarSize; ++sector)
        {
            const float db = juce::Decibels::gainToDecibels(sectorPeak[(size_t)sector], -90.0f);
            const float target = juce::jlimit(0.0f, 1.0f, (db + 72.0f) / 66.0f);
            const float previous = polarLevels[(size_t)sector].load(std::memory_order_relaxed);
            const float smoothed = target > previous ? previous + 0.72f * (target - previous)
                                                      : previous + 0.24f * (target - previous);
            polarLevels[(size_t)sector].store(smoothed, std::memory_order_relaxed);
        }
        spectrumVersion.fetch_add(1, std::memory_order_release);
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
    if (band < 0 || band >= 5) return;
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
    return (band >= 0 && band < 5) ? pEqBandFreq[(size_t)band]->load(std::memory_order_relaxed) : 1000.0f;
}

float HFXAudioProcessor::eqBandGain(int band) const noexcept
{
    return (band >= 0 && band < 5) ? pEqBandGain[(size_t)band]->load(std::memory_order_relaxed) : 0.0f;
}

float HFXAudioProcessor::eqBandQ(int band) const noexcept
{
    return (band >= 0 && band < 5 && pEqBandQ[(size_t)band] != nullptr)
        ? pEqBandQ[(size_t)band]->load(std::memory_order_relaxed) : 0.707f;
}

float HFXAudioProcessor::eqResponseDbAt(float frequencyHz) const noexcept
{
    float totalGain = 1.0f;
    for (int band = 0; band < 5; ++band)
    {
        Biquad responseFilter;
        responseFilter.setPeak(sampleRateHz,
            pEqBandFreq[(size_t)band]->load(std::memory_order_relaxed),
            eqBandQ(band),
            pEqBandGain[(size_t)band]->load(std::memory_order_relaxed)
            - (pEqBandDynamic[(size_t)band] != nullptr
                ? pEqBandDynamic[(size_t)band]->load(std::memory_order_relaxed) * eqDynamicDetector.load(std::memory_order_relaxed) : 0.0f));
        totalGain *= responseFilter.magnitudeAt(sampleRateHz, frequencyHz);
    }
    return juce::Decibels::gainToDecibels(totalGain, -60.0f);
}

float HFXAudioProcessor::saturationTransfer(float input) const noexcept
{
    const float drive = juce::Decibels::decibelsToGain(pDrive->load(std::memory_order_relaxed));
    const float norm = juce::jmax(0.001f, std::tanh(drive));
    return std::tanh(input * drive) / norm;
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

float HFXAudioProcessor::polarLevelAt(int index) const noexcept
{
    return (index >= 0 && index < polarSize) ? polarLevels[(size_t)index].load(std::memory_order_relaxed) : 0.0f;
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
