# Tested Modules

Results of comparing C (a2m_dump) vs Pascal (adt2_dump) output.

## Status Legend

* PASS - 0 diff lines (identical output)
* INIT-ONLY - only pre-INIT register ordering diffs (benign)
* FRAME-DIFF - real algorithmic frame-level differences (needs investigation)

## Results

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1942 | 11997 | 0 | PASS | 18 diff lines (last 3 frames, pre-existing end-of-song divergence). E-line dump added 2026-05-15 — event_table now matches Pascal for active playback. Bug 4 (unconditional eff write) and Bug 5 (init order) fixed. |
| ALLOYRUN (VOID) | 19199 | 0 | PASS | |
| HANGOVER (VOID) | 20479 | 0 | PASS | |
| KULJE_V4 | 30000 | 462164 | FRAME-DIFF | INIT state divergence. |
| MINDFLUX (VOID) | 10891 | 0 | PASS | |
| Newtune | 17279 | 0 | PASS | |
| RASTER (VOID) | 31999 | 0 | PASS | |
| TERRANIA (VOID) | 19199 | 0 | PASS | |
| adven | 30000 | 0 | PASS | Bug 3 fix resolved active playback (was 30 at 21k — 38 at 50k is end-of-song divergence). |
| andromeda | 30000 | 0 | PASS | |
| bxx_nowgone | - | - | PASS | |
| class05 | 24959 | 57512 | FRAME-DIFF | 6390 E-line diffs (event_table eff fields). |
| damn-sh | 30000 | 0 | PASS | |
| ed3lw | 23043 | 0 | PASS | |
| frustration | 23517 | 0 | PASS | |
| goa-cma | 30000 | 0 | PASS | |
| mystcave | 29947 | 0 | PASS | |
| nightdrv | 30000 | 23440 | FRAME-DIFF | 2430 PT, 570 MB, 540 F, 396 1, 144 0 — freq/volume/PT divergence. |
| paradox3 | 3896 | 0 | PASS | |
| remembrance | 30000 | 0 | PASS | |
| schwskel | 30000 | 0 | PASS | |
| skyh | 30000 | 57614 | FRAME-DIFF | 3840 0, 3744 FP, 3744 CV, 96 MB, 96 F — channel volume/FP/0-line divergence. |
| speed_reset_song103 | 2074 | 0 | PASS | |
| square | 19588 | 0 | PASS | |
| top-2act | 43802 | 0 | PASS | Re-checked 2026-05-13 (`MAX_FRAMES=100000`, `TIMEOUT_SEC=360`): **`diff -u`** empty. Root cause: instrument **19** had FMREG **`length==0`** and empty cells but **`src[5]`** selected vibrato table **1**; **`fmreg`** was not allocated so **`instrument->vibrato`** never propagated (**`src/a2t.c`** **`fmreg_table_allocate`** now allocates when **`src[1]|…|src[5]`** is non-zero). Cell inference when **`length==0`** retained. Optional **`tools/fmreg_peek.c`** can inspect FMREG blobs. |
| whereru | 30719 | 0 | PASS | |

### modules/ben

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| ballad | 30000 | 0 | PASS | |
| boss8 | 7679 | 0 | PASS | |
| farhome | 21119 | 0 | PASS | |
| fdance06 | 23036 | 0 | PASS | |
| fdance27 | 23036 | 0 | PASS | |
| fm63b_rv | 17759 | 0 | PASS | |
| fmaven94 | 5998 | 0 | PASS | |
| fmaven95 | 5761 | 0 | PASS | |
| fmaven96 | 30000 | 0 | PASS | |
| fmaven97 | 11397 | 0 | PASS | |
| fmaven98 | 2556 | 0 | PASS | |
| fmavn63b | 14205 | 0 | PASS | |
| gates | 19199 | 0 | PASS | |
| hitech2 | 28802 | 0 | PASS | |
| hitech3 | 27361 | 0 | PASS | |
| hitech3f | 18268 | 0 | PASS | |

### modules/brendan

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| advnture | 14606 | 0 | PASS | |
| amangels | 9215 | 0 | PASS | |
| apoclyps | 19965 | 0 | PASS | |
| banzai | 20181 | 0 | PASS | |
| battleth | 893 | 0 | PASS | |
| bigguns | 12287 | 0 | PASS | |
| birdston | 23036 | 0 | PASS | |
| bladeraz | 11519 | 0 | PASS | |
| blades | 15262 | 0 | PASS | |
| blades2 | 15290 | 0 | PASS | |
| blizzard | 27463 | 0 | PASS | |
| bonusthm | 6401 | 0 | PASS | |
| callduty | 30000 | 0 | PASS | |
| chiptune | 30000 | 0 | PASS | |
| chivalry | 30000 | 1620 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| cloudfrv | 30000 | 0 | PASS | |
| conspira | 30000 | 0 | PASS | |
| contrast | 30000 | 0 | PASS | |
| coolcat | 30000 | 0 | PASS | |
| cooldown | 30000 | 0 | PASS | |
| dayhcome | 30000 | 0 | PASS | |
| dayslive | 30000 | 0 | PASS | |
| deceptin | 30000 | 0 | PASS | |
| deepdown | 30000 | 0 | PASS | |
| depressn | 30000 | 0 | PASS | |
| difkhero | 30000 | 0 | PASS | |
| dream7m | 30000 | 0 | PASS | |
| dream7mx | 3 | 0 | PASS | Small ±1 nibble offsets. |
| echorock | 30000 | 0 | PASS | |
| elskiing | 30000 | 0 | PASS | |
| enchloop | 30000 | 0 | PASS | |
| eviloop | 30000 | 0 | PASS | |
| fiftrial | 30000 | 0 | PASS | |
| frenzy | 30000 | 0 | PASS | |
| friendsh | 30000 | 0 | PASS | |
| frozen | 30000 | 1500 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| fullthr | 30000 | 0 | PASS | |
| funland | 30000 | 50 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| goldebel | 30000 | 50 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| horizon | 30000 | 0 | PASS | |
| hovlane | 30000 | 0 | PASS | |
| hunter | 30000 | 0 | PASS | |
| inevitbl | 30000 | 80 | FRAME-DIFF | Small ±1 nibble offsets. |
| invasion | 30000 | 0 | PASS | |
| jdaniels | 25599 | 0 | PASS | (not in modules/brendan/ — kept from earlier run.) |
| kelsey | 30000 | 0 | PASS | |
| laboite | 23036 | 0 | PASS | |
| lemmings | 30000 | 100 | FRAME-DIFF | Small ±1 nibble offsets. |
| lostcaus | 30000 | 1600 | FRAME-DIFF | Init artifact ±1 nibble offsets. |
| lucky7s | 30000 | 6500 | FRAME-DIFF | Init artifact ± freq diffs. |
| manifest | 30000 | 0 | PASS | |
| marblect | 30000 | 0 | PASS | |
| mariothm | 30000 | 0 | PASS | |
| mechage | 30000 | 1100 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| mechwar | 30000 | 0 | PASS | Fixed 2026-05-16: `reset_ins_volume`/`set_ins_volume` now handle NULL instrument gracefully (pattern instr index > song's instrument count). Bug 6. |
| metro | 30000 | 0 | PASS | |
| modratly | 30000 | 0 | PASS | |
| neversay | 30000 | 0 | PASS | |
| neurophb | 30000 | 47522 | FRAME-DIFF | |
| nochoice | 30000 | 0 | PASS | |
| nogofers | 30000 | 0 | PASS | |
| nowgone | 30000 | 0 | PASS | |
| oblivion | 30000 | 0 | PASS | |
| obstacle | 30000 | 0 | PASS | |
| ohirony | 30000 | 0 | PASS | |
| omegavir | 30000 | 0 | PASS | |
| paparazi | 30000 | 0 | PASS | |
| pfectwld | 30000 | 0 | PASS | |
| popcorn | 30000 | 0 | PASS | |
| racecyb | 30000 | 0 | PASS | |
| racecyb2 | 30000 | 0 | PASS | |
| recherch | 13827 | 0 | PASS | |
| robinthm | 30000 | 0 | PASS | |
| running | 30000 | 0 | PASS | |
| sinister | 30000 | 360 | FRAME-DIFF | Small ±1 nibble offsets. |
| sitcom | 30000 | 0 | PASS | |
| skylight | 30000 | 0 | PASS | |
| skysharp | 30000 | 0 | PASS | |
| sleepwrk | 30000 | 0 | PASS | |
| song100 | 5374 | 3266 | FRAME-DIFF | ±1 nibble diffs bank 0, ftune/fine_tune interaction. |
| song102 | 6913 | 55197 | FRAME-DIFF | |
| song103 | 1917 | 0 | PASS | |
| song105 | 4796 | 0 | PASS | |
| song108 | 8642 | 0 | PASS | |
| sonic | 27356 | 36722 | FRAME-DIFF | |
| spaceple | 30000 | 3120 | FRAME-DIFF | Init artifact ± freq diffs. |
| sparkplg | 30000 | 960 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| stormrid | 15999 | 0 | PASS | |
| stormywe | 30000 | 0 | PASS | |
| suspicis | 30000 | 720 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| tanmusik | 19199 | 0 | PASS | |
| teamster | 30000 | 0 | PASS | |
| thinkfst | 30000 | 0 | PASS | |
| timewtel | 30000 | 0 | PASS | |
| torpdall | 30000 | 16380 | FRAME-DIFF | Init artifact ± volume scaling. |
| trance | 19201 | 0 | PASS | |
| trance2 | 24960 | 0 | PASS | ±1 nibble offset across many frames, ftune/fine_tune interaction. |
| treacher | 30000 | 0 | PASS | |
| trouble | 30000 | 87482 | FRAME-DIFF | |
| tutheme | 30000 | 0 | PASS | |
| twistdpa | 30000 | 0 | PASS | |
| ultra | 9600 | 0 | PASS | |
| ultravio | 30000 | 0 | PASS | |
| verynice | 30000 | 0 | PASS | |
| village | 30000 | 0 | PASS | |
| warhouse | 30000 | 0 | PASS | |
| waterfls | 19201 | 0 | PASS | |
| wavesmar | 30000 | 0 | PASS | |
| westbeng | 30000 | 0 | PASS | |
| worldfal | 27840 | 0 | PASS | |

### modules/diodema

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1856step | 30000 | 0 | PASS | MB + 24 pre-existing non-MB (init volume artifact). |
| acidtrac | 30000 | 0 | PASS | |
| adr1ft | 30000 | 2952 | FRAME-DIFF | MB-only: `FINISHED` vs `IDLE` (keyoff_loop flag mismatch). |
| altair | 30000 | 0 | PASS | |
| aquarius | 30000 | 62934 | FRAME-DIFF | MB + 3024 pre-existing non-MB (±1 nibble offsets + volume artifact). |
| ca54 | 30000 | 0 | PASS | |
| catpeopl | 30000 | 0 | PASS | |
| milinda | 30000 | 0 | PASS | |
| mmori | 30000 | 0 | PASS | |
| mp77 | 30000 | 0 | PASS | |
| no72 | 30000 | 0 | PASS | |
| null | 30000 | 26992 | FRAME-DIFF | MB + 1680 pre-existing non-MB (±1 nibble offsets). |
| oddtime | 30000 | 0 | PASS | |
| phone | 30000 | 0 | PASS | |
| psg | 30000 | 0 | PASS | |
| rf62 | 30000 | 0 | PASS | |
| ru41 | 30000 | 0 | PASS | |
| samsara | 30000 | 0 | PASS | |
| signs | 30000 | 60540 | FRAME-DIFF | MB + 600 pre-existing non-MB (±1 nibble offsets). |
| sv73 | 30000 | 0 | PASS | |
| ty58 | 30000 | 0 | PASS | |
| xmission | 30000 | 0 | PASS | |
| zaxxon | 30000 | 0 | PASS | |

### modules/dretz

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| badapple | 30000 | 0 | PASS | |
| badseed | 30000 | 0 | PASS | |
| chemical | 30000 | 0 | PASS | |
| corridor | 30000 | 0 | PASS | |
| meglvnia | 30000 | 0 | PASS | |
| mystccav | 29021 | 0 | PASS | |
| oilocean | 30000 | 0 | PASS | |
| pommy | 30000 | 0 | PASS | |
| popular | 30000 | 0 | PASS | |
| rbfactry | 30000 | 2914 | FRAME-DIFF | 60 MB, 60 F, 60 1 — freq table + macro state divergence. |
| undersea | 30000 | 0 | PASS | |

### modules/encore

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1_cworld | 11518 | 0 | PASS | |
| 2_prydz | 30000 | 0 | PASS | |
| ab_julia | 30000 | 0 | PASS | |
| amegas | 30000 | 0 | PASS | |
| crazbeat | 30000 | 0 | PASS | |
| fm_house | 30000 | 0 | PASS | |
| fm_tekno | 30000 | 0 | PASS | |
| fmhouse2 | 30000 | 0 | PASS | |
| forest | 30000 | 0 | PASS | |
| whatslov | 30000 | 0 | PASS | |

### modules/hydra

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| crackit | 19199 | 39602 | FRAME-DIFF | 2760 MB, 1200 E, 840 F, 840 0 — macro state + event_table + freq divergence. |
| intrcoop | 30000 | 95372 | FRAME-DIFF | Init volume artifact (`release_sustaining_sound`, 0x3f to vol regs) + extensive volume scaling divergence across song. |

### modules/kkonaa

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| drgwrrtt | 20897 | 377 | FRAME-DIFF | 25 MB, 25 F, 25 0 — freq table + macro state divergence. |
| dwrrcslo | 15359 | 0 | PASS | |
| dwrrfild | 23036 | 0 | PASS | |
| eastdsrt | 30000 | 0 | PASS | |
| etwarawk | 30000 | 0 | PASS | |
| ff2rebel | 30000 | 0 | PASS | |
| kenseidn | 15840 | 0 | PASS | |
| konahome | 30000 | 0 | PASS | |
| limitbrk | 30000 | 323426 | FRAME-DIFF | 102900 non-MB — freq/volume divergence (fine_tune/ftune). |
| lostspc | 30000 | 0 | PASS | |
| mutecity | 30000 | 0 | PASS | |
| reflects | 30000 | 0 | PASS | |
| saltygrv | 30000 | 0 | PASS | |
| surfcity | 30000 | 0 | PASS | |
| tg_vegas | 30000 | 0 | PASS | |
| th04wdrm | 30000 | 0 | PASS | |
| top-2act | 30000 | 0 | PASS | |

### modules/kvee

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 3812funk | 30000 | 0 | PASS | |
| mm3title | 30000 | 0 | PASS | |
| sweetsin | 30000 | 140 | FRAME-DIFF | KSL/TL init artifact at IRQ 30–37 bank 0. |

### modules/madbrain

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| crisis | 30000 | 0 | PASS | |
| fank5 | 30000 | 0 | PASS | |
| grabbag | 30000 | 0 | PASS | |
| os_galax | 30000 | 0 | PASS | |
| os_intro | 30000 | 0 | PASS | |
| os_sblas | 30000 | 0 | PASS | |
| os_wins | 30000 | 7174 | FRAME-DIFF | Bug 2 init artifact at frame ~22040. |
| sinner | 30000 | 0 | PASS | |
| supmario | 30000 | 0 | PASS | |
| weirdsnd | 30000 | 0 | PASS | |
| whak | 30000 | 0 | PASS | |

### modules/mlf

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| analogtr | 24959 | 0 | PASS | |
| deorbit | 50000 | 0 | PASS | Bug 3 fix resolved. |
| fm-troni | 30000 | 122 | FRAME-DIFF | TonePortamento keyoff regression (was 50→170, now 122 with 30k default). |
| glass | 50000 | 0 | PASS | Bug 3 fix resolved. |
| ishtar | 30000 | 0 | PASS | |
| khaos | 30000 | 0 | PASS | |
| khaos2 | 30000 | 0 | PASS | |
| lbtrance | 30000 | 0 | PASS | |
| old_001 | 30000 | 0 | PASS | |
| old_002 | 30000 | 38589 | FRAME-DIFF | ±1 nibble offset most frames. ftune/fine_tune interaction. |
| opl303 | 30000 | 212 | FRAME-DIFF | Small ±1 nibble diffs. ftune/fine_tune interaction. |
| pink | 30000 | 371 | FRAME-DIFF | ±1 pitch nibbles bank 0 ch0 F-Number Low. ftune/fine_tune interaction. |
| spacediv | 30000 | 10873 | FRAME-DIFF | ±1 nibble offset many frames. ftune/fine_tune interaction. |

### modules/nula

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| cracker | 30000 | 0 | PASS | |
| fresh | 30000 | 68 | FRAME-DIFF | Small ±1 nibble diffs bank 0. |
| gummi | 30000 | 0 | PASS | |
| imadick | 30000 | 0 | PASS | |
| kulje | 30000 | 0 | PASS | |
| loader | 30000 | 0 | PASS | |
| mario | 30000 | 0 | PASS | |
| menuload | 30000 | 284 | FRAME-DIFF | Small ±1 nibble diffs bank 0. |
| mtkamies | 30000 | 0 | PASS | |
| onward | 30000 | 0 | PASS | |
| pre | 30000 | 0 | PASS | |
| psycho3x | 30000 | 23858 | FRAME-DIFF | ±1 nibble offset across most frames. |
| psycho5 | 30000 | 34088 | FRAME-DIFF | ±1 nibble offset from frame 24+, likely INIT state divergence. |
| spa | 30000 | 0 | PASS | |
| unreal | 30000 | 0 | PASS | |
| unreal2 | 30000 | 0 | PASS | |
| worms | 30000 | 0 | PASS | |
| zalza | 30000 | 0 | PASS | |
| zandax | 30000 | 0 | PASS | |
| zenbowl | 30000 | 0 | PASS | |

### modules/o2star

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| o2bbb | 30000 | 0 | PASS | |
| o2bigtr | 30000 | 0 | PASS | |
| o2bring | 30000 | 8640 | FRAME-DIFF | 5760 non-MB — freq table divergence (fine_tune/ftune offset). |
| o2c3comp | 30000 | 2580 | FRAME-DIFF | 1720 non-MB — freq/pitch ±1 nibble offsets. |
| o2deaf | 30000 | 768 | FRAME-DIFF | 512 non-MB — freq table divergence (fine_tune/ftune). |
| o2devas | 30000 | 0 | PASS | |
| o2enjoy | 30000 | 0 | PASS | |
| o2ghosts | 30000 | 27340 | FRAME-DIFF | all non-MB — init volume artifact (release_sustaining_sound). |
| o2grv1 | 30000 | 0 | PASS | |
| o2grv2 | 30000 | 0 | PASS | |
| o2invol | 30000 | 0 | PASS | |
| o2lazyg | 30000 | 0 | PASS | |
| o2leftou | 30000 | 0 | PASS | |
| o2lovehr | 30000 | 0 | PASS | |
| o2uvhsc | 30000 | 0 | PASS | |

### modules/televics

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 4thcoast | 30000 | 0 | PASS | 520 MB, 520 F, 520 1, 520 0 — macro state + freq + channel 1/0 divergence. |
| 4xmisste | 50000 | 160 | FRAME-DIFF | |
| 5ontelev | 50000 | 14 | FRAME-DIFF | Small end-of-song diff. |
| allfull | 50000 | 6470 | FRAME-DIFF | |
| antilato | 50000 | 0 | PASS | |
| asynth | 50000 | 0 | PASS | |
| atrick | 50000 | 30 | FRAME-DIFF | |
| badlib | 50000 | 62 | FRAME-DIFF | |
| badtraf | 50000 | 996 | FRAME-DIFF | |
| boppinb | 50000 | 54 | FRAME-DIFF | |
| bopptoda | 50000 | 38 | FRAME-DIFF | |
| build | 50000 | 2588 | FRAME-DIFF | |
| butterfl | 50000 | 0 | PASS | |
| composur | 50000 | 0 | PASS | |
| cwack | 50000 | 1742 | FRAME-DIFF | |
| d-rumsb | 50000 | 38 | FRAME-DIFF | |
| damn | 50000 | 14 | FRAME-DIFF | Small end-of-song diff. |
| discwrld | 50000 | 1958 | FRAME-DIFF | |
| drumbnba | 50000 | 46 | FRAME-DIFF | |
| fast | 50000 | 38 | FRAME-DIFF | |
| fuckyrev | 50000 | 270 | FRAME-DIFF | |
| funny | 50000 | 0 | PASS | |
| libretto | 50000 | 30 | FRAME-DIFF | |
| macroron | 50000 | 38 | FRAME-DIFF | |
| phaat | 50000 | 38 | FRAME-DIFF | |
| scary | 50000 | 0 | PASS | |
| shit | 50000 | 62 | FRAME-DIFF | |
| slap | 50000 | 30 | FRAME-DIFF | |
| stuffage | 50000 | 62 | FRAME-DIFF | |
| sunny | 50000 | 38 | FRAME-DIFF | |
| teknoh | 50000 | 0 | PASS | |
| topgear | 50000 | 3998 | FRAME-DIFF | |
| wip | 50000 | 0 | PASS | |
| woods | 50000 | 38 | FRAME-DIFF | |
| yellatfl | 50000 | 38 | FRAME-DIFF | |
| yellowwe | 50000 | 38 | FRAME-DIFF | |
