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
| altair | 86399 | 0 | PASS | `run_one_test.sh modules/altair.a2m` with `MAX_FRAMES=100000` (2026-05-13): `diff -u` empty — **86399** frames (**172798** dump lines each side). No post–song-end trimming needed. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. (`TIMEOUT_SEC=180` used for this run; ~68s wall time.) |
| adven | 38399 | 0 | PASS | |
| 1942 | 100000 | 0 | PASS | Re-checked 2026-05-13 (`MAX_FRAMES=100000`); 0 diff. (Earlier note: was INIT-ONLY once; now identical to Pascal for full dump.) |
| top-2act | 43802 | 38758 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): 38758-line diff; first diverging frame **10895**, bank1 **`shadow_regs[1][0xA3]`** (OPL sec. ch slot for **track 10 / 0x1A3**): C=`81`, Pascal=`7d` (+4 f-num low, same “+4 steps” family as fank5). No safe `a2t.c` tweak in this pass reduced the diff (prototyped freq-shift / note routing — dropped to avoid regressions). |
| 3812funk | - | - | PASS | |
| bxx_nowgone | - | - | PASS | |
| fank5 | 93738 | 3002 | FRAME-DIFF | Re-run (`MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** **3002** lines (was **15768** in an older snapshot). Song **93738** IRQ frames (**187476** dump lines/side). First diverging IRQ frame **47232** (bank **1** line only at that instant; primary bank **0** matches). Register **`shadow_regs[1][0xB0]`**: C=`2a`, Pascal=`0a` — only **bit 5 (0x20)**, i.e. OPL **key-on** on **secondary** **`B0`**, differs; **`A0`** byte matches. **`regoffs_n(10)==0x100`** ⇒ logical channel index **10** (paired **4op** low with **9**). **`key_off` / `change_freq`** handling of **`0x2000`** matches Pascal **`HI(freq) AND NOT $20`** + **`0x1fff`** merge; remaining gap likely **`macro_poll_proc`** (`MACRO_*` / **`freq_table`** bit **13**) or **`poll_proc` vs `macro_poll_proc`** interaction vs **`adt2_dump`** (`a2t_update_dump`). Next: **`A2M_DUMP_CONTEXT`** (or **`dump_context_f`**) at IRQ **47230–47234**, **`chan==10`**. |
| mmori | - | - | PASS | |
| farhome | 21119 | 0 | PASS | |
| badapple | 100000 | 71752 | FRAME-DIFF | Re-run 2026-05-13 (`run_one_test.sh modules/badapple.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** **71752** lines; **200000** dump lines/side. First diverging IRQ frame **14878** — **bank 0** line only (**bank 1** matches at that frame). Register **`shadow_regs[0][0xA5]`**: C=`0x99`, Pascal=`0x9a`. **`0xA5`** is **F-num low** (**`0xa0 + regoffs_n(4)`**), i.e. logical **channel index 4** — not a TL register. **`tremolo()`** slot **1** vs **`tremolo2`** alignment already done; this gap is **frequency**, not tremolo TL. Next per **TESTING.md**: **`A2M_DUMP_CONTEXT`** around IRQ **14876–14880**, **`chan == 4`**, compare **`freq_table[4]`**, **`change_frequency` / `change_freq`**, vibrato, and **macro** polling vs Pascal. Post–song-end: diff starts mid-song → not INIT-only trailing noise. |
| badseed | 38019 | 0 | PASS | |
| corridor | 49139 | 36020 | FRAME-DIFF | bank1 freq regs 1 step behind; vibrato/keyoff timing. C: tempo=90, IRQ_freq=270, speed=6. vibrato pos advances by 2 per tick (speed=2). Both C and Pascal have identical ticklooper/poll_proc timing. Root cause unclear — likely a subtly different effect processing order between play_line and update_effects. |
| ca54 | - | - | FRAME-DIFF | |
| crisis | 100000 | 28298 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): **28298**-line `diff -u`; first diverging frame **19580**, **bank 0** only at that instant: **`shadow_regs[0][0xA1]`** C=`57`, Pascal=`58` (f-num low byte one step low on the channel mapped to OPL **0xA1**). Bank 1 line matches at frame 19580. `calc_freq_shift_up` already uses Pascal-style `(oc<<10)+fr`. Earlier `new_process_note` / porta–note-delay alignment attempts did **not** shrink this diff; next step is a frame-local trace (vibrato + `freq_table` vs OPL) around 19579–19580. |
| fm-troni | 100000 | 50 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): **50**-line `diff -u` (was **170** before fix). **15261** primary-bank **`shadow_regs[0][0xA2]`** now matches Pascal. Cause was **row 79→80**, **track 5**: **FSlideUp** then **tone porta** with **key-off** note (`0x8d`); `porta.freq` stayed **0** or **nFreq** disagreed in octave with **freq** after slide → bogus **`tone_portamento`**. Fix in **`a2t.c` `process_effects` / `ef_TonePortamento`**: **`porta.speed`** from merged **`effect_table`**; for **key-off** + valid stripped note, **`porta.freq = freq_table & 0x1fff`** (else **`nFreq`+fine**). Also vibrato/trem **speed/depth** from merged effect bytes; **`tone_portamento`** compares like Pascal (unmasked **porta.freq**). First remaining diff ~**34557**, often **bank 1** (`… 1` dump line), not **0xA2** @15261. |
| fm63b_rv | - | - | FRAME-DIFF | 321 frame-level diffs |
| os_sblas | 68181 | 10499 | FRAME-DIFF | tone portamento target includes fine_tune in C but not in Pascal |
| pink | - | 1503 | FRAME-DIFF | not yet investigated |
| whereru | 30719 | ~44555 | FRAME-DIFF | ch5 freq diverges at frame 4653; cause unclear |
