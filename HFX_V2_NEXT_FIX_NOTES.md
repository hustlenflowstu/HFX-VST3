# HFX V2 – Next Fix Update

Implemented in source:
- Five visible EQ filter selectors attached to the existing eight-choice EQ filter parameters.
- Increased graph area and reflowed EQ controls.
- Removed Mono Maker and Haas/Delay from the Pan & Width controls.
- Added independent Left Pan and Right Pan controls with constant-power split stereo positioning.
- Width uses mid/side processing. Legacy mono/haas parameters remain in the state tree for old-session compatibility but are no longer in the signal path.

Reference audio observations:
- All five supplied files are 2:25.98, stereo, 44.1 kHz MP3.
- The full mix decodes to about -11.16 dBFS RMS and +0.55 dBFS sample peak (MP3 decoding can reconstruct peaks above 0 dBFS).
- Bass is nearly mono (L/R correlation ~1.00); the music file is wide and has negative L/R correlation (~-0.21).
These measurements describe the supplied audio, not its original processing chain.

Still outstanding:
- EQ response curve remains a simplified gain approximation, not a measured filter response.
- Compressor needs a larger gain-reduction history/transfer visualization and further parity work.
- Saturator tone/warmth needs a dedicated tonal circuit and oversampling.
- Limiter remains sample-peak based without lookahead/true-peak oversampling.
- Presets still need individual tuning and auditioning; the supplied audio is already processed, not dry source material.
- Auto Mix remains local signal-statistics analysis, not a true expert-producer or generative model; it needs per-stem analysis and explainable recommendations.
- Not compiled or tested in Ableton. Build before installing and keep the crash-fixed build as rollback.
