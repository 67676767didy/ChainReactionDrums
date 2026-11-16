# Chain Reaction Drums — Windows VST3 source (v0.2 visual / graph update)

An original JUCE-based probabilistic drum-machine plugin inspired by the interaction model described by the user. It does not include CRD Machine's proprietary code, assets, presets, or copied source interface.

## This update

The editor has been redesigned around the supplied screenshots and reference videos:

- Large node-and-wire patch graph with rectangular instrument labels, small trigger indicators, thin probability wires, and percentage labels.
- Eight named starting nodes: kick, snare, ghost, snare roll, ghost flam, hat, sub, and knock.
- Drag from one node to another to toggle a directed probability connection.
- Drag WAV/AIFF files onto nodes or use the per-node Load button.
- A right-side take/recovery panel and a bottom 16-step activity grid establish the layout for the recording workflow shown in the reference video.
- Dark charcoal / warm-white treatment, as selected by the user, while retaining the sparse typography and diagram-like geometry of the reference.

## Working audio / sequencing features in this source

- VST3 and standalone targets through JUCE/CMake.
- Eight sample/synth voices.
- Directed probability links with bounded chain depth.
- Host tempo / transport following when a host position is available; internal tempo fallback when no host playhead exists.
- GM-style drum note output on MIDI channel 10.
- Chaos-driven graph mutation, dropout, damage/saturation, tape/filter tone, half-time probability, reverb, and master level.
- Pattern lock, graph randomization, ten program names, JSON preset import/export, and plugin state persistence.

## Not yet implemented (important)

The screenshots/videos informed the UI layout, but the following are not functional in this source build yet and are not represented as working controls: rolling five-minute audio capture, selectable recovery ranges, last-take browser, full-mix WAV recording, sample-aligned per-instrument stem export, editable per-node chance sliders, dedicated roll/burst/stutter/speed-up/slow-down generators, true tape-stop playback-rate ramps, and true arranged half-time sections. The right-side take/recovery panel and bottom activity grid are currently visual layout placeholders. This update should not be mistaken for a feature-complete clone.

Other engineering limitations inherited from the initial prototype: loaded sample playback does not yet resample to the host sample rate; the voice/sample data ownership should be made real-time safe before production use; the graph randomizer and recursive triggering need further real-time/audio-thread hardening.

## Build on Windows

1. Install **Visual Studio 2022** with *Desktop development with C++*, CMake 3.22+, and Git.
2. The CMake project fetches JUCE 8.0.6 on first configure, so internet access is required for that step.
3. Open Developer PowerShell in this folder:

   ```powershell
   cmake -S . -B build -A x64
   cmake --build build --config Release --target ChainReactionDrums_VST3
   ```

4. The VST3 bundle is normally under `build\ChainReactionDrums_artefacts\Release\VST3\Chain Reaction Drums.vst3` (the exact path can vary by generator).
5. Copy the bundle to `C:\Program Files\Common Files\VST3` and rescan plugins in FL Studio.

This archive contains source code, not a precompiled Windows VST3 binary. It has not been compiled or validated in a Windows JUCE toolchain in this environment.
