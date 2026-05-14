## Init Sequence Comparison

### Pascal (adt2_dump.pas → a2player.pas)

```
OPL3EMU_init                  ← chip reset, all regs = 0
init_songdata                 ← clear song info
init_timer_proc
load file (a2m_loader etc.)   ← sets songdata.common_flag etc.
─── start_playing ───
  stop_playing:
    release_sustaining_sound(1..20)  ← vol=63, key_on/off for ALL 20
    opl2out($bd, 0)
    opl3exp($0004), opl3exp($0005)
    init_buffers
  init_player:
    opl2out($01, 0)
    keyoff 18 channels via _chan_n
    clear ADSR $080..$08d, $090..$095
    ← sets percussion_mode, flag_4op from songdata.common_flag
    ← **sets _chan_n/_chan_m/_chan_c** based on percussion_mode (stale during stop!)
    opl2out($01, $20), opl2out($08, $40)
    opl3exp($0105), opl3exp($04 + flag_4op<<8)
    key_off(17), key_off(18)
    opl2out($bd, misc_register)
    global_volume = 63
    voice_table[i] = i, arpgg[i].state = 1
  set_current_order(0)
  play_status := isPlaying
set_overall_volume(63)        ← after frame_hook
```

### C (a2t.c)

```
a2t_init(freq)                ← OPL3_Reset → chip reset, all regs = 0
a2t_load(name)                ← load file into memory
─── a2t_play ───
  a2t_stop:
    release_sustaining_sound(0..19)  ← vol=63, key_on/off for ALL 20
    opl2out($bd, 0)
    opl3exp($0004), opl3exp($0005)
    init_buffers
  a2_import                         ← sets songdata, common_flag etc.
  init_player:
    opl2out($01, 0)
    keyoff 18 channels via regoffs_n
    clear ADSR $080..$08d, $090..$095
    ← percussion_mode, flag_4op etc. already set by a2_import
    opl2out($01, $20), opl2out($08, $40)
    opl3exp($0105), opl3exp($04 + flag_4op<<8)
    key_off(16), key_off(17)
    opl2out($bd, misc_register)
    **init_buffers**                  ← SECOND call (Pascal doesn't)
    global_volume = 63
    voice_table[i] = i+1, arpgg[i].state = 1
  set_current_order(0)
  play_status := isPlaying
set_overall_volume(63)        ← from a2m_dump.c main()
```

### Differences Found

| Aspect | Pascal | C | Impact |
|--------|--------|---|-------|
| `init_buffers` count | Once in `stop_playing` | **Twice** (stop + init_player) | Re-clears channel state after import — redundant, harmless |
| `_chan_n/m/c` / `regoffs` timing | Set in `init_player` AFTER `stop_playing` — **stale during stop** | Computed live via `!!percussion_mode` — **always current** | **Potential bug**: Pascal's `stop_playing` → `release_sustaining_sound` writes vol=63 using old `_chan_m` from PREVIOUS song. If prev song had different `percussion_mode`, it writes to wrong regs |
| `common_flag` parsing | In `init_player` | In `a2_import` (before `init_player`) | Same result, different location |
| `key_off` indices | 17,18 (1-based → C 16,17) | 16,17 (0-based) | **Same** (just 1-based vs 0-based) |
| `speed` / `update_timer` in stop | Not done | `speed=4; update_timer(50)` | C resets tempo/speed, Pascal doesn't |
