# Adlib Tracker 2 (AT2) File Format Reference

> Compiled from `TECHINFO.DOC` (techinfo.htm) by Stanislav Baranec (SubZ3ro),
> and `adtrack2.mht` — the complete Adlib Tracker 2 documentation by Stanislav Baranec.

---

## Overview

All AT2 data files (A2M, A2T, A2P, A2i, A2B, A2W, A2F) share a common structure:
a fixed-length **header** (with ID string, CRC, format version, block-length fields)
followed by one or more **compressed data blocks**.

### Compression

| Format version | Compression |
|---------------|-------------|
| 1, 5          | SixPack     |
| 2, 6          | LZW         |
| 3, 7          | LZSS        |
| 4, 8          | none (raw)  |
| 9+            | aPLib (Jibz) |

### Version Summary

| Ext | Versions | Description |
|-----|----------|-------------|
| A2M | 1–11     | Module (full song) |
| A2T | 1–11     | Tiny module |
| A2P | 1–10     | Pattern |
| A2i | 1–9      | Instrument |
| A2B | 1–9      | Instrument bank |
| A2W | 1–2      | Instrument bank with macros |
| A2F | 1        | Instrument with FM-register macro |

**Note:** Versions 12–14 use the same format as version 11 (the header data-block
count and structure are identical; the difference is in supported features at
the tracker level).

### Common Compatibility Constants

- Octave frequency range: `0x156`–`0x2AE`
- Vibrato/Tremolo speed table (32 entries, sine half-wave):
  `0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,253,250,244,235,224,212,197,180,161,141,120,97,74,49,24`
- IRQ timer frequency: `1193180` Hz

---

## A2M — Module Format

ID string: `_a2module_` (10 bytes)
Header size: 16 bytes (v1–8) / variable (v9+)

### Header

| Offset | Size | Field |
|--------|------|-------|
| 0x0000 | 10   | ID string `_a2module_` |
| 0x000A | 4    | CRC32 |
| 0x000E | 1    | File format version |
| 0x000F | 1    | Number of patterns `#pat` |

#### Versions 1–4

After header: 5 × word (data block lengths 0–4), then 5 compressed data blocks.

**Data block 0 (songdata) after decompression:**

| Offset | Size | Content |
|--------|------|---------|
| 0x0000 | 43   | Song name (Pascal string, max 42 chars) |
| 0x002B | 43   | Composer (Pascal string, max 42 chars) |
| 0x0056 | 250×33 | 250 instrument names (32 chars each) |
| 0x2090 | 250×13 | 250 instrument specs (see below) |
| 0x2D42 | 128  | Pattern order table (entries 0x00–0x7F) |
| 0x2DC2 | 1    | Initial tempo |
| 0x2DC3 | 1    | Initial speed |

Instrument spec (13 bytes):

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 1    | AM/Vibrato/EG (modulator) |
| 0x01   | 1    | AM/Vibrato/EG (carrier) |
| 0x02   | 1    | KSL/Volume (modulator) |
| 0x03   | 1    | KSL/Volume (carrier) |
| 0x04   | 1    | Attack/Decay (modulator) |
| 0x05   | 1    | Attack/Decay (carrier) |
| 0x06   | 1    | Sustain/Release (modulator) |
| 0x07   | 1    | Sustain/Release (carrier) |
| 0x08   | 1    | Waveform (modulator) |
| 0x09   | 1    | Waveform (carrier) |
| 0x0A   | 1    | Feedback/FM |
| 0x0B   | 1    | Miscellaneous (unused) |
| 0x0C   | 1    | Fine-tune (signed) |

**Data blocks 1–4 (patterns) after decompression:**

Each block holds 16 patterns (`block_number - 1` × 16 + 0..15).
Each pattern: 64 lines × 9 channels × 4 bytes = 2304 bytes.

Pattern event (4 bytes):

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 1    | Note (0–96; 255 = Key-Off) |
| 0x01   | 1    | Instrument (0–250) |
| 0x02   | 1    | Effect command (0–15) |
| 0x03   | 1    | Effect data |

#### Versions 5–8

After header: 9 × word (block lengths 0–8), flags byte at `0x016`, then 9 compressed data blocks.

Data block 0 (songdata) — instrument spec adds **panning** field (byte 0x0B),
misc field removed. A `flags` byte is appended at the end.

**Flags byte:**

| Bit | Switch |
|-----|--------|
| 0   | Update speed |
| 1   | Track volume lock |
| 2   | Volume peak lock |
| 3   | Tremolo depth |
| 4   | Vibrato depth |
| 5   | Track panning lock |
| 6   | *unused* |
| 7   | *unused* |

**Data blocks 1–8 (patterns):** Each block holds 8 patterns.
Each pattern: 64 lines × 18 channels × 4 bytes = 4608 bytes.
Effect command range: 0–35.

#### Version 9

After header: 17 × dword (block lengths 0–16), then 17 compressed data blocks.

Data block 0 (songdata):

| Offset | Size | Content |
|--------|------|---------|
| 0x0000 | 43   | Song name (max 42 chars) |
| 0x002B | 43   | Composer (max 42 chars) |
| 0x0056 | 255×33 | 255 instrument names (32 chars each) |
| 0x2135 | 255×14 | 255 instrument specs (14 bytes each, adds voice type) |
| 0x2F27 | 255×3831 | 255 instrument macro-definitions |
| 0x0F1730 | 255×521 | 255 arpeggio/vibrato macro-definitions |
| 0x111E27 | 128  | Pattern order table |
| 0x111EA7 | 1    | Initial tempo |
| 0x111EA8 | 1    | Initial speed |
| 0x111EA9 | 1    | Flags |
| 0x111EAA | 2    | Pattern length |
| 0x111EAC | 1    | Number of tracks |
| 0x111EAD | 2    | Macro speed-up factor |

Instrument spec v9 (14 bytes): same as v5–8 but adds voice type (byte 0x0D):
- 0 = melodic
- 1–5 = percussion (BD, SD, TT, TC, HH)

**Flags byte (v9):**

| Bit | Switch |
|-----|--------|
| 0   | Update speed |
| 1   | Track volume lock |
| 2   | Volume peak lock |
| 3   | Tremolo depth |
| 4   | Vibrato depth |
| 5   | Track panning lock |
| 6   | Percussion track extension |
| 7   | Volume scaling |

**Instrument macro-definition (3831 bytes each):**

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 1    | Length |
| 0x01   | 1    | Loop begin position |
| 0x02   | 1    | Loop length |
| 0x03   | 1    | Key-off position |
| 0x04   | 1    | Arpeggio macro-table number |
| 0x05   | 1    | Vibrato macro-table number |
| 0x06   | 255×15 | FM-register definitions |

FM-register definition (15 bytes):

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 1    | AM/Vibrato/EG (modulator) |
| 0x01   | 1    | AM/Vibrato/EG (carrier) |
| 0x02   | 1    | KSL/Volume (modulator) |
| 0x03   | 1    | KSL/Volume (carrier) |
| 0x04   | 1    | Attack/Decay (modulator) |
| 0x05   | 1    | Attack/Decay (carrier) |
| 0x06   | 1    | Sustain/Release (modulator) |
| 0x07   | 1    | Sustain/Release (carrier) |
| 0x08   | 1    | Waveform (modulator) |
| 0x09   | 1    | Waveform (carrier) |
| 0x0A   | 1    | Feedback/FM (bit 7 = retrigger flag) |
| 0x0B   | 2    | Frequency slide (signed word) |
| 0x0D   | 1    | Panning (0=center, 1=left, 2=right) |
| 0x0E   | 1    | Duration |

**Arpeggio/Vibrato macro-definition (521 bytes each):**

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 1    | Length |
| 0x01   | 1    | Speed |
| 0x02   | 1    | Loop begin position |
| 0x03   | 1    | Loop length |
| 0x04   | 1    | Key-off position |
| 0x05   | 255  | Arpeggio table |
| 0x104  | 1    | Length |
| 0x105  | 1    | Speed |
| 0x106  | 1    | Delay |
| 0x107  | 1    | Loop begin position |
| 0x108  | 1    | Loop length |
| 0x109  | 1    | Key-off position |
| 0x10A  | 255  | Vibrato table |

Arpeggio values: 0 = default note, 1–96 = half-tones to add, 0x81–0xE1 = fixed-note.
Vibrato values: 0 = default, −0x7F..0x7F = frequency units to add (signed).

**Data blocks 1–16 (patterns):** Each holds 8 patterns.
Each pattern: 256 lines × 20 channels × 6 bytes = 30720 bytes.

Pattern event (6 bytes):

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 1    | Note (0–96; 0x90 = fixed; 255 = Key-Off) |
| 0x01   | 1    | Instrument (0–255) |
| 0x02   | 1    | 1st effect command (0–39) |
| 0x03   | 1    | 1st effect data |
| 0x04   | 1    | 2nd effect command (0–39) |
| 0x05   | 1    | 2nd effect data |

#### Version 10

Same layout as v9, but songdata adds at the end:
- 4-op track extension flags (1 byte)
- Initial lock flags for 20 tracks (20 bytes)

**4-op track extension flags:**

| Bit | Tracks |
|-----|--------|
| 0   | 1, 2   |
| 1   | 3, 4   |
| 2   | 5, 6   |
| 3   | 10, 11 |
| 4   | 12, 13 |
| 5   | 14, 15 |
| 6   | *unused* |
| 7   | *unused* |

**Initial lock flags (per track):**

| Bits | Field |
|------|-------|
| 0–1  | Panning position: 0=center, 1=left, 2=right |
| 2–3  | Volume slide type: 0=def, 1=car, 2=mod, 3=car w/mod |
| 4    | Volume lock state |
| 5    | Peak lock state |
| 6    | *unused* |
| 7    | *unused* |

Instrument names in v10 are 42 characters each (255 × 43 bytes).

#### Version 11

Same layout as v10, but songdata adds:
- 128 pattern names (42 characters each, 128 × 43 bytes)
- 255 disabled FM-register column flag tables (28 bytes each)

Disabled FM-register column flags (28 bytes):

| Offset | Field |
|--------|-------|
| 0x00   | Attack rate (modulator) |
| 0x01   | Decay rate (modulator) |
| 0x02   | Sustain level (modulator) |
| 0x03   | Release rate (modulator) |
| 0x04   | Waveform type (modulator) |
| 0x05   | Output level (modulator) |
| 0x06   | Key scaling level (modulator) |
| 0x07   | Multiplier (modulator) |
| 0x08   | Tremolo (modulator) |
| 0x09   | Vibrato (modulator) |
| 0x0A   | Key scale rate (modulator) |
| 0x0B   | Sustain (modulator) |
| 0x0C   | Attack rate (carrier) |
| 0x0D   | Decay rate (carrier) |
| 0x0E   | Sustain level (carrier) |
| 0x0F   | Release rate (carrier) |
| 0x10   | Waveform type (carrier) |
| 0x11   | Output level (carrier) |
| 0x12   | Key scaling level (carrier) |
| 0x13   | Multiplier (carrier) |
| 0x14   | Tremolo (carrier) |
| 0x15   | Vibrato (carrier) |
| 0x16   | Key scale rate (carrier) |
| 0x17   | Sustain (carrier) |
| 0x18   | Connection type |
| 0x19   | Feedback |
| 0x1A   | Frequency slide |
| 0x1B   | Panning |

---

## A2T — Tiny Module Format

ID string: `_a2tiny_module_` (15 bytes)

### Header

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 15   | ID string `_a2tiny_module_` |
| 0x0F   | 4    | CRC32 |
| 0x13   | 1    | File format version |
| 0x14   | 1    | Number of patterns `#pat` |
| 0x15   | 1    | Initial tempo |
| 0x16   | 1    | Initial speed |

#### Versions 1–4

6 × word (block lengths 0–5), then 6 compressed data blocks.

- Block 0: instruments (variable count × 13 bytes)
- Block 1: pattern order (128 bytes)
- Blocks 2–5: patterns (identical layout to A2M v1–4: 9 ch × 64 lines × 4 bytes)

#### Versions 5–8

Flags byte at `0x017`, then 10 × word (block lengths 0–9), then 10 compressed data blocks.

Instrument spec adds panning (13 bytes → same as A2M v5–8).

Patterns: 18 ch × 64 lines × 4 bytes, effect range 0–35.

#### Version 9

Flags at `0x017`, pattern length (word) at `0x018`, track count at `0x01A`,
macro speed-up (word) at `0x01B`, then 20 × dword (block lengths 0–19).

20 compressed data blocks:
- Block 0: instrument specs (variable × 14 bytes, adds voice type)
- Block 1: instrument macro-defs (255 × 3831 bytes)
- Block 2: arpeggio/vibrato macro-defs (255 × 521 bytes)
- Block 3: pattern order (128 bytes)
- Blocks 4–19: patterns (20 ch × 256 lines × 6 bytes)

Flags byte same as A2M v9.

#### Version 10

Same as A2T v9, but at `0x01D`: 4-op flags (1 byte) + 20 lock flags (20 bytes),
then 20 × dword (block lengths).

Instrument names are 42 characters.

#### Version 11

Same as A2T v10, but at `0x01E`: 4-op flags (1 byte) + 20 lock flags (20 bytes),
then 21 × dword (block lengths 0–20).

Block 3 adds disabled FM-register column flags (255 × 28 bytes).
Patterns shift to blocks 5–20.

---

## A2P — Pattern Format

ID string: `_a2pattern_` (11 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 11   | ID string `_a2pattern_` |
| 0x0B   | 4    | CRC32 |
| 0x0F   | 1    | File format version |

#### Versions 1–4

Word length + data block. Pattern: 9 ch × 64 lines × 4 bytes.

#### Versions 5–8

Word length + data block. Pattern: 18 ch × 64 lines × 4 bytes.

#### Version 9

Dword length + data block. Pattern: 20 ch × 256 lines × 6 bytes (two effect columns).

#### Version 10

Dword length + data block. Same as v9 pattern layout, but appends a 30-character pattern name.

---

## A2i — Instrument Format

ID string: `_a2ins_` (7 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 7    | ID string `_a2ins_` |
| 0x07   | 2    | CRC16 |
| 0x09   | 1    | File format version |

#### Versions 1–4

Byte length + data block (13-byte instrument + 23-byte name, no panning).

#### Versions 5–8

Byte length + data block (13-byte instrument with panning + 23-byte name).

#### Version 9

Word length + data block (14-byte instrument with voice type + 33-byte name).

---

## A2B — Instrument Bank Format

ID string: `_a2insbank_` (11 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 11   | ID string `_a2insbank_` |
| 0x0B   | 4    | CRC32 |
| 0x0F   | 1    | File format version |

#### Versions 1–4

Word length + data block. 250 names (33 bytes each) + 250 instruments (13 bytes each, no panning).

#### Versions 5–8

Word length + data block. 250 names + 250 instruments (13 bytes with panning).

#### Version 9

Dword length + data block. 255 names (33 bytes each) + 255 instruments (14 bytes each, with voice type).

---

## A2W — Instrument Bank with Macros Format

ID string: `_a2insbank_w/macros_` (20 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 20   | ID string `_a2insbank_w/macros_` |
| 0x14   | 4    | CRC32 |
| 0x18   | 1    | File format version |

#### Version 1

Two dword lengths + 2 data blocks:
- Block 0: instrument specs/macro-defs
- Block 1: arpeggio/vibrato macro-defs

Structure of block 0: 255 names (33 bytes) + 255 instruments (14 bytes with voice type)
+ 255 macro-definitions (3831 bytes each — same as A2M v9+).

Structure of block 1: 255 arpeggio/vibrato macro-defs (521 bytes each — same as A2M v9+).

#### Version 2

Three dword lengths + 3 data blocks:
- Block 0: instrument specs/macro-defs
- Block 1: arpeggio/vibrato macro-defs
- Block 2: disabled FM-register columns (255 × 28 bytes)

---

## A2F — Instrument with FM-Register Macro Format

ID string: `_a2ins_w/fm-macro_` (18 bytes)

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 18   | ID string `_a2ins_w/fm-macro_` |
| 0x12   | 4    | CRC32 |
| 0x16   | 1    | File format version (currently 1) |
| 0x17   | 2    | Data block length |
| 0x19   | %len | Data block |

Data block after decompression:

| Offset | Size | Content |
|--------|------|---------|
| 0x00   | 14   | Instrument spec (with voice type) |
| 0x0E   | 33   | Instrument name (32 chars) |
| 0x2F   | 3831 | FM-register definition macro-table |
| 0xF26  | 28   | Disabled FM-register column flags |

---

## Effect Command System

Effect commands consist of two bytes: `effect_def` (fixed) and `effect` (info byte).
The player uses 86 effect commands and 16 extended commands.

See `A2M_effects.md` for the complete effect reference including hex codes,
character mappings, and parameter bit layouts.

---

## Note Types

| Value | Meaning |
|-------|---------|
| 0     | No note / empty |
| 1–96  | Note (1=C-0 .. 96=B-7) |
| 0x90  | Fixed note flag (v9+ patterns) |
| 0xFF  | Key-Off |

---

## Supported Foreign Formats (AdT2 Tracker)

The original Adlib Tracker 2 could import from and convert to:

**Song formats:**
- A2M (AdT2), A2P (AdT2 pattern), A2T (AdT2 tiny module)
- AMD (Amusic), CFF (BoomTracker 4.0), DFM (Digital-FM)
- FMK (FM-Kingtracker), HSC (HSC AdLib Composer / HSC-Tracker)
- MTK (MPU-401 Tracker), RAD (Reality AdLib Tracker v1)
- S3M (Scream Tracker 3.x), SAT (Surprise! AdLib Tracker v1,5,6)
- SA2 (Surprise! AdLib Tracker 2.0 v8,9), XMS (XMS-Tracker)

**Instrument formats:**
- A2i (AdT2), A2F (AdT2 w/fm-register macro)
- CiF (BoomTracker 4.0), FiN (FM-Kingtracker)
- iNS (HSC-Tracker/RAD-Tracker/SAdT/Amusic)
- SBi (Creative Labs FM instrument), SGi (Sound Generator 3.0)

**Bank formats:**
- A2B (AdT2), A2W (AdT2 w/macros)
- BNK (AdLib instrument bank v1.0)
- FiB (FM-Kingtracker), iBK (Creative Labs FM instrument bank)
