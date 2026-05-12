# Tested Modules

Results of comparing C (a2m_dump) vs Pascal (adt2_dump) output.

## Status Legend

* PASS - 0 diff lines (identical output)
* INIT-ONLY - only pre-INIT register ordering diffs (benign)
* FRAME-DIFF - real algorithmic frame-level differences (needs investigation)

## Results

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| ALLOYRUN (VOID) | 19199 | 0 | PASS | |
| HANGOVER (VOID) | 20479 | 0 | PASS | |
| MINDFLUX (VOID) | 10891 | 0 | PASS | |
| RASTER (VOID) | 31999 | 0 | PASS | |
| TERRANIA (VOID) | 19199 | 0 | PASS | |
| andromeda | 58316 | 0 | PASS | |
| altair | 86399 | 0 | PASS | Re-checked 2026-05-13 with `MAX_FRAMES=100000`; 0 diff. `run_one_test.sh` now defaults `TIMEOUT_SEC=120` for long dumps. |
| adven | 38399 | 0 | PASS | |
| 1942 | 11997 | 0 | PASS | NOW PASSES (was INIT-ONLY) |
| top-2act | 43802 | 38758 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): 38758-line diff; first diverging frame **10895**, bank1 **`shadow_regs[1][0xA3]`** (OPL sec. ch slot for **track 10 / 0x1A3**): C=`81`, Pascal=`7d` (+4 f-num low, same “+4 steps” family as fank5). No safe `a2t.c` tweak in this pass reduced the diff (prototyped freq-shift / note routing — dropped to avoid regressions). |
| 3812funk | - | - | PASS | |
| bxx_nowgone | - | - | PASS | |
| fank5 | 93738 | 15768 | FRAME-DIFF | constant frequency offset on ch0 from frame 4812; ch0 start freq differs by 4 steps |
| mmori | - | - | PASS | |
| farhome | 21119 | 0 | PASS | |
| badapple | - | - | FRAME-DIFF | 390 frame-level diffs |
| badseed | 38019 | 0 | PASS | |
| corridor | 49139 | 36020 | FRAME-DIFF | bank1 freq regs 1 step behind; vibrato/keyoff timing. C: tempo=90, IRQ_freq=270, speed=6. vibrato pos advances by 2 per tick (speed=2). Both C and Pascal have identical ticklooper/poll_proc timing. Root cause unclear — likely a subtly different effect processing order between play_line and update_effects. |
| ca54 | - | - | FRAME-DIFF | |
| crisis | - | - | FRAME-DIFF | 112 frame-level diffs |
| fm-troni | - | - | FRAME-DIFF | 120 frame-level diffs |
| fm63b_rv | - | - | FRAME-DIFF | 321 frame-level diffs |
| os_sblas | 68181 | 10499 | FRAME-DIFF | tone portamento target includes fine_tune in C but not in Pascal |
| pink | - | 1503 | FRAME-DIFF | not yet investigated |
| whereru | 30719 | ~44555 | FRAME-DIFF | ch5 freq diverges at frame 4653; cause unclear |
