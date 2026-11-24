# HFX V2 source update

## Included in this update
- Added an eight-option filter-type parameter to each of the five EQ bands.
- Added RBJ-style biquad coefficient generation for bell, notch, low shelf, high shelf, low cut and high cut.
- Added four cascaded biquad stages for the 48 dB/oct low-cut and high-cut selections.
- Connected the filter-type parameters to the audio processor and retained per-instance APVTS state.

## Important validation status
This is a source-code update, not a verified release build. It has not been compiled against the project's JUCE dependency or tested in Ableton Live. The visible EQ dropdowns and graph response drawing still require editor integration. The other requested DSP/UI work (compressor, saturator, split stereo pan, limiter lookahead/oversampling, preset audition/mastering refinement, and advanced Auto Mix) is not represented as completed by this update.

The 48 dB/oct modes use four cascaded 12 dB/oct sections. This is a practical approximation; the final response should be measured and tuned during build/test.
