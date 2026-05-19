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

### Bug 12: TonePortamento effect_table Cleared on New Note with val=0 — FIXED
- **Root cause**: When a row carries a note AND `ef_TonePortamento` with `val=0` (no speed specified) AND no prior TonePortamento carry-over, C's `update_effect_table` (`src/a2t.c:1302`) falls into its else branch and clears `effect_table` (def=0, val=0). Pascal (`a2player.pas:1574-1579`) instead sets `effect_table := ef_TonePortamento` (def=3, val=0) — preserving the effect type so that `new_process_note` sees portamento active and defers the note output.
- **How it manifests**: At frame 2045 of `limitbrk.a2m`, channels 3/4 have `note=0x32/0x36`, `eff[0]=03:00`. C clears `effect_table[0]` to `00:00`, so `new_process_note` sees `defer_note_row=false` and outputs the note immediately. Pascal keeps `effect_table.def=ef_TonePortamento`, defers the note, and activates tone_portamento with `porta_table.freq` set to the target note frequency. This cascades into 310,177 diff lines (PT: 46,800 | 0/F/MB: 28,050 each).
- **Fix applied** (`src/a2t.c:1584-1598`): Replaced `update_effect_table` call in the TonePortamento case with inline logic matching Pascal's three-way branch: (1) if `val != 0`: set def+val, (2) else if last_effect is TonePortamento with non-zero val: carry over val, (3) else: set def=ef_TonePortamento, val=0.
- **Resolved module**: `limitbrk` — 310,177 diff lines → **0** (verified at 30k frames).

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

### Bug 10: C ef_SetInsVolume/ef_ForceInsVolume Missing is_data_empty Guard — FIXED
- **Root cause**: Pascal's `ef_SetInsVolume` and `ef_ForceInsVolume` handlers (`a2player.pas:1662-1670, 1672-1678`) use a two-part guard: `voice_table[chan] <> 0` **and** `not is_data_empty(songdata.instr_data[voice_table[chan]], INSTRUMENT_SIZE)`. C only checked if the instrument index was valid (`get_instr_data_by_ch` returns non-NULL), but did not check if the instrument data was all zeros (empty). When an instrument slot exists at a valid index but contains zeroed data, C processed the volume effect (writing `volC = 63 - val` to the carrier volume register) while Pascal skipped it entirely.
- **How it manifests**: For `amegas.a2m`, channels 2/5/6/7 use instrument 0x12 which exists but has all-zero FM data. At frame 3865, `ef_SetInsVolume` with `val=0x00` triggers: C calls `set_ins_volume(BYTE_NULL, 0x3f, chan)` → `fmpar_table[chan].volC = 0x3f`, carrier vol register = silence. Pascal skips the effect → `fmpar_table[chan].volC` stays at 0x00 from prior `reset_ins_volume` call.
- **Fix applied** (`src/a2t.c`): Added `is_data_empty(instr, sizeof(tINSTR_DATA))` check to both `ef_SetInsVolume` and `ef_ForceInsVolume` handlers, matching Pascal's guard.
- **Resolved module**: `amegas` — 437,366 diff lines → **0** (verified at 30k frames).

### Bug 13: v5-8 Loader Missing ManualFSlide→FineTune Conversion — FIXED
- **Root cause**: C's v5-8 pattern loader (`src/a2t.c:4105-4160`) read raw effect bytes directly without converting `ef_ManualFSlide` (22) to `ef_Extended2` with `FineTuneUp`/`FineTuneDown` sub-commands. Pascal's `import_old_a2m_event2` (`iloaders.inc:395-423`) performs this conversion: when `effect_def == 22`, it maps to `ef_Extended2` with `ef_ex2_FineTuneUp*16 + (effect/16)` or `ef_ex2_FineTuneDown*16 + (effect%16)`.
- **How it manifests**: C interprets raw def=22 as `ef_Tremolo` (new format), while Pascal converts it to FineTune effects. This causes `ftune_table` to diverge (C stays at 0, Pascal accumulates fine-tune values), cascading into frequency table and shadow register diffs.
- **Fix applied** (`src/a2t.c:4130-4142`): Added ManualFSlide conversion in v5-8 loader, matching Pascal's logic.
- **Resolved module**: `old_002` — 185,144 diff lines → 2,942 (98.4% reduction). Remaining 2,942 diffs are a separate frequency offset issue (0x30 delta, 420 frames).
- **Root cause**: C's `arpgg_table[slot][chan].state` reaches a different state than Pascal's `arpgg_table[chan].state` during arpeggio effect carry-over (rows where `ef_Arpeggio` persists with `val=0x00`). The state machine cycles `0→1→2→0`, and at frame 1378 for `rbfactry` channel 10, C reaches state 2 (uses `add2=15`) while Pascal reaches state 1 (uses `add1=0`).
- **How it manifests**: `arpeggio()` computes `freq = nFreq(note-1+add)` based on current state. C: `nFreq(49-1+15) = nFreq(63) = 0x1598`. Pascal: `nFreq(49-1+0) = nFreq(48) = 0x1157`. Frequency delta = `0x441` (1089). This propagates to `freq_table`, `macro_table.vib_freq` (MB line), and OPL F-number registers (shadow_regs bank 1).
- **Key locations**:
  - C: `src/a2t.c:2584-2600` (`arpeggio()` function — state transition + freq computation)
  - C: `src/a2t.c:1486-1507` (`process_effects_slot_body` — arpeggio effect handling, `reset_state` logic)
  - C: `src/a2t.c:1322-1345` (`play_line_arpgg_cleanup_pascal` — pre-Case cleanup)
  - C: `src/a2t.c:1433-1456` (`process_effects_slot_prepare` — unconditional `effect_table` overwrite)
  - Pascal: `a2player.pas:3063-3081` (`arpeggio()` — state transition + freq computation)
  - Pascal: `a2player.pas:1490-1535` (LOOP1 arpeggio effect handling — note/state reset logic)
  - Pascal: `a2player.pas:1361-1374` (LOOP1 arpgg cleanup — `NOT (effect_def = ef_Arpeggio) and (effect <> 0)` guard)
- **Debug findings** (2026-05-16):
  - `poll_proc()` is called every 3 frames (frames 1375, 1378, 1381) — same timing in both C and Pascal.
  - `play_line()` is called at frames 1375 and 1381, but NOT at 1378 (speed condition `1 >= speed` is false).
  - At frame 1378, `effect_table` has `def=0, val=15` (carried over from frame 1375's `play_line()`).
  - `arpeggio()` is called at frames 1375, 1378, 1381 with states 1, 2, 0 respectively.
  - C's state at frame 1378: 2 (before `arpeggio()` advances to 0). Pascal's state at frame 1378: 1.
  - The divergence is NOT about `effect_table` being cleared on x00 rows (the debug shows `val=15`, not 0).
  - The divergence is about the state machine cycling at different rates between C and Pascal.
- **Failed fix attempts** (all caused 479K-line regressions):
  1. Guard in `process_effects_slot_prepare`: `if ((def != ef_Arpeggio) || (val != 0))` — too broad, affects all `def=0, val=0` rows.
  2. Restore from `event_table` in `process_effects_slot_body` — `event_table` has wrong values for many rows.
  3. Check previous `effect_table.def` before preserving — `ef_Arpeggio=0` makes it impossible to distinguish "no effect" from "arpeggio carry-over".
  4. Check `event_table[chan].eff[slot]` for arpeggio — `event_table` carries over values, causing false positives.
- **Core problem**: `ef_Arpeggio = 0`, so `def=0, val=0` rows are indistinguishable from "no effect" rows. Any fix that preserves `effect_table` for `def=0, val=0` also preserves it for "no effect" rows, causing massive regressions.
- **Investigation needed**:
  1. Why does Pascal's state machine cycle at a different rate than C's? Both call `poll_proc` at the same frames.
  2. Is there a difference in how `effect_table` is stored? Pascal uses `ef_Arpeggio+ef_fix1` ($80) in low byte; C uses `ef_Arpeggio` (0) in `def` field.
  3. Does Pascal's `update_effects` check a different condition than C's `update_effects_slot`?
  4. Consider adding `ef_fix1` ($80) to C's `effect_table.def` for arpeggio effects, to match Pascal's encoding and enable proper carry-over detection.
- **Affected modules**: `rbfactry` (842 diff lines at 30k, 240 non-MB). `drgwrrtt` (75 F/MB/0 diffs, arpeggio state out of phase by 1 step causing 0x46 frequency offset on ch6). Likely affects other modules with arpeggio carry-over patterns.

### Bug 15: retrig_table Off-by-1 Timing Alignment — UNFIXED
- **Root cause**: C's `ticklooper` timing is misaligned with Pascal's by 1 frame increment for the retrig_table counter on channel 2 of `yellowwe.a2m`. Beginning at frame 26905 (when ch2 receives note=37, ins=3 with `ef_RetrigNote` effect value 0xF1=241), C's `retrig_table[2]` is always exactly 1 ahead of Pascal's (`C = Pascal + 1`) for 30 consecutive frames.
- **Pattern observed** (frames 26905–26934, all diffs on ch2 only):
  ```
  Frames    Pascal   C
  26905-09  1        2
  26910-14  2        3
  26915-19  3        4
  26920-24  4        5
  26925-29  5        6
  26930-34  6        7
  ```
  Delta is always 1, increments every 5 frames (matching retrig speed parameter).
- **How it manifests**: `retrig_table[chan]` increments each tick. When it reaches the effect value threshold (241), the note is retriggered. With C 1 tick ahead, in a module with retrig speed=1 and a long enough pattern, C would trigger the retrig 1 frame before Pascal. For yellowwe the window is only 30 frames and the threshold 241 is never reached, so it's benign.
- **Likely cause**: The `ticklooper` reset/wrap logic at `src/a2t.c:4608-4610` (C) vs `a2player.pas:4306-4308` (Pascal) aligns `play_line` calls to a different phase of the audio frame cycle. The 1-frame offset propagates to the RT counter increment inside `update_effects`.
- **Affected modules**: yellowwe (272 diff lines, all RT off-by-1). Potentially any module using `ef_RetrigNote`/`ef_MultiRetrigNote` where the retrig counter stays below threshold within the active window; audible only if the retrig threshold is reached during a pattern segment.
- **Fix direction**: Align C's `ticklooper` reset timing with Pascal's — either adjust the reset condition at `src/a2t.c:4609` (`if (ticklooper >= IRQ_freq / tempo) ticklooper = 0;`) to match Pascal's `if (ticklooper >= IRQ_freq DIV tempo) then ticklooper := 0;`, or adjust the increment timing of `retrig_table[chan]` to match Pascal's update_effects tick count.

### Bug 14: Arpeggio val=0 Skip Guard Missing — FIXED
- **Root cause**: Pascal (`a2player.pas:1498-1499`) skips the entire arpeggio block when `effect_def = ef_Arpeggio` AND `effect = 0`, preserving the previous arpeggio state/add1/add2. C had no such guard and always processed the arpeggio block, overwriting `add1`/`add2` with 0 and resetting state on carry-over rows.
- **How it manifests**: On rows where `ef_Arpeggio` persists with `val=0x00` (pattern data contains raw 0, normalized to 0x80 in player state), C zeroes the arpeggio parameters while Pascal preserves them. This causes the arpeggio state machine to cycle at different rates, producing frequency table, arpeggio table, and shadow register diffs.
- **Fix applied** (`src/a2t.c:1478-1480`): Added guard before the arpeggio switch block: `if ((def == ef_Arpeggio) && (val == 0)) break;`
- **Resolved module**: `4xmisste` — 228,524 diff lines → 25,362 (88.9% reduction). All AT/F/0 diffs eliminated (72→0, 48→0, 48→0). Remaining 25,362 are benign MB-only keyoff_loop state diffs (same as Bug 11).

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
| encore | 10 | 8 | 2 | Bug 10 resolved amegas (437K→0). 18-30 lines each after Fix 1 for others. |
| hydra | 2 | 0 | 2 | 2,128 and 13,822 diffs |
| dretz | 11 | 11 | 0 | All PASS at 100k frames |
| o2star | 15 | 14 | 1 | `o2ghosts`: 53,516 diffs |
| televics | 36 | 8 | 28 | 4thcoast frame-drift, topgear/build/cwack/discwrld large diffs, rest small/end-of-song. 5ontelev: Bug 11 benign MB diff (2,304). 4xmisste: Bug 14 fixed (228K→25K), remaining MB is Bug 11. |
| brendan | ~70 | ~64 | 6 | Multiple bug classes compound |
| diodema | 23 | 15 | 6 | Bug 3 fix resolved samsara (206→0), zaxxon (3548→0) |
| mlf | 14 | 9 | 5 | Bug 3 fix resolved deorbit (128→0), glass (962→0) |
| mlf | 14 | 9 | 5 | Bug 13 fix resolved old_002 (185,144→2,942, 98.4% reduction) |
| brendan | ~70 | ~65 | 5 | Bug 3 fix resolved chivalry (2438→0) |
| brendan | ~70 | ~66 | 4 | Bug 8 resolved dream7mx (C–Pascal divergence at 0x14c eliminated. Shadow reg diff: 0) |
| kkonaa | 2 | 1 | 1 | `limitbrk`: Bug 12 fix resolved (310,177→0). `adr1ft`: Bug 11 (benign keyoff_loop MB diff). |

## Key Files

- `src/a2t.c:1072` — the `if (ins == 0) return;` guard to fix (Bug 2)
- `src/a2t.c:801-830` — `release_sustaining_sound` (Bug 1, fixed)
- `src/a2t.c:1210-1259` — C's `output_note` (Bug 3 location)
- `src/a2t.h:278-297` — `tFM_INST_DATA` and `tINSTR_DATA` structs
- `adt2play_sdl/a2player.pas:925-999` — Pascal's `set_ins_data` (reference)
- `adt2play_sdl/a2player.pas:1111-1177` — Pascal's `output_note` (reference)
- `src/a2t.c:2121-2215` — C's `play_line` (ordering reference)
- `src/a2t.c:1648-1691` — C's `ef_SetInsVolume`/`ef_ForceInsVolume` handlers (Bug 10 fix: added `is_data_empty` guard)
- `adt2play_sdl/a2player.pas:1662-1678` — Pascal's `ef_SetInsVolume`/`ef_ForceInsVolume` handlers (reference for Bug 10)
- `src/a2t.c:2175-2189` — C's `play_line` unconditional eff write (Bug 4 fix)
- `src/a2t.c:3416-3420` — C's `init_player` key_off/init_buffers order (Bug 5 fix)
- `adt2play_sdl/a2player.pas:1313-1322` — Pascal's LOOP1 unconditional eff write (reference for Bug 4)

## Next Steps

1. **Bug 11 (keyoff_loop state divergence)** — investigate why Pascal's `keyoff_loop[13..14]` is `true` at frame 161+ for `adr1ft.a2m` while C's is `false`. No X1 effects (`ef_ex_cmd_MKOffLoopEn`) found in pattern data for channels 12-13. Both implementations initialize `keyoff_loop=FALSE` and only set it via X1 effect. Benign: zero shadow register or audio diffs, strictly MB (macro state) dump difference. 118,703 diff lines across 3,514 frames (all MB-only). Also affects `4xmisste` (25,362 MB), `5ontelev` (2,304 MB), `glass` (12,240 MB), `xmission` (20,754 MB), `zaxxon` (2,304 MB).
2. **Bug 9 (arpeggio state divergence)** — investigate exact state transition sequence between C and Pascal for `rbfactry` frames 1377→1378. Compare Pascal's `arpgg_cleanup` (`a2player.pas:1361-1374`) and effect carry-over logic (`a2player.pas:1494-1535`) against C's `play_line_arpgg_cleanup_pascal` and `process_effects_slot_body`.
3. **Bug 7 (instrument data truncated)** — fix C's loader to read all 255 instrument slots from the file instead of only `count` entries. This would resolve the remaining MV/CV diffs in `o2ghosts`, `sparkplg`, `sweetsin`.
3. ~~Fix `e2_vslide_type` initialization~~ — confirmed NOT A BUG.
4. ~~**Bug 3 (TonePortamento on note=0)** — FIXED 2026-05-15.~~ Resolved 7 modules.
 5. ~~**Bug 6 (NULL instrument in volume functions)** — FIXED 2026-05-16.~~ Resolved `mechwar` (25,624 → 0).
 6. ~~**Bug 8 (Pascal SetInsVolume bounds guard)** — FIXED 2026-05-16.~~ Resolved `dream7mx` (0x14c divergence eliminated).
  7. ~~**Bug 10 (C ef_SetInsVolume/ef_ForceInsVolume is_data_empty guard)** — FIXED 2026-05-17.~~ Resolved `amegas` (437,366 → 0).
  8. ~~**Bug 12 (TonePortamento effect_table cleared on new note with val=0)** — FIXED 2026-05-18.~~ Resolved `limitbrk` (310,177 → 0).
  9. ~~**Bug 13 (v5-8 loader missing ManualFSlide→FineTune conversion)** — FIXED 2026-05-18.~~ Resolved `old_002` (185,144 → 2,942, 98.4% reduction).
  10. ~~**Bug 14 (Arpeggio val=0 skip guard missing)** — FIXED 2026-05-18.~~ Resolved `4xmisste` (228,524 → 25,362, 88.9% reduction).
  11. **Re-test all FRAME-DIFF modules** after each fix.
8. **Update MODULES_TESTED.md** with results.
9. **Investigate remaining ±1 nibble offsets** (null, signs, aquarius, fm-troni, spacediv, old_002, psycho3x, psycho5) — likely distinct ftune/fine_tune interaction bug separate from Bug 3.