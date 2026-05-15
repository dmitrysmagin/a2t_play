# BUGS.md — Active Bug Analysis for a2t_play

## Three Distinct Bug Classes Identified

### Bug 1: ADSR Init Artifact — FIXED
- **Root cause**: C's `release_sustaining_sound()` (`src/a2t.c:801`) only zeroed 4 of 10 ADSR fields (`decM`, `attckM`, `decC`, `attckC`), while Pascal's `FillChar` zeros all 10 fields per ADSR record (including `sustn`, `rel`, `wform`).
- **Fix applied**: Added 6 missing zeroing lines at `src/a2t.c:810-819`. Encore module diffs dropped from hundreds to 18-30 lines each.

### Bug 2: Post-RSS Volume 0x3f Artifact — UNFIXED
- **Root cause**: C's `set_ins_data()` has `if (ins == 0) return;` at line 1072. After RSS calls `release_sustaining_sound()` (which sets `event_table[chan].instr_def = 0` and `reset_chan[chan] = true`), the next row with `ins=0` returns early — skipping `voice_table[chan] = ins` (line 1125) and `reset_ins_volume(chan)` (line 1130).
- **Pascal behavior**: No early return. Lines 993-998 execute unconditionally — `voice_table[chan] := ins` and `reset_ins_volume(chan)` always run.
- **Failed Fix 2a**: Adding `reset_ins_volume(chan)` after RSS made `o2ghosts` much worse (53K → 400K diffs) because `voice_table[chan]` was 0, causing `get_instr_data_by_ch(chan)` to bail out.
- **Attempt 2026-05-15 — three variations, all regressed**:
  - **Attempt A** (ins==0: set `voice_table=0`, call `reset_ins_volume` via old voice_table, clear `reset_chan`):
    o2ghosts 30k: 53,516→89,595, crackit 3.3k: 2,128→3,938
  - **Attempt B** (same but don't clear `reset_chan`): still regressed
  - **Attempt C** (only set `voice_table=0`, `event_table=0`, clear `reset_chan`, no volume change): still regressed
  - **Root insight**: Pascal's `ins_parameter(0, x)` when `ins=0` does `&songdata.instr_data[-INSTRUMENT_SIZE + x]`. With `INSTRUMENT_SIZE=14` (SizeOf(tADTRACK2_INS)), this reads from `instr_names[255]` character ~29+x (offset ~11037–11050 in `tFIXED_SONGDATA`). Those bytes are undefined memory (beyond the Pascal String's actual length) and happen to be 0xFF (all bits set), which when masked `& $3F` yields 0x3F = volume 63. So Pascal's "unconditional" path keeps volume 63 *by accident*. Any fix that resets volume from an actual instrument value, or that changes `voice_table`/`event_table` state early, cascades into many more diffs than the original early-return approach.
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
|---|---|---|---|---|---|
| encore | 10 | 7 | 3 | 18-30 lines each after Fix 1 |
| hydra | 2 | 0 | 2 | 2,128 and 13,822 diffs |
| dretz | 11 | 11 | 0 | All PASS at 100k frames |
| o2star | 15 | 14 | 1 | `o2ghosts`: 53,516 diffs |
| televics | 36 | 8 | 28 | 4thcoast frame-drift, topgear/build/cwack/discwrld large diffs, rest small/end-of-song. |
| brendan | ~70 | ~64 | 6 | Multiple bug classes compound |
| diodema | 23 | 15 | 6 | Bug 3 fix resolved samsara (206→0), zaxxon (3548→0) |
| mlf | 14 | 9 | 5 | Bug 3 fix resolved deorbit (128→0), glass (962→0) |
| brendan | ~70 | ~65 | 5 | Bug 3 fix resolved chivalry (2438→0) |

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

1. **Fix Bug 2** (unfixed): The `if (ins == 0) return;` guard at line 1072 needs restructuring that matches Pascal's unconditional execution of `voice_table[chan] := ins` and `reset_ins_volume(chan)` at `a2player.pas:993-998`. Simple approaches regressed — needs frame-by-frame tracing with `-DA2M_DUMP_CONTEXT` at specific IRQ divergence points (e.g. o2ghosts frame 10 bank 1 volume regs) to determine exactly which register writes differ and design a fix that matches without cascading.
2. ~~Fix `e2_vslide_type` initialization~~ — confirmed NOT A BUG.
3. ~~**Bug 3 (TonePortamento on note=0)** — FIXED 2026-05-15.~~ See root cause above. Resolved 7 modules.
4. **Re-test all FRAME-DIFF modules** after each fix.
5. **Update MODULES_TESTED.md** with results. (Updated 2026-05-15: Bug 3 fix resolved samsara/deorbit/glass fully; partially resolved others. All tested at `MAX_FRAMES=50000`.)
6. **Investigate remaining ±1 nibble offsets** (null, signs, aquarius, fm-troni, spacediv, old_002, psycho3x, psycho5) — likely distinct ftune/fine_tune interaction bug separate from Bug 3.