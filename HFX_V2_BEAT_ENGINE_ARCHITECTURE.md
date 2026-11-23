# HFX V2 — Individual Stem Beat Engine Architecture

## Selected workflow: one HFX instance per stem

HFX V2 uses **Option A: Individual Stem Processing**. In Ableton Live, put one HFX instance on each audio track/stem. Choose `BEAT` as Track Mode, then select that track's `Beat Stem Role`:

1. Kick
2. Snare
3. Percussion
4. Closed Hat
5. Open Hat
6. EFX
7. 808 / Bass
8. Drum Filler
9. Other

Each plugin instance owns its own AudioProcessorValueTreeState (APVTS), so each stem's parameter values, role selection, and saved state are independent. Ableton's track routing and faders remain responsible for balancing the stems together.

## Signal path target

Each instance will use this order, with modules individually bypassable in the V2 UI:

`Input trim → role-aware filter/EQ → transient stage → compressor → saturation → stereo/mono stage → true-peak limiter → output meter`

The first code increment adds a persistent stem-role parameter and UI selector. It deliberately does not yet pretend that all target modules are implemented: the current DSP still contains the existing single peaking EQ, envelope compressor, tanh saturation, stereo width/pan/Haas, and sample-peak limiter. The next DSP increments should replace/extend these modules one at a time and validate them against offline test tones and drum samples.

## Preset data model

Preset entries should be data records, not preset-name-only menu items. Each preset must include:

- stable preset ID, display name, category, stem role, and description;
- complete values for every exposed parameter and module enable/bypass state;
- schema version, so future parameter additions can migrate old presets;
- safe input/output gain and limiter ceiling defaults.

Target beat bank: 50 presets across the 5 requested families, 10 each. The role-specific families must cover Kick, Snare, Percussion, Closed Hat, Open Hat, EFX, 808/Bass, and Drum Filler as applicable. Loading a preset must change only the current plugin instance.

## AI Auto Mix boundary

The current Auto Mix is **not AI**. It measures RMS, peak and crest factor for four seconds, then applies fixed rules. Keep it labelled as an analysis/recommendation assistant until a real inference model is integrated. For stem mode, analysis should use the selected stem role as context, return a reviewable recommendation, and only write parameters after the user presses Apply. Provide A/B snapshot, intensity, reset, and manual override. A single stem instance cannot infer the balance of all other stems; mix-wide decisions require reading multiple stems or a shared analysis bus.

## Engineering sequence

1. Stem-role parameter and per-instance persistence (this increment).
2. Role-aware preset schema and 50 fully populated beat presets.
3. Proper multi-band EQ with analyzer-driven display.
4. Drum transient shaper and role-tuned compressor.
5. Saturation/clipper options, stereo tools, oversampled true-peak limiter.
6. Role-aware Auto Mix recommendations with Apply/A-B workflow; only call it AI if a genuine model is integrated and evaluated.
7. Regression tests, latency/CPU checks, mono/stereo tests, and Ableton Live 11 validation.

## Compatibility note

The new `stemRole` parameter is additive and has a default of Kick. Existing projects should retain their previous parameters when loaded; confirm this in the host after building. No claim of successful compilation or DAW validation is made by this source update.
