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
| badapple | 100000 | 71752 | FRAME-DIFF | **`A2M_DUMP_CONTEXT`** at IRQ **14874–14880** (`make test DUMP_CONTEXT=1`, stderr → e.g. `test/badapple_ctx.log`): divergence aligns with **pattern 53**, **order 10**, **row 112→113** boundary (**ticks** **6199→6201**). **Channel 4** (4‑op **high**, paired low **5**): **`freq_table[4]=0x2599`**, **`ftune_table[4]=1`** (**Extended2** **FineTuneUp** **`0x41`**), **`macro_table[4].vib_freq=0x0599`**, **`vib_table=0`**. Pascal expects **`0x259a`** — **single‑step** F-num diff. **Arpeggio (`play_line`):** Pascal **`a2player.pas`** runs **`arpgg_table` / `arpgg_table2`** cleanup as **`if … else if`** **before** **`Case effect_def`**; C uses **`play_line_arpgg_cleanup_pascal`** between **`process_effects_slot_prepare`** and **`process_effects_slot_body`**. **`diff -u`** still **71752** (`MAX_FRAMES=100000`); IRQ **14878**, **`arpgg_table[*][4].note==0`** — cleanup irrelevant. Capture stderr with **`make test DUMP_CONTEXT=1`** (**GNU make**, MSYS2 **`CC`** per **`run_one_test.sh`**). **`freq_decode` / `instr_data`:** **`instrument 5`** **`fine_tune=0`** — not wrong **`srci[12]`**. **`nFreq(15)=0x0598`**. **Row 112**: **`ftune_table=0`** but **`freq&1fff=0x0599`** ⇒ **+1** vs **`nFreq+fine+ftune`**; **`calc_freq_shift_up(0x598,1)=0x599`** matches one **`portamento_up(...,1)`** (e.g. FMREG **`freq_slide`**). **Row 113**: **`ftune_table=1`**, **delta 0**. Pascal **`0x9a`** vs C **`0x99`** is **one step above `0x599`**. **`src/a2m_dump.c`** prints **`badapple ch4 porta/fslide/glfsld`** and **`badapple ch4 fmreg`** (**`freq_slide`**, **`dis_fmreg_cols`**, **`freq_slide_col26_active`**). **Next:** grep those lines in **`test/badapple_ctx.log`**. **Regression:** **`altair`** **PASS**; **`fm-troni`** **50** diff lines. |
| badseed | 38019 | 0 | PASS | |
| corridor | 49139 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/corridor.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **49139** IRQ frames (**98278** dump lines each side). ~173s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. *(Earlier snapshot listed FRAME-DIFF / ~36k-line diff; current tree matches Pascal.)* |
| ca54 | 72583 | 129429 | FRAME-DIFF | Re-run 2026-05-13 (`run_one_test.sh modules/ca54.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** **129429** lines; **`wc -l`** each dump **145166** ⇒ **72583** IRQ frames (**~55s** wall time). First diverging frame **2560**, **bank 1** only (**primary bank 0** matches): **`shadow_regs[1][0xA2]`** — C=`98`, Pascal=`95` (**f-num low +3** on C vs Pascal); **`regoffs_n(14)==0x102`** ⇒ logical channel **14** (secondary OPL **A2**, 4‑op **high** paired with **15**). **`macro_poll_proc`** loop aligned with Pascal **`For chan := 1 to nm_tracks`**: C now uses **`chan < songinfo->nm_tracks`** instead of **`chan < 20`** — **ca54 diff unchanged** on this run (correctness / parity fix). Next per **TESTING.md**: **`A2M_DUMP_CONTEXT`** / **`dump_context_f`** around IRQ **2558–2565**, **`chan==14`** — **`macro_table[14]`** vibrato (**`vib_freq`**, **`vib_count`**, **`vib_pos`**) vs **`freq_table`** / **`change_freq`** after **`macro_vibrato__porta_*`**. |
| crisis | 55870 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/crisis.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **55870** IRQ frames (**111740** dump lines each side); last frame index **55869**. ~33s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. *(Earlier snapshot: FRAME-DIFF ~28k lines @ frame **19580**, **`shadow_regs[0][0xA1]`** C=`57` vs Pascal=`58`; current tree matches Pascal — likely helped by **`macro_poll_proc`** looping **`nm_tracks`** vs fixed **20**, plus other parity work.)* |
| fm-troni | 100000 | 50 | FRAME-DIFF | Re-run 2026-05-13 (`MAX_FRAMES=100000`): **50**-line `diff -u` (was **170** before fix). **15261** primary-bank **`shadow_regs[0][0xA2]`** now matches Pascal. Cause was **row 79→80**, **track 5**: **FSlideUp** then **tone porta** with **key-off** note (`0x8d`); `porta.freq` stayed **0** or **nFreq** disagreed in octave with **freq** after slide → bogus **`tone_portamento`**. Fix in **`a2t.c` `process_effects` / `ef_TonePortamento`**: **`porta.speed`** from merged **`effect_table`**; for **key-off** + valid stripped note, **`porta.freq = freq_table & 0x1fff`** (else **`nFreq`+fine**). Also vibrato/trem **speed/depth** from merged effect bytes; **`tone_portamento`** compares like Pascal (unmasked **porta.freq**). First remaining diff ~**34557**, often **bank 1** (`… 1` dump line), not **0xA2** @15261. |
| fm63b_rv | - | - | FRAME-DIFF | 321 frame-level diffs |
| os_sblas | 68181 | 10499 | FRAME-DIFF | tone portamento target includes fine_tune in C but not in Pascal |
| pink | - | 1503 | FRAME-DIFF | not yet investigated |
| whereru | 30719 | ~44555 | FRAME-DIFF | ch5 freq diverges at frame 4653; cause unclear |
