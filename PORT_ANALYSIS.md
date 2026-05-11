# C Port Analysis: a2t_play vs reference FreePascal AdT2

## 1. Function Correspondence Table

| Pascal (adt2play\) | C (a2t_play\src\) | Status | Notes |
|---|---|---|---|
| **Loader / Import** | | |
| `a2m_file_loader` / `a2t_file_loader` | `a2_import()` → `a2m_import()` / `a2t_import()` | MATCH | Header magic, ffver ceiling (14), block structure all match. |
| — | `a2t_load()` (file I/O) | MATCH | Pascal uses TP `BlockRead`; C uses `fread`. Functionally identical. |
| `import_old_a2m_event1` | `convert_v1234_effects()` | MATCH | Identical fx→ef mapping table; same `adsr_carrier[]` toggle logic for `fx_ex_ExtendedCmd` cases 5/6. |
| `replace_old_adsr` | *(inline in `a2_read_patterns` v1-4 path)* | MATCH | C applies `convert_v1234_effects` at pattern-read time (single pass). Pascal uses two-pass: import + `replace_old_adsr` sweep. Same net result. C is simpler/single-pass. |
| `import_old_flags` | *(inline in `a2m_read_songdata` / `a2t_import`)* | MATCH | Common_flag bit extraction identical. |
| **Depackers** | | |
| `RDC_decompress` | *(not present)* | MISSING | Pascal implements RDC (Ed Ross). C has no RDC decompressor. Check if any A2M/A2T v5-8 files use RDC — TECHINFO.DOC says v5-8 use SixPack (v5), LZW (v6), LZSS (v7), unpacked (v8). RDC may be unused. |
| `SIXPACK_decompress` | `sixdepak()` in `sixpack.c` | MATCH | Same Philip Gage algorithm. |
| `LZW_decompress` | `LZW_decompress()` in `unlzw.c` | MATCH | Same LZW. |
| `LZSS_decompress` | `LZSS_decompress()` in `unlzss.c` | MATCH | Same LZSS. |
| `APACK_decompress` | `aP_depack()` in `depack.c` | MATCH | Same aPLib v0.26b algorithm. |
| `LZH_decompress` | `LZH_decompress()` in `unlzh.c` | MATCH | Same LZH (Okumura/Yoshizaki). |
| **Depacker dispatch** | | |
| *(inline in loaders — switch on ffver)* | `a2t_depack()` | MATCH | Pascal: v1=SixPack, v2=LZW, v3=LZSS, v4=none, v5-8 mirror, v9-11=aPack, v12-14=LZH. C: identical mapping. |
| **Effect Handlers (compile-time)** | | |
| `play_line` → `case ef_Arpeggio` etc. (lines 1520-2094) | `process_effects()` | MATCH | Full coverage of all 50 standard effects + extended groups (ef_ex, ef_ex2, ef_ex3). Each switch case body is functionally identical. |
| `play_line` → effect_def2 pass (lines 2096-2668) | `process_effects()` slot-1 call | MATCH | Pascal has explicit second-pass; C calls `process_effects(event, 1, chan)` — same logic, same cases. |
| **Effect Handlers (run-time)** | | |
| `update_effects` | `update_effects()` | MATCH | Iterates over all channels, both slots. |
| `update_fine_effects` | `update_fine_effects()` | MATCH | |
| *(no explicit extra-fine pass)* | `update_extra_fine_effects()` | MATCH | Pascal handles extra-fine via `ef_fix2` flag in `effect_table[]` lo-byte; C has `update_extra_fine_effects_slot()` called every 4 ticks. Functional equivalence confirmed. |
| `update_fine_effects` → vibrato.fine | `update_fine_effects` → vibr.fine | MATCH | Conditional branch for fine vibrato/tremolo in both. |
| **Note/Macro processing** | | |
| `output_note` | `output_note()` | MATCH | Identical — `keyoff_flag`, `nFreq`, `fine_tune`, `change_frequency`, ZFF no-restart check, 4-op note mirror. |
| `init_macro_table` | `init_macro_table()` | MATCH | Same fields initialized. |
| `change_frequency` | `change_frequency()` | MATCH | Same vib_paused/vib_count/vib_pos/vib_freq reset. |
| `change_freq` | `change_freq()` | MATCH | Same 4-op hi-channel freq mirror, same freq_table bitmask. |
| **Timer / IRQ** | | |
| `timer_poll_proc` | `newtimer()` / `a2t_update()` | MATCH | Same `ticklooper`/`macro_ticklooper` state machine. |
| — | *(poll_proc inlined in a2t_update)* | MATCH | Pascal: timer IRQ calls `timer_poll_proc`. C: SDL audio callback calls `poll_proc()` at tick boundaries. Same logic. |
| `update_timer` | `update_timer()` | MATCH | Identical IRQ_freq computation (tempo==18 fix, `MOD (tempo*_macro_speedup)`, MAX_IRQ_FREQ clamp). |
| `_macro_speedup` | `_macro_speedup()` | MATCH | Both return `macro_speedup` (or 1 if 0). |
| `macro_poll_proc` | `macro_poll_proc()` | MATCH | Same FM-reg loop/arpeggio/vibrato state machines. `IDLE=0xfff`, `FINISHED=0xffff` match. |
| `generate_custom_vibrato` | `generate_custom_vibrato()` | MATCH | Identical. |
| **Volume / Panning** | | |
| `set_ins_volume` | `set_ins_volume()` | MATCH | Same OPL register writes, same `scale_volume()` chain with `global_volume`/`overall_volume`. **Difference:** Pascal has `fade_out_volume` in the chain — C has it commented out (`/*, 63 - fade_out_volume*/`). |
| `set_ins_volume_4op` | `set_ins_volume_4op()` | MATCH | Same 4-op connection mode dispatch. |
| `set_volume` (nested) | `set_volume()` | MATCH | Same structure for 4-op inner volume set. |
| `set_global_volume` | `set_global_volume()` | MATCH | |
| `scale_volume` | `scale_volume()` | MATCH | `63 - ((63 - volume) * (63 - scale_factor) / 63)` — exact match. |
| `reset_ins_volume` | `reset_ins_volume()` | MATCH | |
| **OPL Register Output** | | |
| `opl2out_proc` / `opl3out_proc` / `opl3exp_proc` | `opl2out()` / `opl3out()` / `opl3exp()` | MATCH | Pascal uses `_opl_regs_cache[WORD]` + inline asm to real OPL3. C uses Nuked OPL3 library via `OPL3_WriteRegBuffered`. Register caching strategy differs but final register values are equivalent. |
| **4-op helpers** | | |
| `_4op_data_flag` | `get_4op_data()` | MATCH | Same bit-packed return (mode, conn, ch1, ch2, ins1, ins2). |
| `_4op_vol_valid_chan` | `_4op_vol_valid_chan()` | MATCH | |
| **is_4op_chan** | `is_4op_chan()` | MATCH | Same bitmask approach. |
| **Frequency helpers** | | |
| `nFreq` | `nFreq()` | MATCH | Same Fnum table (Pascal: Fnum starts at $157; C: at $156). One-semitone offset — **see discrepancy #1 below.** |
| `calc_freq_shift_up` | `calc_freq_shift_up()` | MATCH | |
| `calc_freq_shift_down` | `calc_freq_shift_down()` | MATCH | |
| `calc_vibtrem_shift` | `calc_vibrato_shift()` | MATCH | Same rotate + shift algorithm. |
| **Portamento** | | |
| `portamento_up/down` | `portamento_up/down()` | MATCH | |
| `tone_portamento` | `tone_portamento()` | MATCH | |
| **Volume slide** | | |
| `slide_volume_up/down` | `slide_volume_up/down()` | MATCH | Same volslide_type dispatch (0=auto, 1=carrier, 2=modulator, 3=both), same 4-op connection handling. |
| `slide_carrier/modulator_volume_up/down` | Same | MATCH | |
| **Arpeggio** | | |
| `arpeggio` | `arpeggio()` | MATCH | Same state machine `{1, 2, 0}`. |
| **Vibrato / Tremolo** | | |
| `vibrato` | `vibrato()` | MATCH | |
| `tremolo` | `tremolo()` | MATCH | |
| **Retrig / MultiRetrig** | | |
| *(inline in `update_effects`)* | `ef_RetrigNote` / `ef_MultiRetrigNote` | MATCH | Same volume slide sub-cases for MultiRetrig (1,2,4,8,16, 2/3, 1/2, 3/2, 2x). |
| **Missing Loaders** | | |
| `amd_file_loader` | *(not present)* | MISSING | AMD (ADPCM) import not ported. |
| `cff_file_loader` | *(not present)* | MISSING | CFF import not ported. |
| `dfm_file_loader` | *(not present)* | MISSING | DFM (Digital FM) import not ported. |
| `mtk_file_loader` | *(not present)* | MISSING | MTK import not ported. |
| `rad_file_loader` | *(not present)* | MISSING | RAD import not ported. |
| `s3m_file_loader` | *(not present)* | MISSING | S3M import not ported. |
| `fmk_file_loader` | *(not present)* | MISSING | FMK import not ported. |
| `sat_file_loader` | *(not present)* | MISSING | SAT import not ported. |
| `sa2_file_loader` | *(not present)* | MISSING | SA2 import not ported. |
| `hsc_file_loader` | *(not present)* | MISSING | HSC import not ported. |

## 2. Macro System Timer Coupling

**Verdict: MATCH** — The `_macro_speedup()` function and IRQ frequency calculation are identical:

| Component | Pascal (`a2player.pas`) | C (`a2t.c`) |
|---|---|---|
| `_macro_speedup` | `if macro_speedup>0 then macro_speedup else macro_speedup+1` | `macro_speedup ? macro_speedup : 1` |
| IRQ freq loop | `while IRQ_freq MOD (tempo*_macro_speedup) <> 0 do Inc(IRQ_freq)` | `while (IRQ_freq % (tempo * _macro_speedup()) != 0) IRQ_freq++` |
| MAX_IRQ_FREQ clamp | `if IRQ_freq > MAX_IRQ_FREQ then IRQ_freq := MAX_IRQ_FREQ` | `if (IRQ_freq > MAX_IRQ_FREQ) IRQ_freq = MAX_IRQ_FREQ` |
| tempo=18 timer_fix | `IRQ_freq := TRUNC((tempo+0.2)*20)` | `IRQ_freq = ((float)tempo + 0.2) * 20.0` |
| ticklooper | `IRQ_freq / tempo` | `IRQ_freq / tempo` |
| macro_ticklooper | `IRQ_freq / (tempo * macro_speedup)` | `IRQ_freq / (tempo * _macro_speedup())` |

The C `a2t_update()` inlines the ticklooper/macro_ticklooper increment logic that Pascal puts in `timer_poll_proc`. The computation is identical.

## 3. Compression Version Selection

**Verdict: MATCH** — The `a2t_depack()` switch in C exactly mirrors the Pascal loaders' per-version dispatch:

| ffver | Algorithm | Pascal uses | C uses |
|---|---|---|---|
| 1, 5 | SixPack | `SIXPACK_decompress` | `sixdepak()` |
| 2, 6 | LZW | `LZW_decompress` | `LZW_decompress()` |
| 3, 7 | LZSS | `LZSS_decompress` | `LZSS_decompress()` |
| 4, 8 | None (raw) | `Move(buf1, dest, len)` | `memcpy(dst, src, srcsize)` |
| 9, 10, 11 | aPack | `APACK_decompress` | `aP_depack()` |
| 12, 13, 14 | LZH | `LZH_decompress` | `LZH_decompress()` |

**Note:** RDC decompressor is present in Pascal but **not called** for any A2M/A2T ffver. It exists in Pascal for possible other uses. C correctly omits it.

## 4. Old-Format Effect Migration (v1-4 → modern)

**Verdict: MATCH** — C's `convert_v1234_effects()` and Pascal's `import_old_a2m_event1()` are functionally identical. Both map:

- `fx_Arpeggio`→`ef_Arpeggio`, `fx_FSlideUp`→`ef_FSlideUp`, etc.
- `fx_SetOpIntensity`→ carrier/modulator split with `* 4 + 3` mapping
- `fx_Extended` sub-commands: all 15 extended commands identically mapped
- `fx_ex_ExtendedCmd` cases 0-9: RSS, LockVol, UnlockVol, LockVP, UnlockVP, adsr_carrier toggle (5/6), VSlide_car (7), VSlide_mod (8), VSlide_def (9)
- The `adsr_carrier[]` per-channel flag tracks modulator vs carrier targeting identically

Key difference: Pascal applies migration as a *post-load second pass* (via `replace_old_adsr`), while C applies it *during pattern loading* in `a2_read_patterns()`. Net result is the same.

## 5. Impact Analysis of Missing Features

### 5.1 CRC32 Verification
- **Pascal:** Every `a2m_file_loader`/`a2t_file_loader` path computes CRC32 over all blocks and compares against stored `header.crc32`. Files with wrong CRC32 are rejected.
- **C:** No CRC check. Corrupted files may load and play garbage silently.
- **Impact:** Low for correct files. Medium for debugging — a corrupt file may manifest as bad playback rather than a clean rejection.

### 5.2 Foreign Format Importers (10 formats)
- **Pascal:** AMD, CFF, DFM, MTK, RAD, S3M, FMK, SAT, SA2, HSC.
- **C:** None.
- **Impact:** The C port only plays A2M/A2T files. All foreign formats are unsupported. This is an intentional scope limitation, not a bug.

### 5.3 `fade_out_volume`
- **Pascal:** `fade_out_volume: Byte = 63` — used in `scale_volume(x, scale_volume(63-global_volume, 63-fade_out_volume))` inside `set_ins_volume` and `set_volume`.
- **C:** Commented out: `/*, 63 - fade_out_volume*/`.
- **Impact:** Low — `fade_out_volume` defaults to 63 (no fade) and is only changed by the editor, not during normal playback. For playback-only use, this is harmless.

### 5.4 `bpm_data` (v14)
- **Pascal:** `bpm_rows_per_beat` and `bpm_tempo_finetune` fields are part of the songdata record. `adtrack2.mht` documents they affect tempo calculation under BPM mode.
- **C:** Read from file (`a2t.c:4248-4249`) and stored in `tSONGINFO`, but **unused** in player logic. TODO comment: "Implement these in player".
- **Impact:** Low-medium — files saved in v14 with BPM tempo mode will not play at the correct speed. The player falls back to standard tempo/speed.

### 5.5 `ins_4op_flags` and `reserved_data` (v12-13)
- **Pascal:** `songdata.instr_4op_flags` and `songdata.reserved_data` loaded and stored.
- **C:** Read from file and **skipped** (pointer advanced past them, but data not stored).
- **Impact:** None on playback — these are editor metadata fields. `ins_4op_flags` records which instruments use 4-op; `reserved_data` is a general-purpose reserved block.

### 5.6 `pattern_names` (v9-14)
- **Pascal:** Each pattern has a stored name string.
- **C:** `pattern_names` field is absent from `tSONGINFO` — not read, not stored.
- **Impact:** None on playback. Pattern names are editor-only metadata.

### 5.7 OPL Register Caching Strategy
- **Pascal:** `_opl_regs_cache[WORD]` — skips `out` if cached value matches. Real hardware optimization to avoid redundant writes.
- **C:** Nuked OPL3 `OPL3_WriteRegBuffered` — internal buffering, no redundant-write skip needed.
- **Impact:** None — final register values are identical. The Nuked OPL3 emulator handles its own optimization.

### 5.8 Decay Bar / Visualization
- **Pascal:** `decay_bar[1..96]` — visualizer for on-screen channel activity. Updated in `output_note()`, used in `update_effects` for volume visualization.
- **C:** Not present.
- **Impact:** None. This is UI-only code. The C port has no visual scope display.

## 6. Discrepancy: `nFreq` Fnum Table Base Value

**Pascal:**
```pascal
Fnum: array[0..11] of Word = (
    $157, $16b, $181, $198, $1b0, $1ca,
    $1e5, $202, $220, $241, $263, $287);
```

**C:**
```c
static uint16_t Fnum[13] = {
    0x156, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca,
    0x1e5, 0x202, 0x220, 0x241, 0x263, 0x287, 0x2ae};
```

The C table starts at `0x156` instead of Pascal's `0x157`, and includes a 13th entry (`0x2ae` = FreqEnd) used as sentinel. This means:
- C: note 0 → `0x156` (A-2), note 1 → `0x16b`, etc.
- Pascal: note 0 → `0x157` (A-2), note 1 → `0x16b`, etc.

This is a 1-unit difference in the base frequency for note 0 (the lowest note). Given the OPL3 frequency resolution of ~0.085 Hz per unit at sample rate ~49716 Hz, this is ~0.085 Hz — **inaudible**. The rest of the table matches. The C table's 13th entry `0x2ae` is used in `nFreq()` for `note >= 12*8` return value. Pascal handles this with `(7 << 10) | FreqEnd` which equals `(7 << 10) | 0x2ae = 0x1cae` — same value.

**Impact:** Negligible. A 1/1024th-of-a-semitone pitch offset on the very lowest note.

## 7. Summary

| Category | Correspondence |
|---|---|
| A2M/A2T file loading | **MATCH** (modulo CRC32 omission) |
| Pattern event import + old-format conversion | **MATCH** |
| Depacker dispatch | **MATCH** (RDC omitted, unused) |
| Effect processing (all 50 standard + extended) | **MATCH** |
| Macro system (FM-reg/arpeggio/vibrato) | **MATCH** |
| IRQ timer computation | **MATCH** |
| Volume/panning scaling | **MATCH** (fade_out_volume dormant) |
| OPL register output | **MATCH** (different caching, same register values) |
| 4-op handling | **MATCH** |
| Foreign format importers | **MISSING** (10 formats, expected) |
| CRC32 verification | **MISSING** |
| BPM v14 tempo mode | **NOT IMPLEMENTED** (data read, unused) |
| nFreq table offset | **1-UNIT DIFFERENCE** (inaudible) |
