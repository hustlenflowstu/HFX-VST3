# HFX VST3 crash fix

## Root cause found in source
`PluginProcessor` retrieved `eqBandType0` through `eqBandType4` from the APVTS, but `makeParameters()` did not create those parameters. `updateParameters()` then dereferenced the missing pointers. This is a null-pointer crash risk during plugin initialization / preparation.

## Fix applied
Created all five `AudioParameterChoice` entries in `makeParameters()`, using the eight EQ filter choices expected by `updateParameters()`.

## Validation
Source-level patch only. This archive has **not** been compiled or tested in Ableton Live. Build a new VST3 from this source before replacing the installed plugin. Keep the previous installer as rollback.
