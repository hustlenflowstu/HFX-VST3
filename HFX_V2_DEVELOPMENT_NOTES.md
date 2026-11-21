# HFX V2 development pass

This pass updates the JUCE editor and preset selection foundation while preserving the existing audio processor and installer workflow.

## Included in this pass

- Separate Vocal and Beat preset menus, with eight starting-point presets in each category.
- Different main visual displays for each module: EQ spectrum/response, compressor transfer curve, saturation transfer curve, stereo-field visualization, and limiter peak/ceiling display.
- Orange multi-pass glow treatment for the HFX and HF wordmarks.
- The Auto Mix completion message now describes its result as a starting point rather than claiming a perfect mix.
- The fourth redundant EQ knob is hidden because the current processor only exposes one EQ frequency, gain, and Q band.

## Important engineering status

This is an interface and preset-foundation pass, not a finished commercial DSP release. The existing processor is intentionally preserved in this pass. It currently has one peaking EQ band, a basic envelope compressor, tanh saturation, stereo width/pan/low mono/Haas processing, a sample-peak limiter, and a short RMS/peak/crest-based Auto Mix recommendation routine.

The five processors currently remain in the shared signal chain; this pass does not add per-module bypass switches or implement a multi-band EQ, true-peak/lookahead limiter, sidechain compressor, or intelligent source separation. The new module graphics are interface visualizations; the compressor/saturator/limiter drawings are illustrative rather than measured transfer/peak histories. Do not market these as completed features yet.

## Build and test

1. Commit the updated source files to a feature branch first, keeping the current successful main-branch build as rollback.
2. Run the existing Windows GitHub Actions workflow.
3. Test the generated VST3 in Ableton with audio playing through it.
4. Verify Vocal/Beat mode switching and each of the 16 preset selections.
5. Verify parameter changes are audible and plugin state recalls after closing/reopening the project.
6. Only then merge to main and build a new installer.

The Linux workspace used to prepare this ZIP did not complete CMake configuration because JUCE 8.0.6 must be fetched by CMake and the fetch timed out. Therefore this ZIP is not represented as compiled or validated. The Windows GitHub Actions build remains the authoritative compile check.
