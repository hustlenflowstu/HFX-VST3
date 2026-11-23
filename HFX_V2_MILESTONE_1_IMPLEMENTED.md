# HFX V2 – Milestone 1 Source Changes

This package is a source-code implementation increment based on the uploaded HFX project and the supplied UI references.

## Implemented in this increment

- Replaced the single active parametric EQ stage with five serial peaking-filter bands per channel.
- Added APVTS parameters for each band's frequency and gain (Low, Low-Mid, Mid, High-Mid, High).
- Added ten EQ controls in the editor: five frequency controls and five gain controls.
- Changed the EQ response drawing from a hard-coded curve to a response curve calculated from the five band frequency/gain values.
- Kept the incoming FFT spectrum trace in the EQ display.
- Added an audio-thread visual history buffer for the incoming waveform and stereo X/Y scope data.
- Changed the compressor display to show a live waveform and threshold line, and to display the current threshold and ratio.
- Changed the pan/width display to draw the captured stereo X/Y signal rather than a generated sine pattern.
- Changed the limiter display to use captured waveform samples and the actual ceiling parameter.
- Made the saturator transfer curve respond to the current Drive parameter.
- Strengthened the HFX/HF orange glow drawing.

## Important limitations / not yet implemented

- This is not the completed HFX V2 suite and does not yet reproduce every pixel of the supplied references.
- EQ uses five peaking filters with adjustable centre frequency and gain; per-band Q, low/high shelving, high-pass/low-pass filters, dynamic EQ and draggable graph nodes are not included yet.
- The compressor display currently visualizes the incoming waveform and threshold setting; a dedicated gain-reduction history and moving compression-envelope trace remain to be implemented.
- Pan/width scope is captured from the input signal before processing; output-side scope and correlation metering remain to be implemented.
- Saturator display is a parameter-driven transfer curve, not yet a measured harmonic spectrum/oversampling analyzer.
- Limiter waveform is the captured incoming signal; post-limiter output history and gain-reduction history remain to be implemented.
- Stem roles still need their own dedicated DSP chains and role-aware preset maps.
- The current 8 vocal and 8 beat presets have not yet been replaced with 50 + 50 fully parameterized presets.
- Auto Mix remains a rule-based analysis assistant, not a trained/embedded AI mixing model. A more advanced analyzer, recommendation explanation, intensity control, Apply/Undo and A/B workflow remain to be built.

## Build and test status

The source has been edited and statically inspected. It has **not** been compiled or tested in Ableton Live 11 in this environment. Build it with the repository's existing Windows build workflow and retain the previous known-good installer as a rollback.

## Compatibility note

The old single-band parameter IDs (`eqFreq`, `eqGain`, `eqQ`) are retained for state compatibility, but the new five-band EQ DSP reads `eqBandFreq0..4` and `eqBandGain0..4`.
