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
| KULJE_V4 | 30000 | 532481 | FRAME-DIFF | INIT state divergence. |
| MINDFLUX (VOID) | 10891 | 0 | PASS | |
| Newtune | 17279 | 0 | PASS | |
| RASTER (VOID) | 31999 | 0 | PASS | |
| TERRANIA (VOID) | 19199 | 0 | PASS | |
| adven | 30000 | 0 | PASS | Bug 3 fix resolved active playback (was 30 at 21k — 38 at 50k is end-of-song divergence). |
| andromeda | 30000 | 0 | PASS | |
| bxx_nowgone | 3357 | 0 | PASS | |
| class05 | 24959 | 0 | PASS | 6390 E-line diffs (event_table eff fields). |
| damn-sh | 30000 | 0 | PASS | |
| ed3lw | 23043 | 0 | PASS | |
| frustration | 23517 | 0 | PASS | |
| goa-cma | 30000 | 0 | PASS | |
| mystcave | 29947 | 0 | PASS | |
| nightdrv | 30000 | 5438 | FRAME-DIFF | 2430 PT, 570 MB, 540 F, 396 1, 144 0 — freq/volume/PT divergence. |
| paradox3 | 25091 | 0 | PASS | |
| remembrance | 30000 | 0 | PASS | |
| schwskel | 30000 | 0 | PASS | |
| skyh | 30000 | 56174 | FRAME-DIFF | 3840 0, 3744 FP, 3744 CV, 96 MB, 96 F — channel volume/FP/0-line divergence. |
| speed_reset_song103 | 2074 | 0 | PASS | |
| square | 19588 | 0 | PASS | |
| top-2act | 30000 | 0 | PASS | Re-checked 2026-05-13 (`MAX_FRAMES=100000`, `TIMEOUT_SEC=360`): **`diff -u`** empty. Root cause: instrument **19** had FMREG **`length==0`** and empty cells but **`src[5]`** selected vibrato table **1**; **`fmreg`** was not allocated so **`instrument->vibrato`** never propagated (**`src/a2t.c`** **`fmreg_table_allocate`** now allocates when **`src[1]|…|src[5]`** is non-zero). Cell inference when **`length==0`** retained. Optional **`tools/fmreg_peek.c`** can inspect FMREG blobs. |
| whereru | 30000 | 0 | PASS | |

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
| chiptune | 12799 | 0 | PASS | |
| chivalry | 30000 | 0 | PASS | Init artifact bank 1 KSL/TL. |
| cloudfrv | 17919 | 0 | PASS | |
| conspira | 14398 | 0 | PASS | |
| contrast | 23036 | 0 | PASS | |
| coolcat | 12956 | 0 | PASS | |
| cooldown | 10397 | 0 | PASS | |
| dayhcome | 9076 | 0 | PASS | |
| dayslive | 28799 | 0 | PASS | |
| deceptin | 30000 | 0 | PASS | |
| deepdown | 19560 | 0 | PASS | |
| depressn | 24197 | 0 | PASS | |
| difkhero | 11509 | 0 | PASS | |
| dream7m | 9599 | 0 | PASS | |
| dream7mx | 2076 | 0 | PASS | Small ±1 nibble offsets. |
| echorock | 6547 | 0 | PASS | |
| elskiing | 21120 | 0 | PASS | |
| enchloop | 1727 | 0 | PASS | |
| eviloop | 30000 | 0 | PASS | |
| fiftrial | 7768 | 0 | PASS | |
| frenzy | 22270 | 0 | PASS | |
| friendsh | 11877 | 0 | PASS | |
| frozen | 16796 | 0 | PASS | Init artifact bank 1 KSL/TL. |
| fullthr | 9026 | 0 | PASS | |
| funland | 15999 | 383 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| goldebel | 3199 | 383 | FRAME-DIFF | Init artifact bank 1 KSL/TL. |
| horizon | 13437 | 0 | PASS | |
| hovlane | 13122 | 0 | PASS | |
| hunter | 22801 | 0 | PASS | |
| inevitbl | 16680 | 144007 | FRAME-DIFF | Small ±1 nibble offsets. |
| invasion | 15999 | 0 | PASS | |
| jdaniels | 25599 | 0 | PASS | (not in modules/brendan/ — kept from earlier run.) |
| kelsey | 3199 | 0 | PASS | |
| laboite | 23036 | 0 | PASS | |
| lemmings | 15999 | 48008 | FRAME-DIFF | Small ±1 nibble offsets. |
| lostcaus | 22399 | 26007 | FRAME-DIFF | Init artifact ±1 nibble offsets. |
| lucky7s | 8396 | 0 | PASS | Init artifact ± freq diffs. |
| manifest | 17599 | 21283 | FRAME-DIFF | |
| marblect | 30000 | 0 | PASS | |
| mariothm | 893 | 0 | PASS | |
| mechage | 12575 | 0 | PASS | Init artifact bank 1 KSL/TL. |
| mechwar | 13437 | 0 | PASS | Fixed 2026-05-16: `reset_ins_volume`/`set_ins_volume` now handle NULL instrument gracefully (pattern instr index > song's instrument count). Bug 6. |
| metro | 24575 | 0 | PASS | |
| modratly | 22031 | 0 | PASS | |
| neversay | 15648 | 0 | PASS | |
| neurophb | 30000 | 47522 | FRAME-DIFF | |
| nochoice | 15999 | 0 | PASS | |
| nogofers | 12799 | 0 | PASS | |
| nowgone | 30000 | 0 | PASS | |
| oblivion | 28799 | 0 | PASS | |
| obstacle | 8832 | 0 | PASS | |
| ohirony | 11999 | 0 | PASS | |
| omegavir | 20044 | 0 | PASS | |
| paparazi | 22508 | 0 | PASS | |
| pfectwld | 30000 | 0 | PASS | |
| popcorn | 12177 | 0 | PASS | |
| racecyb | 23806 | 0 | PASS | |
| racecyb2 | 30000 | 0 | PASS | |
| recherch | 13827 | 0 | PASS | |
| robinthm | 16198 | 0 | PASS | |
| running | 30000 | 0 | PASS | |
| sinister | 13437 | 0 | PASS | Small ±1 nibble offsets. |
| sitcom | 10370 | 0 | PASS | |
| skylight | 17576 | 0 | PASS | |
| skysharp | 9597 | 0 | PASS | |
| sleepwrk | 17856 | 0 | PASS | |
| song100 | 5374 | 866 | FRAME-DIFF | ±1 nibble diffs bank 0, ftune/fine_tune interaction. |
| song102 | 6913 | 55197 | FRAME-DIFF | |
| song103 | 1917 | 0 | PASS | |
| song105 | 4796 | 0 | PASS | |
| song108 | 8642 | 0 | PASS | |
| sonic | 27356 | 36722 | FRAME-DIFF | |
| spaceple | 23227 | 24967 | FRAME-DIFF | Init artifact ± freq diffs. |
| sparkplg | 26881 | 0 | PASS | Init artifact bank 1 KSL/TL. |
| stormrid | 15999 | 0 | PASS | |
| stormywe | 14423 | 0 | PASS | |
| suspicis | 10755 | 0 | PASS | Init artifact bank 1 KSL/TL. |
| tanmusik | 19199 | 0 | PASS | |
| teamster | 17599 | 0 | PASS | |
| thinkfst | 18801 | 0 | PASS | |
| timewtel | 16797 | 0 | PASS | |
| torpdall | 13498 | 0 | PASS | Init artifact ± volume scaling. |
| trance | 19201 | 0 | PASS | |
| trance2 | 24960 | 0 | PASS | ±1 nibble offset across many frames, ftune/fine_tune interaction. |
| treacher | 19199 | 0 | PASS | |
| trouble | 30000 | 87482 | FRAME-DIFF | |
| tutheme | 12601 | 0 | PASS | |
| twistdpa | 21120 | 0 | PASS | |
| ultra | 9600 | 0 | PASS | |
| ultravio | 15650 | 0 | PASS | |
| verynice | 22378 | 0 | PASS | |
| village | 30000 | 0 | PASS | |
| warhouse | 7918 | 0 | PASS | |
| waterfls | 19201 | 0 | PASS | |
| wavesmar | 18000 | 0 | PASS | |
| westbeng | 17282 | 0 | PASS | |
| worldfal | 27840 | 0 | PASS | |

### modules/diodema

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1856step | 30000 | 0 | PASS | MB + 24 pre-existing non-MB (init volume artifact). |
| acidtrac | 30000 | 0 | PASS | |
| adr1ft | 21498 | 118703 | FRAME-DIFF | MB-only: `FINISHED` vs `IDLE` (keyoff_loop flag mismatch). |
| altair | 30000 | 0 | PASS | |
| aquarius | 30000 | 10802 | FRAME-DIFF | MB + 3024 pre-existing non-MB (±1 nibble offsets + volume artifact). |
| ca54 | 30000 | 0 | PASS | |
| catpeopl | 30000 | 0 | PASS | |
| milinda | 30000 | 0 | PASS | |
| mmori | 30000 | 0 | PASS | |
| mp77 | 30000 | 0 | PASS | |
| no72 | 30000 | 0 | PASS | |
| null | 12668 | 0 | PASS | MB + 1680 pre-existing non-MB (±1 nibble offsets). |
| oddtime | 30000 | 0 | PASS | |
| phone | 9955 | 0 | PASS | |
| psg | 30000 | 13610 | FRAME-DIFF | |
| rf62 | 30000 | 0 | PASS | |
| ru41 | 30000 | 0 | PASS | |
| samsara | 30000 | 0 | PASS | |
| signs | 30000 | 0 | PASS | MB + 600 pre-existing non-MB (±1 nibble offsets). |
| sv73 | 30000 | 6485 | FRAME-DIFF | |
| ty58 | 30000 | 0 | PASS | |
| xmission | 30000 | 186788 | FRAME-DIFF | |
| zaxxon | 30000 | 20738 | FRAME-DIFF | |

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
| rbfactry | 30000 | 0 | PASS | 60 MB, 60 F, 60 1 — freq table + macro state divergence. |
| undersea | 30000 | 0 | PASS | |

### modules/encore

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1_cworld | 11518 | 0 | PASS | |
| 2_prydz | 1919 | 0 | PASS | |
| ab_julia | 30000 | 0 | PASS | |
| amegas | 30000 | 0 | PASS | Fixed 2026-05-17: `ef_SetInsVolume`/`ef_ForceInsVolume` now check `is_data_empty()` on instrument data (matching Pascal's guard). Previously C processed volume effects for empty instrument slots while Pascal skipped them. |
| crazbeat | 30000 | 0 | PASS | |
| fm_house | 30000 | 0 | PASS | |
| fm_tekno | 26482 | 0 | PASS | |
| fmhouse2 | 30000 | 0 | PASS | |
| forest | 30000 | 0 | PASS | |
| whatslov | 30000 | 0 | PASS | |

### modules/hydra

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| crackit | 19199 | 3782 | FRAME-DIFF | 2760 MB, 1200 E, 840 F, 840 0 — macro state + event_table + freq divergence. |
| intrcoop | 30000 | 12602 | FRAME-DIFF | Init volume artifact (`release_sustaining_sound`, 0x3f to vol regs) + extensive volume scaling divergence across song. |

### modules/kkonaa

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| drgwrrtt | 20897 | 352 | FRAME-DIFF | 25 MB, 25 F, 25 0 — freq table + macro state divergence. |
| dwrrcslo | 15359 | 0 | PASS | |
| dwrrfild | 23036 | 0 | PASS | |
| eastdsrt | 30000 | 0 | PASS | |
| etwarawk | 30000 | 0 | PASS | |
| ff2rebel | 30000 | 0 | PASS | |
| kenseidn | 15840 | 0 | PASS | |
| konahome | 30000 | 0 | PASS | |
| limitbrk | 30000 | 310177 | FRAME-DIFF | 102900 non-MB — freq/volume divergence (fine_tune/ftune). |
| lostspc | 30000 | 0 | PASS | |
| mutecity | 16781 | 0 | PASS | |
| reflects | 19198 | 0 | PASS | |
| saltygrv | 30000 | 0 | PASS | |
| surfcity | 19797 | 0 | PASS | |
| tg_vegas | 30000 | 0 | PASS | |
| th04wdrm | 30000 | 0 | PASS | |
| top-2act | 30000 | 0 | PASS | |

### modules/kvee

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 3812funk | 30000 | 0 | PASS | |
| mm3title | 17596 | 0 | PASS | |
| sweetsin | 30000 | 1126 | FRAME-DIFF | KSL/TL init artifact at IRQ 30–37 bank 0. |

### modules/madbrain

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| crisis | 30000 | 0 | PASS | |
| fank5 | 30000 | 0 | PASS | |
| grabbag | 30000 | 0 | PASS | |
| os_galax | 30000 | 0 | PASS | |
| os_intro | 30000 | 0 | PASS | |
| os_sblas | 30000 | 0 | PASS | |
| os_wins | 30000 | 0 | PASS | Bug 2 init artifact at frame ~22040. |
| sinner | 13534 | 0 | PASS | |
| supmario | 14335 | 0 | PASS | |
| weirdsnd | 30000 | 0 | PASS | |
| whak | 30000 | 0 | PASS | |

### modules/mlf

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| analogtr | 24959 | 0 | PASS | |
| deorbit | 30000 | 0 | PASS | Bug 3 fix resolved. |
| fm-troni | 30000 | 542 | FRAME-DIFF | TonePortamento keyoff regression (was 50→170, now 122 with 30k default). |
| glass | 30000 | 193103 | FRAME-DIFF | Bug 3 fix resolved. |
| ishtar | 30000 | 286046 | FRAME-DIFF | |
| khaos | 30000 | 0 | PASS | |
| khaos2 | 30000 | 0 | PASS | |
| lbtrance | 30000 | 0 | PASS | |
| old_001 | 24959 | 0 | PASS | |
| old_002 | 25599 | 185144 | FRAME-DIFF | ±1 nibble offset most frames. ftune/fine_tune interaction. |
| opl303 | 30000 | 1058 | FRAME-DIFF | Small ±1 nibble diffs. ftune/fine_tune interaction. |
| pink | 24360 | 2297 | FRAME-DIFF | ±1 pitch nibbles bank 0 ch0 F-Number Low. ftune/fine_tune interaction. |
| spacediv | 30000 | 50287 | FRAME-DIFF | ±1 nibble offset many frames. ftune/fine_tune interaction. |

### modules/nula

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| cracker | 30000 | 0 | PASS | |
| fresh | 17291 | 327 | FRAME-DIFF | Small ±1 nibble diffs bank 0. |
| gummi | 6405 | 0 | PASS | |
| imadick | 9599 | 0 | PASS | |
| kulje | 14079 | 0 | PASS | |
| loader | 15377 | 0 | PASS | |
| mario | 11531 | 0 | PASS | |
| menuload | 13439 | 0 | PASS | Small ±1 nibble diffs bank 0. |
| mtkamies | 23732 | 0 | PASS | |
| onward | 26879 | 0 | PASS | |
| pre | 29759 | 0 | PASS | |
| psycho3x | 12805 | 130483 | FRAME-DIFF | ±1 nibble offset across most frames. |
| psycho5 | 12805 | 168586 | FRAME-DIFF | ±1 nibble offset from frame 24+, likely INIT state divergence. |
| spa | 7691 | 0 | PASS | |
| unreal | 6551 | 0 | PASS | |
| unreal2 | 12805 | 0 | PASS | |
| worms | 3839 | 0 | PASS | |
| zalza | 2891 | 0 | PASS | |
| zandax | 13119 | 0 | PASS | |
| zenbowl | 12002 | 0 | PASS | |

### modules/o2star

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| o2bbb | 30000 | 0 | PASS | |
| o2bigtr | 30000 | 0 | PASS | |
| o2bring | 30000 | 0 | PASS | 5760 non-MB — freq table divergence (fine_tune/ftune offset). |
| o2c3comp | 17547 | 0 | PASS | 1720 non-MB — freq/pitch ±1 nibble offsets. |
| o2deaf | 19455 | 0 | PASS | 512 non-MB — freq table divergence (fine_tune/ftune). |
| o2devas | 30000 | 0 | PASS | |
| o2enjoy | 30000 | 0 | PASS | |
| o2ghosts | 30000 | 0 | PASS | all non-MB — init volume artifact (release_sustaining_sound). |
| o2grv1 | 30000 | 0 | PASS | |
| o2grv2 | 18431 | 0 | PASS | |
| o2invol | 30000 | 0 | PASS | |
| o2lazyg | 30000 | 0 | PASS | |
| o2leftou | 30000 | 0 | PASS | |
| o2lovehr | 30000 | 0 | PASS | |
| o2uvhsc | 30000 | 0 | PASS | |

### modules/televics

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 4thcoast | 30000 | 0 | PASS | 520 MB, 520 F, 520 1, 520 0 — macro state + freq + channel 1/0 divergence. |
| 4xmisste | 30000 | 228524 | FRAME-DIFF | |
| 5ontelev | 30000 | 20738 | FRAME-DIFF | Small end-of-song diff. |
| allfull | 30000 | 0 | PASS | |
| antilato | 30000 | 0 | PASS | |
| asynth | 30000 | 0 | PASS | |
| atrick | 24575 | 0 | PASS | |
| badlib | 30000 | 0 | PASS | |
| badtraf | 30000 | 0 | PASS | |
| boppinb | 26881 | 0 | PASS | |
| bopptoda | 15354 | 0 | PASS | |
| build | 30000 | 0 | PASS | |
| butterfl | 30000 | 0 | PASS | |
| composur | 30000 | 0 | PASS | |
| cwack | 30000 | 0 | PASS | |
| d-rumsb | 30000 | 0 | PASS | |
| damn | 30000 | 0 | PASS | Small end-of-song diff. |
| discwrld | 30000 | 0 | PASS | |
| drumbnba | 30000 | 0 | PASS | |
| fast | 30000 | 0 | PASS | |
| fuckyrev | 30000 | 0 | PASS | |
| funny | 30000 | 0 | PASS | |
| libretto | 16383 | 0 | PASS | |
| macroron | 30000 | 0 | PASS | |
| phaat | 15359 | 0 | PASS | |
| scary | 30000 | 0 | PASS | |
| shit | 30000 | 0 | PASS | |
| slap | 27639 | 0 | PASS | |
| stuffage | 26115 | 0 | PASS | |
| sunny | 30000 | 0 | PASS | |
| teknoh | 30000 | 0 | PASS | |
| topgear | 30000 | 0 | PASS | |
| wip | 30000 | 0 | PASS | |
| woods | 15359 | 0 | PASS | |
| yellatfl | 26879 | 0 | PASS | |
| yellowwe | 30000 | 272 | FRAME-DIFF | |
