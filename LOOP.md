# play_line: Pascal vs C Line-by-Line Comparison

## Pascal play_line structure (5 loops)

| Loop | Pascal lines | What it does | C equivalent |
|------|-------------|--------------|--------------|
| **LOOP 1** | 1306-1467 | Per-channel: read event, save last_effect, clear effect_table def (AND $0ff00), ftune=0, fixup note, compute event_new, copy eff→event_table, set_ins_data, clear vibr/retrig/trem tables, arpeggio cleanup, GlobalFSlide slot 0+1 | LOOP 1 (2195-2276): same operations, but split into `process_effects_slot_prepare`, `play_line_arpgg_cleanup_pascal`, `play_line_apply_global_fslide_row` |
| **LOOP 2** | 1469-1496 | 4op event_new propagation, tremor row reset (both slots), save last_effect→eLo/eHi arrays | LOOP 2 (2278-2281): only `play_line_tremor_row_reset`. No 4op propagation. No eLo/eHi save (C reads last_effect directly in body functions) |
| **LOOP 3** | 1498-2074 | `Case event[chan].effect_def` — sets effect_table, porta_table, vibr_table, trem_table, etc. for slot 0 | LOOP 3 (2283-2286): `process_effects_slot_body(event, 0, chan)` |
| **LOOP 4** | 2076-2650 | `Case event[chan].effect_def2` — same for slot 1 | LOOP 4 (2288-2291): `process_effects_slot_body(event, 1, chan)` |
| **LOOP 5** | 2652-2795 | tporta_flag, effect_table zeroing (if eff=0 and glfsld=0), event_table eff copy, key_off/output_note, SwapArpeggio/SwapVibrato macros, update_fine_effects | LOOP 5 (2293-2302): `new_process_note`, `check_swap_arp_vibr`, `update_fine_effects`. **FIXED**: effect_table zeroing and event_table eff copy added after Case blocks, matching Pascal timing. |

## Key discrepancies found

### 1. LOOP 2 — Missing 4op event_new propagation

- Pascal (1471-1474): If channel is 4op and event_new, also set the paired channel's event_new.
- C: No equivalent. This could cause 4op instrument changes to not propagate to the paired channel.

### 2. LOOP 5 — Missing effect_table zeroing — FIXED

- Pascal (2661-2673): If `effect_def+effect = 0` AND `glfsld_table = 0`, zero effect_table. This runs AFTER the Case blocks.
- C: **FIXED** — Added equivalent logic after `process_effects_slot_body` loops (LOOP 3/4) and before `new_process_note` (LOOP 5). Matches Pascal's timing: zero effect_table when no effect AND no global fslide; copy effect to event_table when effect is present.
- Result: `opl303.a2m` divergence reduced from 12,144 → 2,640 diff lines. `spaceple.a2m` remains at 0 diffs.

### 3. LOOP 5 — event_table eff copy timing

- Pascal (2664-2672): Copies event effect fields to event_table in LOOP 5, AFTER the Case blocks.
- C (2234-2251): Copies event effect fields to event_table in LOOP 1, BEFORE the Case blocks.
- Impact: Pascal's Case blocks use event_table eff fields from the PREVIOUS row. C's Case blocks use event_table eff fields from the CURRENT row.

### 4. process_effects_slot_prepare zeros effect_table.val

- Pascal: `effect_table := effect_table AND $0ff00` preserves val (HI byte).
- C: `process_effects_slot_prepare` zeros both def and val when event has no effect.
- This destroys carry-over val for effects like TonePortamento that rely on val persistence across rows.

## opl303.a2m divergence

The most likely cause of the `opl303.a2m` divergence (frame 9361, chan 3, freq 0x0E63 vs 0x0E33) is discrepancy #4: C zeros effect_table.val in `process_effects_slot_prepare`, while Pascal preserves it via `AND $0ff00`. This affects how carry-over effects behave at row transitions.
