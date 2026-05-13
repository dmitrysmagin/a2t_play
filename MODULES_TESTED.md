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
| 1942 | 100000 | 0 | PASS | Re-checked 2026-05-13 (`MAX_FRAMES=100000`); 0 diff. (Earlier note: was INIT-ONLY once; now identical to Pascal for full dump.) |
| top-2act | 43802 | 38758 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): 38758-line diff; first diverging frame **10895**, bank1 **`shadow_regs[1][0xA3]`** (OPL sec. ch slot for **track 10 / 0x1A3**): C=`81`, Pascal=`7d` (+4 f-num low, same “+4 steps” family as fank5). No safe `a2t.c` tweak in this pass reduced the diff (prototyped freq-shift / note routing — dropped to avoid regressions). |
| 3812funk | - | - | PASS | |
| bxx_nowgone | - | - | PASS | |
| fank5 | 93738 | 15768 | FRAME-DIFF | constant frequency offset on ch0 from frame 4812; ch0 start freq differs by 4 steps |
| mmori | - | - | PASS | |
| farhome | 21119 | 0 | PASS | |
| badapple | 100000 | 69478 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): **69478**-line diff (previously **75682** before fix). First diverging frame **14878**, **bank 0** **`shadow_regs[0][0xA5]`**: C=`99`, Pascal=`9a`. **`tremolo()`** slot **1** now matches Pascal **`tremolo2`**: slide down only when **`pos == 0`** after the increment (wrap), not **`pos & vibtrem_table_size`**. Altair regression (`MAX_FRAMES=20000`): still **PASS**. Remaining mismatch still open (TL/volume family at **0xA5**). |
| badseed | 38019 | 0 | PASS | |
| corridor | 49139 | 36020 | FRAME-DIFF | bank1 freq regs 1 step behind; vibrato/keyoff timing. C: tempo=90, IRQ_freq=270, speed=6. vibrato pos advances by 2 per tick (speed=2). Both C and Pascal have identical ticklooper/poll_proc timing. Root cause unclear — likely a subtly different effect processing order between play_line and update_effects. |
| ca54 | - | - | FRAME-DIFF | |
| crisis | 100000 | 28298 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): **28298**-line `diff -u`; first diverging frame **19580**, **bank 0** only at that instant: **`shadow_regs[0][0xA1]`** C=`57`, Pascal=`58` (f-num low byte one step low on the channel mapped to OPL **0xA1**). Bank 1 line matches at frame 19580. `calc_freq_shift_up` already uses Pascal-style `(oc<<10)+fr`. Earlier `new_process_note` / porta–note-delay alignment attempts did **not** shrink this diff; next step is a frame-local trace (vibrato + `freq_table` vs OPL) around 19579–19580. |
| fm-troni | 100000 | 170 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): **170**-line `diff -u`; **48** IRQ frames differ (**15261–34571**). First: **15261**, **`shadow_regs[0][0xA2]`** C=`2f` vs Pascal=`9f`. **`dump_context`** (`-DA2M_DUMP_CONTEXT`, `a2m_dump.c`): divergence lines up with **track 5** (**`regoffs_n` → 0xA2**). At 15261, **`freq_table[5]=0x0a2f`** and **`macro_table[5].vib_freq=0x0a2f`** while Pascal’s register implies **`~0x0a9f`**; **`0x9f−0x2f = 0x70`** equals **`fslide_table[0][5]=112`** and **`effect_table[0][5]`** tone-porta **`val=0x70`**. Suggests **portamento / macro-freq merge** mismatch on that row (row **80**, pattern **4**, order **6**). |
| fm63b_rv | - | - | FRAME-DIFF | 321 frame-level diffs |
| os_sblas | 68181 | 10499 | FRAME-DIFF | tone portamento target includes fine_tune in C but not in Pascal |
| pink | - | 1503 | FRAME-DIFF | not yet investigated |
| whereru | 30719 | ~44555 | FRAME-DIFF | ch5 freq diverges at frame 4653; cause unclear |
