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

### Bug 3: ±1 Nibble Frequency Offset — UNFIXED
- **Affected modules**: 14 modules across `brendan/diode/songs100-108` sets: `deorbit`, `glass`, `old_002`, `opl303`, `pink`, `spacediv`, `chivalry`, `song100`, `trance2`, `mechage`, `sparkplg`, `lostcaus`, `lemmings`, `lucky7s`.
- **Observation**: Divergence between Pascal's `nFreq(note-1) + SHORTINT(ins_parameter(ins,12))` and C's `nFreq(note - 1) + get_instr_fine_tune(ins)` at `output_note()`.
- **Hypothesis**: Signedness or rounding discrepancy in fine_tune application timing. Both paths match structurally but diverge in when `ftune_table` is applied.

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
|---|---|---|---|---|
| encore | 10 | 7 | 3 | 18-30 lines each after Fix 1 |
| hydra | 2 | 0 | 2 | 2,128 and 13,822 diffs |
| dretz | 11 | 11 | 0 | All PASS at 100k frames |
| o2star | 15 | 14 | 1 | `o2ghosts`: 53,516 diffs |
| televics | 36 | 35 | 0 | 1 unloadable (`slappy.a2m`) |
| brendan | ~70 | ~64 | 6 | Multiple bug classes compound |

## Key Files

- `src/a2t.c:1072` — the `if (ins == 0) return;` guard to fix (Bug 2)
- `src/a2t.c:801-830` — `release_sustaining_sound` (Bug 1, fixed)
- `src/a2t.c:1210-1259` — C's `output_note` (Bug 3 location)
- `src/a2t.h:278-297` — `tFM_INST_DATA` and `tINSTR_DATA` structs
- `adt2play_sdl/a2player.pas:925-999` — Pascal's `set_ins_data` (reference)
- `adt2play_sdl/a2player.pas:1111-1177` — Pascal's `output_note` (reference)
- `src/a2t.c:2121-2215` — C's `play_line` (ordering reference)

## Next Steps

1. **Fix Bug 2**: The `if (ins == 0) return;` guard at line 1072 needs restructuring that matches Pascal's unconditional execution of `voice_table[chan] := ins` and `reset_ins_volume(chan)` at `a2player.pas:993-998`. Simple approaches regressed — needs frame-by-frame tracing with `-DA2M_DUMP_CONTEXT` at specific IRQ divergence points (e.g. o2ghosts frame 10 bank 1 volume regs) to determine exactly which register writes differ and design a fix that matches without cascading.
2. ~~Fix `e2_vslide_type` initialization~~ — confirmed NOT A BUG.
3. **Investigate Bug 3**: Build with `-DA2M_DUMP_CONTEXT` and compare `ftune_table`/`SHORTINT(ins_parameter)` values at divergence frames.
4. **Re-test all FRAME-DIFF modules** after each fix.
5. **Update MODULES_TESTED.md** with results.