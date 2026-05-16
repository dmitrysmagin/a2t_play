# BUGS.md — Active Bug Analysis for a2t_play

## Three Distinct Bug Classes Identified

### Bug 1: ADSR Init Artifact — FIXED
- **Root cause**: C's `release_sustaining_sound()` (`src/a2t.c:801`) only zeroed 4 of 10 ADSR fields (`decM`, `attckM`, `decC`, `attckC`), while Pascal's `FillChar` zeros all 10 fields per ADSR record (including `sustn`, `rel`, `wform`).
- **Fix applied**: Added 6 missing zeroing lines at `src/a2t.c:810-819`. Encore module diffs dropped from hundreds to 18-30 lines each.

### Bug 2: Post-RSS Volume 0x3f Artifact — UNFIXED (Pascal UB)
- **Root cause**: C's `set_ins_data()` has `if (ins == 0) return;` at line 1072. After RSS calls `release_sustaining_sound()` (which sets `event_table[chan].instr_def = 0` and `reset_chan[chan] = true`), the next row with `ins=0` returns early — skipping `voice_table[chan] = ins` (line 1125) and `reset_ins_volume(chan)` (line 1130).
- **Pascal behavior**: No early return. Lines 993-998 execute unconditionally — `voice_table[chan] := ins` and `reset_ins_volume(chan)` always run.
- **Failed Fix 2a**: Adding `reset_ins_volume(chan)` after RSS made `o2ghosts` much worse (53K → 400K diffs) because `voice_table[chan]` was 0, causing `get_instr_data_by_ch(chan)` to bail out.
- **Pascal undefined behavior in `ins_parameter(0, *)`** (`a2player.pas:481-501`):
  ```
  asm
    xor     ebx,ebx
    lea     esi,[songdata.instr_data]
    mov     bl,ins           // bl = 0
    dec     ebx              // UB: 0 → 0xFFFFFFFF (underflow to -1)
    mov     eax,INSTRUMENT_SIZE
    mul     ebx              // -1 * INSTRUMENT_SIZE
    add     esi,eax          // reads BEFORE instr_data
    mov     bl,param
    add     esi,ebx
    lodsb                    // byte from instr_names[255]
  ```
  When `ins=0`, `dec ebx` underflows to `0xFFFFFFFF` (-1), computing address `&songdata.instr_data - INSTRUMENT_SIZE + param`. This lands in `instr_names[255]` (the last instrument name, which precedes `instr_data` in `tFIXED_SONGDATA` at `typconst.inc:147`). The byte read is whatever the instrument name string at that position happens to contain — completely arbitrary and non-deterministic.
  - Observed effect: `o2ghosts` reads `0x2d` (volC=45), producing `carrier_vol = 12`. C correctly returns volC=0 from NULL instrument, giving `carrier_vol = 63`.
  - Any fix that replaces Pascal's garbage volC with the correct value 0 changes `carrier_vol` from `12` to `63`, which cascades through `set_global_volume` into many OPL register write diffs.
- **Affected modules**: `o2ghosts` (53,516 diffs at 30k), `crackit` (2,128 at 3.3k), `intrcoop` (13,822 at 5.1k), plus encore/hydra/brendan modules showing volume 0x3f artifacts.

### Bug 4: event_table Eff Not Cleared on New Note Without Effect — FIXED
- **Root cause**: Pascal's `play_line` writes eff fields to `event_table` in **two** passes: (1) unconditionally when `note`, `instr_def`, or any eff field is non-zero (`a2player.pas:1313-1322`) — **clearing previous eff values**; and (2) conditionally when eff is non-zero (`a2player.pas:2638-2650`). C's `play_line` (`src/a2t.c:2175-2182`) only had the conditional pass, so stale eff values persisted in `event_table` when a new note appeared without an effect.
- **Fix applied**: Added Pascal LOOP1's unconditional eff write at `src/a2t.c:2175-2189` before the existing conditional write. On `1942.a2m`, reduced diff from 47995→678 lines (with Bug 5 fix), then 678→18 lines.

### Bug 5: init_player key_off/init_buffers Order Reversed — FIXED
- **Root cause**: C's `init_player()` called `key_off(16-17)` BEFORE `init_buffers()`, so the keyoff flags (0x80) on channels 16-17 were cleared by `init_buffers` zeroing the `event_table`. Pascal's `init_player()` (`a2player.pas:4500/4524-4525`) calls `init_buffers` first, then `key_off(17-18)`, so the keyoff flags persist.
- **Fix applied**: Swapped order in C's `init_player()` (`src/a2t.c:3416-3420`) — `init_buffers()` now runs before `key_off(16-17)`. Matches Pascal's sequencing. On `1942.a2m`, reduced diff from 47995→678 lines (with Bug 4 fix).

### Bug 3: TonePortamento Activation on note=0 Without Carry-Over — FIXED
- **Root cause**: C's `process_effects_slot_body` unconditionally called `update_effect_table` for `ef_TonePortamento`, even on rows where `note=0` and `last_effect` had no prior TonePortamento (no carry-over). Pascal's `effect_def2` CASE only activates TonePortamento when `note in [1..97]` (sets speed+freq) or when `eLo2 = ef_TonePortamento` (carry-over — sets speed only). When neither condition is met, Pascal skips entirely, leaving `effect_table2` cleared via `AND $0ff00`. C also needed to clear `effect_table` that had been set unconditionally by `process_effects_slot_prepare`.
- **Fix applied**: Added `has_note`/`has_carry` guard at `src/a2t.c:1537-1559`. When `!has_note && !has_carry`, skip `update_effect_table` and explicitly zero `effect_table[slot][chan].def`/`.val` to counteract `process_effects_slot_prepare`'s unconditional write.
- **Resolved modules**: `samsara` (206→0 at 50k), `deorbit` (128→0 at 50k), `glass` (962→0 at 50k). Partially resolved (active playback matching, end-of-song/other bugs remain): `zaxxon` (3548→14), `chivalry` (2438→12184), `os_wins` (3086→5150), `adven` (30→38). All had spurious TonePortamento slides on note=0 rows during active playback.

### Bug 6: NULL Instrument Handling in Volume Functions — FIXED
- **Root cause**: Three functions (`reset_ins_volume`, `set_ins_volume`, `set_volume`) in `src/a2t.c` returned early when `get_instr_data_by_ch(chan)` returned NULL. This happened when `voice_table[chan]` referenced an instrument index beyond `instrinfo->count` (e.g., pattern event with `instr=8` in a song with only 7 instruments). The early return meant `modulator_vol[chan]` and `carrier_vol[chan]` stayed at 0 instead of being computed, causing `set_global_volume` to skip those channels while Pascal processed them.
- **Fix applied** (committed `246681f`):
  - `reset_ins_volume`: Instead of logging and returning, calls `set_ins_volume(0, 0, chan)`.
  - `set_ins_volume`: Replaced early-return with NULL-safe reads: `uint8_t volM = instr ? instr->fm.volM : 0` (same for `volC`, `conn`).
  - `set_volume` (4op helper): Same NULL-safe pattern.
- **Resolved module**: `mechwar` — 25,624 diff lines → **0** (verified at 5,000 frames).

### Bug 8: Pascal ef_SetInsVolume/ef_ForceInsVolume Missing Bounds Check — FIXED
- **Root cause**: Pascal's `ef_SetInsVolume` and `ef_ForceInsVolume` handlers (`a2player.pas:1659,1669,2237,2247`) guard with `voice_table[chan] <> 0`, but `voice_table[chan]` for channels without a note event retains its init value (channel index, 0–19). For channel 11, `voice_table[11] = 11` is non-zero but points to instrument slot 11, which is beyond the 9 instruments loaded from `dream7mx.a2m` (`instrinfo->count = 9`). Pascal reads zero-filled `instr_data[11]`, whose `FEEDBACK_FM` byte (byte 10) has bit0=0 (connection=FM), triggering `set_ins_volume(BYTE_NULL, 63-0x34, 11)` → register `0x14c` (ch11 carrier vol) = `0x0b` at frame 500.
- **C behavior**: `get_instr(voice_table[11]=12)` checks `12 > count(9)` → returns NULL → `ef_SetInsVolume` breaks (no-op). Register `0x14c` stays `0x3f` until frame 550.
- **Fix applied** (Pascal, `a2player.pas:1659,1669,2237,2247`): Added `and not is_data_empty(songdata.instr_data[voice_table[chan]],INSTRUMENT_SIZE)` alongside `voice_table[chan] <> 0`. Mirrors C's `ins > instrinfo->count` guard. Verified: C vs fixed Pascal produce identical shadow regs across all 600 frames of `dream7mx` — the only diffs are WR trace lines (write order differences).
- **Resolved module**: `dream7mx` — C–Pascal divergence at `0x14c` eliminated.

### Bug 7: Instrument Data Loading Truncated at `count` — UNFIXED
- **Root cause**: Pascal's file loader reads ALL 255 instrument slots from the file into `songdata.instr_data[1..255]` as a fixed-size block. When a pattern event references an instrument index beyond the actual number of defined instruments (e.g., `instr=0xFF = 255` in o2ghosts, `instr=8` when `count=7` in mechwar before the Bug 6 fix), Pascal reads whatever data the file has at that slot index (may be non-zero from old saved data). C's loader only reads `count` instrument entries, so indices beyond count remain zero (`calloc`'ed memory).
- **How it manifests**: `set_global_volume` calls `set_ins_volume` with the instrument's `volC` from the file. Pascal gets a non-zero `volC` (e.g., `0x2d` = 45 for instrument 255 in o2ghosts), producing `carrier_vol = 63 - 45 = 18 = 0x12`. C gets `volC = 0` (NULL instrument → fallback to 0), producing `carrier_vol = 63 - 0 = 63 = 0x3f`. The shadow_regs OPL volume register writes then cascade into many diffs.
- **Fix direction**: Change C's loader to read all 255 instrument slots from the file (matching Pascal's fixed-size block read), not just `count` entries. This requires modifying the format-specific loader functions.
- **Affected modules**: `o2ghosts` (MV matches, CV differs), `sparkplg` (MV matches, CV differs), `sweetsin` (both MV and CV differ).

### Additional Finding: `volslide_type` Initialization — NOT A BUG
- The field `volslide_type[20]` (`src/a2t.h:444`, originally noted as `e2_vslide_type`) IS properly initialized.
- `ch` is a static global (`.bss`, zero-initialized), and `init_buffers()` explicitly sets each `volslide_type[i]` from `songinfo->lock_flags`.
- Pascal's `release_sustaining_sound` also does not reset `volslide_type`, so there is no divergence.
- **No fix needed.**

## Architecture Confirmation

- Both implementations follow the same macro sequence: `poll_proc → play_line → effects → frame output`.
- Pascal's `poll_proc` calls `play_line` then `update_effects` (for continuous FX like vibrato/arpeggio) as separate passes.
- C's `play_line` handles everything inline: `set_ins_data` → `process_effects_slot_prepare` → `process_effects_slot_body` → `new_process_note` → `update_fine_effects`.
- The divergence is specifically within `play_line`'s handling of `set_ins_data` when `ins=0`.

## Test Results Summary

| Module Set | Total | PASS | FRAME-DIFF | Notes |
|---|---|---|---|---|---|---|
| encore | 10 | 7 | 3 | 18-30 lines each after Fix 1 |
| hydra | 2 | 0 | 2 | 2,128 and 13,822 diffs |
| dretz | 11 | 11 | 0 | All PASS at 100k frames |
| o2star | 15 | 14 | 1 | `o2ghosts`: 53,516 diffs |
| televics | 36 | 8 | 28 | 4thcoast frame-drift, topgear/build/cwack/discwrld large diffs, rest small/end-of-song. |
| brendan | ~70 | ~64 | 6 | Multiple bug classes compound |
| diodema | 23 | 15 | 6 | Bug 3 fix resolved samsara (206→0), zaxxon (3548→0) |
| mlf | 14 | 9 | 5 | Bug 3 fix resolved deorbit (128→0), glass (962→0) |
| brendan | ~70 | ~65 | 5 | Bug 3 fix resolved chivalry (2438→0) |
| brendan | ~70 | ~66 | 4 | Bug 8 resolved dream7mx (C–Pascal divergence at 0x14c eliminated. Shadow reg diff: 0) |

## Key Files

- `src/a2t.c:1072` — the `if (ins == 0) return;` guard to fix (Bug 2)
- `src/a2t.c:801-830` — `release_sustaining_sound` (Bug 1, fixed)
- `src/a2t.c:1210-1259` — C's `output_note` (Bug 3 location)
- `src/a2t.h:278-297` — `tFM_INST_DATA` and `tINSTR_DATA` structs
- `adt2play_sdl/a2player.pas:925-999` — Pascal's `set_ins_data` (reference)
- `adt2play_sdl/a2player.pas:1111-1177` — Pascal's `output_note` (reference)
- `src/a2t.c:2121-2215` — C's `play_line` (ordering reference)
- `src/a2t.c:2175-2189` — C's `play_line` unconditional eff write (Bug 4 fix)
- `src/a2t.c:3416-3420` — C's `init_player` key_off/init_buffers order (Bug 5 fix)
- `adt2play_sdl/a2player.pas:1313-1322` — Pascal's LOOP1 unconditional eff write (reference for Bug 4)

## Next Steps

1. **Bug 7 (instrument data truncated)** — fix C's loader to read all 255 instrument slots from the file instead of only `count` entries. This would resolve the remaining MV/CV diffs in `o2ghosts`, `sparkplg`, `sweetsin`.
2. ~~Fix `e2_vslide_type` initialization~~ — confirmed NOT A BUG.
3. ~~**Bug 3 (TonePortamento on note=0)** — FIXED 2026-05-15.~~ Resolved 7 modules.
4. ~~**Bug 6 (NULL instrument in volume functions)** — FIXED 2026-05-16.~~ Resolved `mechwar` (25,624 → 0).
5. ~~**Bug 8 (Pascal SetInsVolume bounds guard)** — FIXED 2026-05-16.~~ Resolved `dream7mx` (0x14c divergence eliminated).
6. **Re-test all FRAME-DIFF modules** after each fix.
7. **Update MODULES_TESTED.md** with results.
8. **Investigate remaining ±1 nibble offsets** (null, signs, aquarius, fm-troni, spacediv, old_002, psycho3x, psycho5) — likely distinct ftune/fine_tune interaction bug separate from Bug 3.