# Adlib Tracker 2 (A2M/A2T) Effect Commands

Each effect consists of a command byte (effect_def) and a data byte (effect).
Effect range depends on format version:

| Version | Max Channels | Rows | Bytes/Event | Effect Range |
|---------|-------------|------|-------------|--------------|
| 1-4     | 9           | 64   | 4           | 0-15         |
| 5-8     | 18          | 64   | 4           | 0-35         |
| 9-14    | 20          | 256  | 6 (2 slots) | 0-39 → 0-49  |

## Effect Character Mapping

Effects are represented by ASCII characters in the tracker UI:

```
"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ&%!@=#$~^`><"
 0 1 2 3 4 5 6 7 8 9 101112131415161718192021222324252627282930313233343536373839404142434445464748
```

For versions 9+, each event row has two effect slots (effect 1 and effect 2),
enabling combined effects per note.

---

## Standard Effects (0-47)

| # | Char | Name | Data Format | Description |
|---|------|------|-------------|-------------|
| 0 | `0` | **Arpeggio** | `xy` — x: semitone add 1, y: semitone add 2 | Cycles through note, note+x, note+y each tick |
| 1 | `1` | **FSlideUp** | `xx` — speed | Frequency slide up (portamento up) |
| 2 | `2` | **FSlideDown** | `xx` — speed | Frequency slide down (portamento down) |
| 3 | `3` | **TonePortamento** | `xx` — speed | Slides frequency toward the next note's pitch |
| 4 | `4` | **Vibrato** | `xy` — x: speed, y: depth | Adds sinusoidal pitch oscillation |
| 5 | `5` | **TPortamVolSlide** | `xy` — x: vol slide up, y: vol slide down | Tone portamento + volume slide |
| 6 | `6` | **VibratoVolSlide** | `xy` — x: vol slide up, y: vol slide down | Vibrato + volume slide |
| 7 | `7` | **FSlideUpFine** | `xx` — speed | Fine frequency slide up |
| 8 | `8` | **FSlideDownFine** | `xx` — speed | Fine frequency slide down |
| 9 | `9` | **SetModulatorVol** | `xx` — volume (0-63) | Sets modulator output level |
| 10 | `A` | **VolSlide** | `xy` — x: up, y: down | Slides carrier volume up or down |
| 11 | `B` | **PositionJump** | `xx` — order | Jumps to specified pattern order |
| 12 | `C` | **SetInsVolume** | `xx` — volume (0-63) | Sets overall instrument volume |
| 13 | `D` | **PatternBreak** | `xx` — row | Breaks pattern, jumps to row on next order |
| 14 | `E` | **SetTempo** | `xx` — BPM | Sets song tempo in BPM |
| 15 | `F` | **SetSpeed** | `xx` — ticks/row | Sets speed (ticks per row) |
| 16 | `G` | **TPortamVSlideFine** | `xy` — x: up, y: down | Tone portamento + fine volume slide |
| 17 | `H` | **VibratoVSlideFine** | `xy` — x: up, y: down | Vibrato + fine volume slide |
| 18 | `I` | **SetCarrierVol** | `xx` — volume (0-63) | Sets carrier output level |
| 19 | `J` | **SetWaveform** | `xy` — x: mod waveform, y: car waveform | Sets waveform type (0-7) for both operators |
| 20 | `K` | **VolSlideFine** | `xy` — x: up, y: down | Fine volume slide |
| 21 | `L` | **RetrigNote** | `xx` — retrig speed | Retriggers the note every x ticks |
| 22 | `M` | **Tremolo** | `xy` — x: speed, y: depth | Adds sinusoidal volume oscillation |
| 23 | `N` | **Tremor** | `xy` — x: on time, y: off time | Rapid on/off volume switching |
| 24 | `O` | **ArpggVSlide** | `xy` — x: up, y: down | Arpeggio + volume slide |
| 25 | `P` | **ArpggVSlideFine** | `xy` — x: up, y: down | Arpeggio + fine volume slide |
| 26 | `Q` | **MultiRetrigNote** | `xx` — retrig speed | Multi-note retrigger |
| 27 | `R` | **FSlideUpVSlide** | `xy` — x: up, y: down | Freq slide up + volume slide |
| 28 | `S` | **FSlideDownVSlide** | `xy` — x: up, y: down | Freq slide down + volume slide |
| 29 | `T` | **FSlUpFineVSlide** | `xy` — x: up, y: down | Fine freq slide up + volume slide |
| 30 | `U` | **FSlDownFineVSlide** | `xy` — x: up, y: down | Fine freq slide down + volume slide |
| 31 | `V` | **FSlUpVSlF** | `xy` — x: up, y: down | Freq slide up + fine volume slide |
| 32 | `W` | **FSlDownVSlF** | `xy` — x: up, y: down | Freq slide down + fine volume slide |
| 33 | `X` | **FSlUpFineVSlF** | `xy` — x: up, y: down | Fine freq slide up + fine volume slide |
| 34 | `Y` | **FSlDownFineVSlF** | `xy` — x: up, y: down | Fine freq slide down + fine volume slide |
| 35 | `Z` | **Extended** | `--` — see sub-commands | Extended command set (see below) |
| 36 | `&` | **Extended2** | `--` — see sub-commands | Extended command set 2 (see below) |
| 37 | `%` | **SetGlobalVolume** | `xx` — volume (0-127) | Sets global playback volume |
| 38 | `!` | **SwapArpeggio** | `xx` — arpeggio table | Swaps channel's arpeggio table |
| 39 | `@` | **SwapVibrato** | `xx` — vibrato table | Swaps channel's vibrato table |
| 40 | `=` | **ForceInsVolume** | `xx` — volume (0-63) | Forces instrument volume override |
| 41 | `#` | **Extended3** | `--` — see sub-commands | Extended command set 3 (see below) |
| 42 | `$` | **ExtraFineArpeggio** | `xy` — x: note 1, y: note 2 | Finer arpeggio (also works with x/y = note 1/2) |
| 43 | `~` | **ExtraFineVibrato** | `xy` — x: speed, y: depth | Extra fine vibrato |
| 44 | `^` | **ExtraFineTremolo** | `xy` — x: speed, y: depth | Extra fine tremolo |
| 45 | `` ` `` | **SetCustomSpeedTab** | `xx` — table selector | Sets custom speed table |
| 46 | `>` | **GlobalFSlideUp** | `xx` — speed | Global (all channels) frequency slide up |
| 47 | `<` | **GlobalFSlideDown** | `xx` — speed | Global (all channels) frequency slide down |

---

## Extended Command `Z` (ef_Extended = 35)

Format: `Zx` where `x` is the sub-command, data byte is the parameter.

| Sub | Notation | Name | Data | Description |
|-----|----------|------|------|-------------|
| 0 | `Z0` | SetTremDepth | `x` — 0=off, 1=on | Sets tremolo depth |
| 1 | `Z1` | SetVibDepth | `x` — 0=off, 1=on | Sets vibrato depth |
| 2 | `Z2` | SetAttckRateM | `x` — 0-15 | Sets modulator attack rate |
| 3 | `Z3` | SetDecayRateM | `x` — 0-15 | Sets modulator decay rate |
| 4 | `Z4` | SetSustnLevelM | `x` — 0-15 | Sets modulator sustain level |
| 5 | `Z5` | SetRelRateM | `x` — 0-15 | Sets modulator release rate |
| 6 | `Z6` | SetAttckRateC | `x` — 0-15 | Sets carrier attack rate |
| 7 | `Z7` | SetDecayRateC | `x` — 0-15 | Sets carrier decay rate |
| 8 | `Z8` | SetSustnLevelC | `x` — 0-15 | Sets carrier sustain level |
| 9 | `Z9` | SetRelRateC | `x` — 0-15 | Sets carrier release rate |
| 10 | `ZA` | SetFeedback | `x` — 0-7 | Sets feedback level |
| 11 | `ZB` | SetPanningPos | `x` — 0=center, 1=left, 2=right | Sets panning position |
| 12 | `ZC` | PatternLoop | `x` — loop count | Pattern loop (set/loop back to marker) |
| 13 | `ZD` | PatternLoopRec | `x` — loop count | Pattern loop with recursion |
| 14 | `ZE` | MacroKOffLoop | `x` — 0=disable, 1=enable | Key-off macro loop control |
| 15 | `ZF` | ExtendedCmd | `x` — sub-command | Extended command set (see ZFx) |

### Extended Command `ZF` sub-commands

| Sub | Notation | Name | Description |
|-----|----------|------|-------------|
| 0 | `ZF0` | RSS | Release Sustaining Sound |
| 1 | `ZF1` | ResetVol | Reset instrument volume |
| 2 | `ZF2` | LockVol | Lock volume |
| 3 | `ZF3` | UnlockVol | Unlock volume |
| 4 | `ZF4` | LockVP | Lock volume+peak |
| 5 | `ZF5` | UnlockVP | Unlock volume+peak |
| 6 | `ZF6` | VSlide_mod | Volume slide type: modulator |
| 7 | `ZF7` | VSlide_car | Volume slide type: carrier |
| 8 | `ZF8` | VSlide_def | Volume slide type: default |
| 9 | `ZF9` | LockPan | Lock panning position |
| 10 | `ZFA` | UnlockPan | Unlock panning position |
| 11 | `ZFB` | VibrOff | Vibrato off (reset frequency) |
| 12 | `ZFC` | TremOff | Tremolo off (reset volume) |
| 13 | `ZFD` | FineVibr | Extra fine vibrato mode (w/ `>xx` / `<xx` → fine freq slide) |
| 14 | `ZFE` | FineTrem | Extra fine tremolo mode (w/ `>xx` / `<xx` → extra fine freq slide) |
| 15 | `ZFF` | NoRestart | No force restart on note (w/ `!xx` / `@xx`) |

---

## Extended2 Command `&` (ef_Extended2 = 36)

| Sub | Notation | Name | Data | Description |
|-----|----------|------|------|-------------|
| 0 | `&0` | PatDelayFrame | `x` — frames | Pattern delay by x frames |
| 1 | `&1` | PatDelayRow | `x` — rows | Pattern delay by x rows |
| 2 | `&2` | NoteDelay | `x` — ticks | Delays note start by x ticks |
| 3 | `&3` | NoteCut | `x` — ticks | Cuts note after x ticks |
| 4 | `&4` | FineTuneUp | `x` — amount | Fine tune up |
| 5 | `&5` | FineTuneDown | `x` — amount | Fine tune down |
| 6 | `&6` | GlVolSlideUp | `x` — speed | Global volume slide up |
| 7 | `&7` | GlVolSlideDn | `x` — speed | Global volume slide down |
| 8 | `&8` | GlVolSlideUpF | `x` — speed | Global volume slide up (fine) |
| 9 | `&9` | GlVolSlideDnF | `x` — speed | Global volume slide down (fine) |
| 10 | `&A` | GlVolSldUpXF | `x` — speed | Global volume slide up (extra fine) |
| 11 | `&B` | GlVolSldDnXF | `x` — speed | Global volume slide down (extra fine) |
| 12 | `&C` | VolSlideUpXF | `x` — speed | Channel volume slide up (extra fine) |
| 13 | `&D` | VolSlideDnXF | `x` — speed | Channel volume slide down (extra fine) |
| 14 | `&E` | FreqSlideUpXF | `x` — speed | Frequency slide up (extra fine) |
| 15 | `&F` | FreqSlideDnXF | `x` — speed | Frequency slide down (extra fine) |

---

## Extended3 Command `#` (ef_Extended3 = 41)

Direct OPL register manipulation for individual FM parameters.

| Sub | Notation | Name | Data | Description |
|-----|----------|------|------|-------------|
| 0 | `#0` | SetConnection | `x` — 0=FM, 1=AM | Sets modulator-carrier connection type |
| 1 | `#1` | SetMultipM | `x` — 0-15 | Sets modulator frequency multiplier |
| 2 | `#2` | SetKslM | `x` — 0-3 | Sets modulator key scale level |
| 3 | `#3` | SetTremoloM | `x` — 0/1 | Sets modulator tremolo on/off |
| 4 | `#4` | SetVibratoM | `x` — 0/1 | Sets modulator vibrato on/off |
| 5 | `#5` | SetKsrM | `x` — 0/1 | Sets modulator key scale rate |
| 6 | `#6` | SetSustainM | `x` — 0/1 | Sets modulator sustain on/off |
| 7 | `#7` | SetMultipC | `x` — 0-15 | Sets carrier frequency multiplier |
| 8 | `#8` | SetKslC | `x` — 0-3 | Sets carrier key scale level |
| 9 | `#9` | SetTremoloC | `x` — 0/1 | Sets carrier tremolo on/off |
| 10 | `#A` | SetVibratoC | `x` — 0/1 | Sets carrier vibrato on/off |
| 11 | `#B` | SetKsrC | `x` — 0/1 | Sets carrier key scale rate |
| 12 | `#C` | SetSustainC | `x` — 0/1 | Sets carrier sustain on/off |

---

## Effect Groups (for memory/recall with x00)

Related effects share a group so that `x00` recalls the last non-zero value
of a compatible effect in the same group.

| Group | Effects |
|-------|---------|
| ArpVolSlide | `O` (24), `P` (25) |
| FSlideVolSlide | `R` (27), `V` (31), `S` (28), `W` (32), `T` (29), `X` (33), `U` (30), `Y` (34) |
| TonePortamento | `3` (3) |
| Vibrato | `4` (4), `~` (43) |
| Tremolo | `M` (22), `^` (44) |
| VibratoVolSlide | `6` (6), `H` (17) |
| PortaVolSlide | `5` (5), `G` (16) |
| RetrigNote | `L` (21), `Q` (26) |

---

## Global Effects with Modifiers

Certain effects combine with `ZFD`/`ZFE` in the **other** slot to modify behavior:

| Effect | + slot 2 command | Result |
|--------|-----------------|--------|
| `>xx` (46) | `ZFD` | FSlideUpFine (7) |
| `>xx` (46) | `ZFE` | GlobalFreqSlideUpXF (48) |
| `<xx` (47) | `ZFD` | FSlideDownFine (8) |
| `<xx` (47) | `ZFE` | GlobalFreqSlideDnXF (49) |
| `>xx` / `<xx` | `ZE7` | Forces BPM slide mode |

---

## Note Types

| Value | Meaning |
|-------|---------|
| 0 | Empty / no note |
| 1-96 | Standard note (C-0 to B-7) |
| 0x90 + n | Fixed note (always plays note n regardless of pitch) |
| 0xFF | Key-off |
