# HFX V2 – Next Fix Source Update

This source update addresses the latest feedback against the current Milestone 1 project.

## Included source changes

- EQ spectrum is sampled using logarithmic frequency-to-FFT-bin mapping and a 4096-point FFT.
- EQ frequency/gain nodes can be selected and dragged directly on the EQ graph; the graph updates the matching APVTS parameters.
- The compressor detector now uses a smoothed RMS-power envelope and a 6 dB soft-knee gain computer rather than the previous peak-envelope hard-knee behavior.
- Stereo vectorscope history is captured after the processing chain, so pan and width changes affect the displayed stereo image. Bypass still updates the scope with the unprocessed signal.
- The bypass/status controls are positioned below the graph rather than overlapping it.
- Vocal and beat preset lists now contain 50 entries each. Preset selection writes five-band EQ and dynamics/saturation/stereo/limiter parameter starting points.
- Auto Mix shows an asynchronous analysis message and requests four seconds of incoming playback. Its local analysis now estimates low, mid and high band energy and applies restrained five-band EQ, threshold, drive, width and input-gain suggestions.
- HFX/HF text is drawn directly with stronger multi-pass orange glow rather than relying on a label painted over the glow.

## Important limitations

- The Auto Mix is a local adaptive signal-analysis assistant, not a connected generative AI model. It does not understand the arrangement or compare separate tracks unless the user routes/analyses them individually.
- The compressor is a functional feed-forward RMS compressor with a soft knee, but it does not yet include a full visual gain-reduction history, sidechain filters, stereo-link control, or selectable knee.
- The limiter remains a fast sample-peak limiter with release smoothing; true-peak oversampling and lookahead are future work.
- The saturator remains the existing waveshaping processor. Its transfer display is parameter-linked, but a dedicated oversampled saturation path and harmonic spectrum view remain future work.
- The 50 presets are curated starting-point parameter snapshots. They are not guaranteed mastering/mix settings; adjust them to the source material.
- Beat stem roles are still selected per plugin instance. A dedicated role-aware DSP chain for each of the nine stem types remains future work.

## Build status

Not compiled or tested in Ableton in this environment. CMake configuration started but timed out while FetchContent attempted to retrieve JUCE 8.0.6. Build and DAW validation are still required before replacing a working installation.
