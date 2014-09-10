{$IFNDEF FPC}
=== Compile with FPC compiler only ===
{$ENDIF}

/////////////////////////////////////////////////////
//                                                 //
//  This unit replays tiny modules                 //
//  created with /´DLiB TR/´CK3R ][ G3 REV.36      //
//                                                 //
//  system requirements:                           //
//  . 100k free conventional memory                //
//  . 2MB+ free extended memory (recommended 4MB+) //
//  . Yamaha YMF262 / OPL3 (compatible) sound card //
//  . MS-DOS 3.0 (compatible) operating system     //
//                                                 //
/////////////////////////////////////////////////////
//                                                 //
//  changes by Da!NyL                              //
//    - removed TMT-Pascal & IRQ-Stuff             //
//    - changed assembly procs to pascal procs     //
//    - probably something else                    //
//                                                 //
/////////////////////////////////////////////////////

unit A2replay;

interface

const
  opl3port: Word = $388;
  error_code: Integer = 0;
  current_order: Byte = 0;
  current_pattern: Byte = 0;
  current_line: Byte = 0;
  tempo: Byte = 50;
  speed: Byte = 6;
  macrospeedup: Word = 1;
  irq_mode: Boolean = FALSE;

procedure start_playing(var data; size: Word);
procedure poll_proc;
procedure macro_poll_proc;
procedure set_overall_volume(level: Byte);
procedure stop_playing;
type
  topl_out=procedure(port,val:word);
var
  opl_out:topl_out;

implementation

{$IFDEF FPC}
  {$ASMMODE INTEL}
  {$PACKRECORDS 1}
{$ENDIF}

type
  tFM_INST_DATA = Record
                    AM_VIB_EG_modulator,
                    AM_VIB_EG_carrier,
                    KSL_VOLUM_modulator,
                    KSL_VOLUM_carrier,
                    ATTCK_DEC_modulator,
                    ATTCK_DEC_carrier,
                    SUSTN_REL_modulator,
                    SUSTN_REL_carrier,
                    WAVEFORM_modulator,
                    WAVEFORM_carrier,
                    FEEDBACK_FM: Byte;
                  end;
type
  tADTRACK2_INS = Record
                    fm_data: tFM_INST_DATA;
                    panning: Byte;
                    fine_tune: Shortint;
                    perc_voice: Byte;
                  end;
type
  tARPEGGIO_TABLE = Record
                      length,
                      speed,
                      loop_begin,
                      loop_length,
                      keyoff_pos: Byte;
                      data: array[1..255] of Byte;
                    end;
type
  tVIBRATO_TABLE = Record
                     length,
                     speed,
                     delay,
                     loop_begin,
                     loop_length,
                     keyoff_pos: Byte;
                     data: array[1..255] of Shortint;
                   end;
type
  tREGISTER_TABLE_DEF = Record
                          fm_data: tFM_INST_DATA;
//                        freq_slide: Integer;
                          freq_slide: Smallint;
                          panning: Byte;
                          duration: Byte;
                        end;
type
  tREGISTER_TABLE = Record
                      length,
                      loop_begin,
                      loop_length,
                      keyoff_pos,
                      arpeggio_table,
                      vibrato_table: Byte;
                      data: array[1..255] of tREGISTER_TABLE_DEF;
                    end;
type
  tMACRO_TABLE = Record
                   arpeggio: tARPEGGIO_TABLE;
                   vibrato:  tVIBRATO_TABLE;
                 end;
type
  tFM_PARAMETER_TABLE = Record
                          adsrw_car,
                          adsrw_mod: Record
                                       attck,dec,sustn,rel,
                                       wform: Byte;
                                     end;
                          connect,
                          feedb,
                          multipM,kslM,tremM,vibrM,ksrM,sustM,
                          multipC,kslC,tremC,vibrC,ksrC,sustC: Byte;
                        end;
type
  tADTRACK2_EVENT = Record
                      note,
                      instr_def,
                      effect_def,
                      effect,
                      effect_def2,
                      effect2: Byte;
                    end;
type
  tDIS_FMREG_COL = array[0..27] of Boolean;

type
  tFIXED_SONGDATA = Record
                      instr_data:    array[1..255] of tADTRACK2_INS;
                      instr_macros:  array[1..255] of tREGISTER_TABLE;
                      macro_table:   array[1..255] of tMACRO_TABLE;
                      pattern_order: array[0..$7f] of Byte;
                      tempo:         Byte;
                      speed:         Byte;
                      flags:   Byte;
                      pattlen:      Word;
                      noftracks:     Byte;
                      macrospeedup: Word;
                      op4flags:      Byte;
                      lockflags:    array[1..20]  of Byte;
                      dis_fmreg_col: array[1..255] of tDIS_FMREG_COL;
                    end;
type
  tPLAY_STATUS = (isPlaying,isPaused,isStopped);

type
  tVARIABLE_DATA = array[0..7]    of
                   array[1..20]   of
                   array[0..$0ff] of tADTRACK2_EVENT;
type
  tPATTERN_DATA = array[0..15] of tVARIABLE_DATA;

type
  tDUMMY_BUFF = array[0..PRED(655350)] of Byte;

const
  _panning: array[0..2] of Byte = ($30,$10,$20);

const
  _instr:  array[0..11] of Byte = ($20, $20,
                                   $40, $40,
                                   $60, $60,
                                   $80, $80,
                                   $0e0,$0e0,
                                   $0c0,
                                   $0bd);
type
  tTRACK_ADDR = array[1..20] of Word;

const
  INSTRUMENT_SIZE = SizeOf(tADTRACK2_INS);
  CHUNK_SIZE = SizeOf(tADTRACK2_EVENT);
  PATTERN_SIZE = 20*256*CHUNK_SIZE;
  NONE = $0ff;

const                    { 01 - 02 - 03 - 04 - 05 - 06 - 07 - 08 - 09 - 10 - 11 - 12 - 13 - 14 - 15 - 16 - 17 - 18 - 19 - 20 }
  _chmm_n: tTRACK_ADDR = ($003,$000,$004,$001,$005,$002,$006,$007,$008,$103,$100,$104,$101,$105,$102,$106,$107,$108,NONE,NONE);
  _chmm_m: tTRACK_ADDR = ($008,$000,$009,$001,$00a,$002,$010,$011,$012,$108,$100,$109,$101,$10a,$102,$110,$111,$112,NONE,NONE);
  _chmm_c: tTRACK_ADDR = ($00b,$003,$00c,$004,$00d,$005,$013,$014,$015,$10b,$103,$10c,$104,$10d,$105,$113,$114,$115,NONE,NONE);
                         {                                                                            BD   SD   TT   TC   HH }
  _chpm_n: tTRACK_ADDR = ($003,$000,$004,$001,$005,$002,$106,$107,$108,$103,$100,$104,$101,$105,$102,$006,$007,$008,$008,$007);
  _chpm_m: tTRACK_ADDR = ($008,$000,$009,$001,$00a,$002,$110,$111,$112,$108,$100,$109,$101,$10a,$102,$010,$014,$012,$015,$011);
  _chpm_c: tTRACK_ADDR = ($00b,$003,$00c,$004,$00d,$005,$113,$114,$115,$10b,$103,$10c,$104,$10d,$105,$013,NONE,NONE,NONE,NONE);

var
  _chan_n: tTRACK_ADDR;
  _chan_m: tTRACK_ADDR;
  _chan_c: tTRACK_ADDR;

const
  ef_Arpeggio          = 0;
  ef_FSlideUp          = 1;
  ef_FSlideDown        = 2;
  ef_TonePortamento    = 3;
  ef_Vibrato           = 4;
  ef_TPortamVolSlide   = 5;
  ef_VibratoVolSlide   = 6;
  ef_FSlideUpFine      = 7;
  ef_FSlideDownFine    = 8;
  ef_SetModulatorVol   = 9;
  ef_VolSlide          = 10;
  ef_PositionJump      = 11;
  ef_SetInsVolume      = 12;
  ef_PatternBreak      = 13;
  ef_SetTempo          = 14;
  ef_SetSpeed          = 15;
  ef_TPortamVSlideFine = 16;
  ef_VibratoVSlideFine = 17;
  ef_SetCarrierVol     = 18;
  ef_SetWaveform       = 19;
  ef_VolSlideFine      = 20;
  ef_RetrigNote        = 21;
  ef_Tremolo           = 22;
  ef_Tremor            = 23;
  ef_ArpggVSlide       = 24;
  ef_ArpggVSlideFine   = 25;
  ef_MultiRetrigNote   = 26;
  ef_FSlideUpVSlide    = 27;
  ef_FSlideDownVSlide  = 28;
  ef_FSlUpFineVSlide   = 29;
  ef_FSlDownFineVSlide = 30;
  ef_FSlUpVSlF         = 31;
  ef_FSlDownVSlF       = 32;
  ef_FSlUpFineVSlF     = 33;
  ef_FSlDownFineVSlF   = 34;
  ef_Extended          = 35;
  ef_Extended2         = 36;
  ef_SetGlobalVolume   = 37;
  ef_SwapArpeggio      = 38;
  ef_SwapVibrato       = 39;
  ef_ForceInsVolume    = 40;
  ef_Extended3         = 41;
  ef_ExtraFineArpeggio = 42;
  ef_ExtraFineVibrato  = 43;
  ef_ExtraFineTremolo  = 44;
  ef_ex_SetTremDepth   = 0;
  ef_ex_SetVibDepth    = 1;
  ef_ex_SetAttckRateM  = 2;
  ef_ex_SetDecayRateM  = 3;
  ef_ex_SetSustnLevelM = 4;
  ef_ex_SetRelRateM    = 5;
  ef_ex_SetAttckRateC  = 6;
  ef_ex_SetDecayRateC  = 7;
  ef_ex_SetSustnLevelC = 8;
  ef_ex_SetRelRateC    = 9;
  ef_ex_SetFeedback    = 10;
  ef_ex_SetPanningPos  = 11;
  ef_ex_PatternLoop    = 12;
  ef_ex_PatternLoopRec = 13;
  ef_ex_MacroKOffLoop  = 14;
  ef_ex_ExtendedCmd    = 15;
  ef_ex_cmd_RSS        = 0;
  ef_ex_cmd_ResetVol   = 1;
  ef_ex_cmd_LockVol    = 2;
  ef_ex_cmd_UnlockVol  = 3;
  ef_ex_cmd_LockVP     = 4;
  ef_ex_cmd_UnlockVP   = 5;
  ef_ex_cmd_VSlide_mod = 6;
  ef_ex_cmd_VSlide_car = 7;
  ef_ex_cmd_VSlide_def = 8;
  ef_ex_cmd_LockPan    = 9;
  ef_ex_cmd_UnlockPan  = 10;
  ef_ex_cmd_VibrOff    = 11;
  ef_ex_cmd_TremOff    = 12;
  ef_ex_cmd_FineVibr   = 13;
  ef_ex_cmd_FineTrem   = 14;
  ef_ex_cmd_NoRestart  = 15;
  ef_ex2_PatDelayFrame = 0;
  ef_ex2_PatDelayRow   = 1;
  ef_ex2_NoteDelay     = 2;
  ef_ex2_NoteCut       = 3;
  ef_ex2_FineTuneUp    = 4;
  ef_ex2_FineTuneDown  = 5;
  ef_ex2_GlVolSlideUp  = 6;
  ef_ex2_GlVolSlideDn  = 7;
  ef_ex2_GlVolSlideUpF = 8;
  ef_ex2_GlVolSlideDnF = 9;
  ef_ex2_GlVolSldUpXF  = 10;
  ef_ex2_GlVolSldDnXF  = 11;
  ef_ex2_VolSlideUpXF  = 12;
  ef_ex2_VolSlideDnXF  = 13;
  ef_ex2_FreqSlideUpXF = 14;
  ef_ex2_FreqSlideDnXF = 15;
  ef_ex3_SetConnection = 0;
  ef_ex3_SetMultipM    = 1;
  ef_ex3_SetKslM       = 2;
  ef_ex3_SetTremoloM   = 3;
  ef_ex3_SetVibratoM   = 4;
  ef_ex3_SetKsrM       = 5;
  ef_ex3_SetSustainM   = 6;
  ef_ex3_SetMultipC    = 7;
  ef_ex3_SetKslC       = 8;
  ef_ex3_SetTremoloC   = 9;
  ef_ex3_SetVibratoC   = 10;
  ef_ex3_SetKsrC       = 11;
  ef_ex3_SetSustainC   = 12;

const
  ef_fix1 = $80;
  ef_fix2 = $90;

const
  irq_freq: Word = 50;
  irq_initialized: Boolean = FALSE;
  timer_fix: Boolean = TRUE;
  pattern_break: Boolean = FALSE;
  pattern_delay: Boolean = FALSE;
  next_line: Byte = 0;
  max_patterns: Byte = 128;
  play_status: tPLAY_STATUS = isStopped;
  overall_volume: Byte = 63;
  global_volume: Byte = 63;

const
  keyoff_flag        = $080;
  fixed_note_flag    = $090;
  pattern_loop_flag  = $0e0;
  pattern_break_flag = $0f0;

var
  fmpar_table:   array[1..20] of tFM_PARAMETER_TABLE;
  volume_lock:   array[1..20] of Boolean;
  volume_table:  array[1..20] of Word;
  vscale_table:  array[1..20] of Word;
  peak_lock:     array[1..20] of Boolean;
  pan_lock:      array[1..20] of Boolean;
  modulator_vol: array[1..20] of Byte;
  carrier_vol:   array[1..20] of Byte;
  event_table:   array[1..20] of tADTRACK2_EVENT;
  voice_table:   array[1..20] of Byte;
  freq_table:    array[1..20] of Word;
  effect_table:  array[1..20] of Word;
  effect_table2: array[1..20] of Word;
  fslide_table:  array[1..20] of Byte;
  fslide_table2: array[1..20] of Byte;
  porta_table:   array[1..20] of Record freq: Word; speed: Byte; end;
  porta_table2:  array[1..20] of Record freq: Word; speed: Byte; end;
  arpgg_table:   array[1..20] of Record state,note,add1,add2: Byte; end;
  arpgg_table2:  array[1..20] of Record state,note,add1,add2: Byte; end;
  vibr_table:    array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
  vibr_table2:   array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
  trem_table:    array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
  trem_table2:   array[1..20] of Record pos,speed,depth: Byte; fine: Boolean; end;
  retrig_table:  array[1..20] of Byte;
  retrig_table2: array[1..20] of Byte;
  tremor_table:  array[1..20] of Record pos: Integer; volume: Word; end;
  tremor_table2: array[1..20] of Record pos: Integer; volume: Word; end;
  panning_table: array[1..20] of Byte;
  last_effect:   array[1..20] of Word;
  last_effect2:  array[1..20] of Word;
  volslide_type: array[1..20] of Byte;
  event_new:     array[1..20] of Boolean;
  notedel_table: array[1..20] of Byte;
  notecut_table: array[1..20] of Byte;
  ftune_table:   array[1..20] of Shortint;
  keyoff_loop:   array[1..20] of Boolean;
  macro_table:   array[1..20] of Record
                                   fmreg_pos,arpg_pos,vib_pos: Word;
                                   fmreg_count,fmreg_duration,arpg_count,
                                   vib_count,vib_delay: Byte;
                                   fmreg_table,arpg_table,vib_table: Byte;
                                   arpg_note: Byte;
                                   vib_freq: Word;
                                 end;

  loopbck_table: array[1..20] of Byte;
  loop_table:    array[1..20,0..255] of Byte;
  misc_register: Byte;

const
  current_tremolo_depth: Byte = 0;
  current_vibrato_depth: Byte = 0;

var
  speed_update,lockvol,panlock,lockVP: Boolean;
  tremolo_depth,vibrato_depth: Byte;
  volume_scaling,percussion_mode: Boolean;
  last_order: Byte;
  reset_chan: array[1..20] of Boolean;

var
  pattdata: ^tPATTERN_DATA;
  songdata: tFIXED_SONGDATA;
  CRC32_table: array[Byte] of Longint;

var
  ticks,tick0,tickD,
  tickXF: Longint;

procedure opl2out(reg,data: Word);inline;
begin
  opl_out(0,reg);
  opl_out(1,data);
end;

procedure opl3out(reg,data: Word);inline;
begin
  if reg <$100 then
  begin
    opl_out(0,reg);
    opl_out(1,data);
  end else
  begin
    opl_out(2,reg and $ff);
    opl_out(3,data);
  end;
end;

procedure opl3exp(data: Word);inline;
begin
  opl_out(2,data and $ff);
  opl_out(3,data shr 8);
end;

function nul_data(var data; size: Word): Boolean;
begin
asm
        xor     ecx,ecx
        mov     esi,[data]
        mov     cx,size
@@1:    lodsb
        cmp     al,0
        jnz     @@2
        loop    @@1
        mov     al,TRUE
        jmp     @@3
@@2:    mov     al,FALSE
@@3:    mov     @result,al
end ['esi','ecx','eax'];
end;

const
  FreqStart = $156;
  FreqEnd   = $2ae;
  FreqRange = FreqEnd-FreqStart;

function nFreq(note: Byte): Word; 

const
  Fnum: array[0..12] of Word = ($156,$16b,$181,$198,$1b0,$1ca,$1e5,
                                $202,$220,$241,$263,$287,$2ae);
begin
asm
        xor     ebx,ebx
        mov     al,note
        xor     ah,ah
        cmp     ax,12*8
        jae     @@1
        push    eax
        mov     bl,12
        div     bl
        mov     bl,ah
        xor     bh,bh
        shl     bx,1
        pop     eax
        mov     cl,12
        div     cl
        xor     ah,ah
        shl     ax,10
        add     ax,word ptr [Fnum+ebx]
        jmp     @@2
@@1:    mov     ax,7
        shl     ax,10
        add     ax,FreqEnd
@@2:    mov @result,ax
end ['eax','ebx','ecx'];
end;

function calc_freq_shift_up(freq,shift: Word): Word; 
begin
asm
        mov     cx,freq
        mov     ax,shift
        mov     bx,cx
        and     bx,0000001111111111b
        mov     dx,cx
        and     dx,0001110000000000b
        add     bx,ax
        and     cx,1110000000000000b
        shr     dx,10
        cmp     bx,FreqEnd
        jb      @@2
        cmp     dx,7
        jnz     @@1
        mov     bx,FreqEnd
        jmp     @@2
@@1:    sub     bx,FreqRange
        inc     dx
@@2:    mov     ax,cx
        shl     dx,10
        add     ax,dx
        add     ax,bx
        mov     @result,ax
end ['eax','ebx','ecx','edx'];
end ;

function calc_freq_shift_down(freq,shift: Word): Word; 
begin
asm
        mov     cx,freq
        mov     ax,shift
        mov     bx,cx
        and     bx,0000001111111111b
        mov     dx,cx
        and     dx,0001110000000000b
        sub     bx,ax
        and     cx,1110000000000000b
        shr     dx,10
        cmp     bx,FreqStart
        ja      @@2
        or      dx,dx
        jnz     @@1
        mov     bx,FreqStart
        jmp     @@2
@@1:    add     bx,FreqRange
        dec     dx
@@2:    mov     ax,cx
        shl     dx,10
        add     ax,dx
        add     ax,bx
        mov     @result,ax
end ['eax','ebx','ecx','edx'];
end;

function calc_vibrato_shift(depth,position: Byte): Word;
const
  vibr: array[0..31] of Byte = (
    0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
    253,250,244,235,224,212,197,180,161,141,120,97,74,49,24);
begin
asm
        xor     ebx,ebx
        mov     al,depth
        xor     ah,ah
        mov     bl,position
        xor     bh,bh
        mov     dh,bl
        and     bx,1fh
        mov     dl,byte ptr [vibr+ebx]
        mul     dl
        rol     ax,1
        xchg    ah,al
        and     ah,1
        shl     ax,1
        test    dh,32
        je      @@1
        inc     ax
        jmp     @@1
@@1:    mov     @result,ax
end ['eax','ebx','ecx','edx']
end;

procedure change_freq(chan: Byte; freq: Word);
begin
  freq_table[chan] := (freq AND $1fff)+
                      (freq_table[chan] AND NOT $1fff);
  opl3out($0a0+_chan_n[chan],LO(freq_table[chan]));
  opl3out($0b0+_chan_n[chan],HI(freq_table[chan]));
end;

function ins_parameter(ins,param: Byte): Byte; 
begin
asm
        xor     ebx,ebx
        lea     esi,[songdata]
        mov     bl,ins
        dec     ebx
        mov     eax,INSTRUMENT_SIZE
        mul     ebx
        add     esi,eax
        mov     bl,param
        add     esi,ebx
        lodsb
        mov     @result,al
end ['eax','ebx','esi'];
end;


function max(value: Word; maximum: Word): Word; 
begin
asm
        mov     ax,value
        cmp     ax,maximum
        jbe     @@1
        mov     ax,maximum
@@1:    mov     @result,ax
end ['eax'];
end;

function concw(lo,hi: Byte): Word; 
begin
asm
        mov     al,lo
        mov     ah,hi
        mov     @result,ax
end ['eax'];
end;

procedure change_frequency(chan: Byte; freq: Word);
begin
  change_freq(chan,freq);
  macro_table[chan].vib_freq := freq;
end;

function _macro_speedup: Word;
begin
asm
        mov     ax,macrospeedup
        or      ax,ax
        jnz     @@1
        inc     ax
@@1:    mov     @result,ax
end ['eax'];
end;

procedure set_clock_rate(clock_rate: Word);
begin
//  If NOT irq_mode then EXIT;
end;

procedure update_timer(Hz: Longint);
begin
  If (Hz = 0) then begin set_clock_rate(0); EXIT end
  else tempo := Hz;
  If (tempo = 18) and timer_fix then IRQ_freq := TRUNC((tempo+0.2)*20)
  else IRQ_freq := 250;
  While (IRQ_freq MOD (tempo*_macro_speedup) <> 0) do Inc(IRQ_freq);
  set_clock_rate(1193180 DIV IRQ_freq);
end;

procedure key_off(chan: Byte);
begin
  freq_table[chan] := LO(freq_table[chan])+
                     (HI(freq_table[chan]) AND NOT $20) SHL 8;
  change_frequency(chan,freq_table[chan]);
  event_table[chan].note := event_table[chan].note OR keyoff_flag;
end;

procedure release_sustaining_sound(chan: Byte);
begin
  opl3out(_instr[02]+_chan_m[chan],63);
  opl3out(_instr[03]+_chan_c[chan],63);

  FillChar(fmpar_table[chan].adsrw_car,
           SizeOf(fmpar_table[chan].adsrw_car),0);
  FillChar(fmpar_table[chan].adsrw_mod,
           SizeOf(fmpar_table[chan].adsrw_mod),0);

  opl3out($0b0+_chan_n[chan],0);
  opl3out(_instr[04]+_chan_m[chan],NONE);
  opl3out(_instr[05]+_chan_c[chan],NONE);
  opl3out(_instr[06]+_chan_m[chan],NONE);
  opl3out(_instr[07]+_chan_c[chan],NONE);

  key_off(chan);
  event_table[chan].instr_def := 0;
  reset_chan[chan] := TRUE;
end;

function scale_volume(volume,scale_factor: Byte): Byte;
begin
  scale_volume := 63-Round((63-volume)/63*
                           (63-scale_factor));
end;

procedure set_ins_volume(modulator,carrier,chan: Byte);

var
  temp: Byte;

begin
  If (modulator <> NONE) then
    begin
      temp := modulator;
      If volume_scaling then
        If (ins_parameter(voice_table[chan],10) AND 1 = 1) or
           (percussion_mode and (chan in [17..20])) then
          modulator := scale_volume(ins_parameter(voice_table[chan],2) AND $3f,modulator);
      If (ins_parameter(voice_table[chan],10) AND 1 = 1) or
         (percussion_mode and (chan in [17..20])) then
        opl3out(_instr[02]+_chan_m[chan],
                scale_volume(scale_volume(modulator,63-global_volume),63-overall_volume)+LO(vscale_table[chan]))
      else
        opl3out(_instr[02]+_chan_m[chan],
                temp+LO(vscale_table[chan]));
      volume_table[chan] := concw(temp,HI(volume_table[chan]));
      If (ins_parameter(voice_table[chan],10) AND 1 = 1) or
         (percussion_mode and (chan in [17..20])) then
        modulator_vol[chan] := 63-scale_volume(modulator,63-global_volume)
      else modulator_vol[chan] := 63-modulator;
    end;

  If (carrier <> NONE) then
    begin
      temp := carrier;
      If volume_scaling then
        carrier := scale_volume(ins_parameter(voice_table[chan],3) AND $3f,carrier);
      opl3out(_instr[03]+_chan_c[chan],
              scale_volume(scale_volume(carrier,63-global_volume),63-overall_volume)+HI(vscale_table[chan]));
      volume_table[chan] := concw(LO(volume_table[chan]),temp);
      carrier_vol[chan] := 63-scale_volume(carrier,63-global_volume);
    end;
end;

procedure reset_ins_volume(chan: Byte);
begin
  If NOT volume_scaling then
    set_ins_volume(ins_parameter(voice_table[chan],2) AND $3f,
                   ins_parameter(voice_table[chan],3) AND $3f,chan)
  else If (ins_parameter(voice_table[chan],10) AND 1 = 0) then
         set_ins_volume(ins_parameter(voice_table[chan],2) AND $3f,0,chan)
       else set_ins_volume(0,0,chan);
end;

procedure set_global_volume;

var
  chan: Byte;

begin
  For chan := 1 to 20 do
    If NOT ((carrier_vol[chan] = 0) and
            (modulator_vol[chan] = 0)) then
      If (ins_parameter(voice_table[chan],10) AND 1 = 0) then
        set_ins_volume(NONE,HI(volume_table[chan]),chan)
      else set_ins_volume(LO(volume_table[chan]),HI(volume_table[chan]),chan);
end;

procedure set_overall_volume(level: Byte);
begin
  overall_volume := max(level,63);
  set_global_volume;
end;

procedure init_macro_table(chan,note,ins: Byte; freq: Word);
begin
  macro_table[chan].fmreg_count := 1;
  macro_table[chan].fmreg_pos := 0;
  macro_table[chan].fmreg_duration := 0;
  macro_table[chan].fmreg_table := ins;
  macro_table[chan].arpg_count := 1;
  macro_table[chan].arpg_pos := 0;
  macro_table[chan].arpg_table := songdata.instr_macros[ins].arpeggio_table;
  macro_table[chan].arpg_note := note;
  macro_table[chan].vib_count := 1;
  macro_table[chan].vib_pos := 0;
  macro_table[chan].vib_table := songdata.instr_macros[ins].vibrato_table;
  macro_table[chan].vib_freq := freq;
  macro_table[chan].vib_delay := songdata.macro_table[macro_table[chan].vib_table].vibrato.delay;
end;

procedure set_ins_data(ins,chan: Byte);

var
  old_ins: Byte;

begin
  If (ins <> event_table[chan].instr_def) or reset_chan[chan] then
    begin
      opl3out(_instr[02]+_chan_m[chan],63);
      opl3out(_instr[03]+_chan_c[chan],63);

      If NOT pan_lock[chan] then
        panning_table[chan] := ins_parameter(ins,11)
      else panning_table[chan] := songdata.lockflags[chan] AND 3;

      opl3out(_instr[00]+_chan_m[chan],ins_parameter(ins,0));
      opl3out(_instr[01]+_chan_c[chan],ins_parameter(ins,1));
      opl3out(_instr[04]+_chan_m[chan],ins_parameter(ins,4));
      opl3out(_instr[05]+_chan_c[chan],ins_parameter(ins,5));
      opl3out(_instr[06]+_chan_m[chan],ins_parameter(ins,6));
      opl3out(_instr[07]+_chan_c[chan],ins_parameter(ins,7));
      opl3out(_instr[08]+_chan_m[chan],ins_parameter(ins,8));
      opl3out(_instr[09]+_chan_c[chan],ins_parameter(ins,9));
      opl3out(_instr[10]+_chan_n[chan],ins_parameter(ins,10) OR _panning[panning_table[chan]]);

      fmpar_table[chan].connect := ins_parameter(ins,10) AND 1;
      fmpar_table[chan].feedb   := ins_parameter(ins,10) SHR 1 AND 7;
      fmpar_table[chan].multipM := ins_parameter(ins,0)  AND $0f;
      fmpar_table[chan].kslM    := ins_parameter(ins,2)  SHR 6;
      fmpar_table[chan].tremM   := ins_parameter(ins,0)  SHR 7;
      fmpar_table[chan].vibrM   := ins_parameter(ins,0)  SHR 6 AND 1;
      fmpar_table[chan].ksrM    := ins_parameter(ins,0)  SHR 4 AND 1;
      fmpar_table[chan].sustM   := ins_parameter(ins,0)  SHR 5 AND 1;
      fmpar_table[chan].multipC := ins_parameter(ins,1)  AND $0f;
      fmpar_table[chan].kslC    := ins_parameter(ins,3)  SHR 6;
      fmpar_table[chan].tremC   := ins_parameter(ins,1)  SHR 7;
      fmpar_table[chan].vibrC   := ins_parameter(ins,1)  SHR 6 AND 1;
      fmpar_table[chan].ksrC    := ins_parameter(ins,1)  SHR 4 AND 1;
      fmpar_table[chan].sustC   := ins_parameter(ins,1)  SHR 5 AND 1;

      fmpar_table[chan].adsrw_car.attck := ins_parameter(ins,5) SHR 4;
      fmpar_table[chan].adsrw_mod.attck := ins_parameter(ins,4) SHR 4;
      fmpar_table[chan].adsrw_car.dec   := ins_parameter(ins,5) AND $0f;
      fmpar_table[chan].adsrw_mod.dec   := ins_parameter(ins,4) AND $0f;
      fmpar_table[chan].adsrw_car.sustn := ins_parameter(ins,7) SHR 4;
      fmpar_table[chan].adsrw_mod.sustn := ins_parameter(ins,6) SHR 4;
      fmpar_table[chan].adsrw_car.rel   := ins_parameter(ins,7) AND $0f;
      fmpar_table[chan].adsrw_mod.rel   := ins_parameter(ins,6) AND $0f;
      fmpar_table[chan].adsrw_car.wform := ins_parameter(ins,9) AND $07;
      fmpar_table[chan].adsrw_mod.wform := ins_parameter(ins,8) AND $07;

      If NOT reset_chan[chan] then
        keyoff_loop[chan] := FALSE;

      If reset_chan[chan] then
        begin
          voice_table[chan] := ins;
          reset_ins_volume(chan);
          reset_chan[chan] := FALSE;
        end;

      If (event_table[chan].note AND $7f in [1..12*8+1]) then
        init_macro_table(chan,event_table[chan].note AND $7f,ins,freq_table[chan])
      else init_macro_table(chan,0,ins,freq_table[chan]);
    end;

  vscale_table[chan] := concw(fmpar_table[chan].kslM SHL 6,
                              fmpar_table[chan].kslC SHL 6);
  voice_table[chan] := ins;
  old_ins := event_table[chan].instr_def;
  event_table[chan].instr_def := ins;

  If NOT volume_lock[chan] or
     (ins <> old_ins) then reset_ins_volume(chan);
end;

procedure update_modulator_adsrw(chan: Byte);
begin
  opl3out(_instr[04]+_chan_m[chan],
          fmpar_table[chan].adsrw_mod.attck SHL 4+
          fmpar_table[chan].adsrw_mod.dec);
  opl3out(_instr[06]+_chan_m[chan],
          fmpar_table[chan].adsrw_mod.sustn SHL 4+
          fmpar_table[chan].adsrw_mod.rel);
  opl3out(_instr[08]+_chan_m[chan],
          fmpar_table[chan].adsrw_mod.wform);
end;

procedure update_carrier_adsrw(chan: Byte);
begin
  opl3out(_instr[05]+_chan_c[chan],
          fmpar_table[chan].adsrw_car.attck SHL 4+
          fmpar_table[chan].adsrw_car.dec);
  opl3out(_instr[07]+_chan_c[chan],
          fmpar_table[chan].adsrw_car.sustn SHL 4+
          fmpar_table[chan].adsrw_car.rel);
  opl3out(_instr[09]+_chan_c[chan],
          fmpar_table[chan].adsrw_car.wform);
end;

procedure update_fmpar(chan: Byte);
begin
  opl3out(_instr[00]+_chan_m[chan],fmpar_table[chan].multipM+
                                   fmpar_table[chan].ksrM  SHL 4+
                                   fmpar_table[chan].sustM SHL 5+
                                   fmpar_table[chan].vibrM SHL 6+
                                   fmpar_table[chan].tremM SHL 7);
  opl3out(_instr[01]+_chan_c[chan],fmpar_table[chan].multipC+
                                   fmpar_table[chan].ksrC  SHL 4+
                                   fmpar_table[chan].sustC SHL 5+
                                   fmpar_table[chan].vibrC SHL 6+
                                   fmpar_table[chan].tremC SHL 7);

  opl3out(_instr[10]+_chan_n[chan],(fmpar_table[chan].connect+
                                    fmpar_table[chan].feedb SHL 1) OR
                                   _panning[panning_table[chan]]);

  vscale_table[chan] := concw(fmpar_table[chan].kslM SHL 6,
                              fmpar_table[chan].kslC SHL 6);
  set_ins_volume(LO(volume_table[chan]),
                 HI(volume_table[chan]),chan);
end;

procedure output_note(note,ins,chan: Byte; restart_macro: Boolean);

var
  freq: Word;

begin
  If (note = 0) and (ftune_table[chan] = 0) then EXIT;
  If NOT (note in [1..12*8+1]) then freq := freq_table[chan]
  else begin
         freq := nFreq(note-1)+SHORTINT(ins_parameter(ins,12));
         opl3out($0b0+_chan_n[chan],0);
         freq_table[chan] := concw(LO(freq_table[chan]),
                                   HI(freq_table[chan]) OR $20);
       end;

  If (ftune_table[chan] = -127) then ftune_table[chan] := 0;
  freq := freq+ftune_table[chan];
  change_frequency(chan,freq);

  If (note <> 0) then
    begin
      event_table[chan].note := note;
      If restart_macro then
        With event_table[chan] do
           If NOT (((effect_def = ef_Extended) and
                   (effect DIV 16 = ef_ex_ExtendedCmd) and
                   (effect MOD 16 = ef_ex_cmd_NoRestart)) or
                  ((effect_def2 = ef_Extended) and
                   (effect2 DIV 16 = ef_ex_ExtendedCmd) and
                   (effect2 MOD 16 = ef_ex_cmd_NoRestart))) then
             init_macro_table(chan,note,ins,freq)
           else macro_table[chan].arpg_note := note;
    end;
end;

procedure output_note_NR(note,ins,chan: Byte; restart_macro: Boolean);

var
  freq: Word;

begin
  If (note = 0) and (ftune_table[chan] = 0) then EXIT;
  If NOT (note in [1..12*8+1]) then freq := freq_table[chan]
  else begin
         freq := nFreq(note-1)+SHORTINT(ins_parameter(ins,12));
         freq_table[chan] := concw(LO(freq_table[chan]),
                                   HI(freq_table[chan]) OR $20);
       end;

  If (ftune_table[chan] = -127) then ftune_table[chan] := 0;
  freq := freq+ftune_table[chan];

  change_frequency(chan,freq);
  If (note <> 0) then
    begin
      event_table[chan].note := note;
      If restart_macro then
        With event_table[chan] do
           If NOT (((effect_def = ef_Extended) and
                   (effect DIV 16 = ef_ex_ExtendedCmd) and
                   (effect MOD 16 = ef_ex_cmd_NoRestart)) or
                  ((effect_def2 = ef_Extended) and
                   (effect2 DIV 16 = ef_ex_ExtendedCmd) and
                   (effect2 MOD 16 = ef_ex_cmd_NoRestart))) then
             init_macro_table(chan,note,ins,freq)
           else macro_table[chan].arpg_note := note;
    end;
end;

procedure update_fine_effects(chan: Byte); forward;
procedure play_line;

var
  event: tADTRACK2_EVENT;
  chan,eLo,eHi,
  eLo2,eHi2: Byte;

function no_loop(current_chan,current_line: Byte): Boolean;

var
//  result: Boolean;
  chan: Byte;

begin
  result := TRUE;
  For chan := 1 to PRED(current_chan) do
    If (loop_table[chan][current_line] <> 0) and
       (loop_table[chan][current_line] <> NONE) then
      begin
        result := FALSE;
        BREAK;
      end;
  no_loop := result;
end;

begin
  For chan := 1 to songdata.noftracks do
    begin
      event := pattdata^[current_pattern DIV 8]
                        [current_pattern MOD 8][chan][current_line];
      If (effect_table[chan] <> 0) then last_effect[chan] := effect_table[chan];
      effect_table[chan] := effect_table[chan] AND $0ff00;
      If (effect_table2[chan] <> 0) then last_effect2[chan] := effect_table2[chan];
      effect_table2[chan] := effect_table2[chan] AND $0ff00;
      ftune_table[chan] := 0;

      If (event.note = NONE) then
        event.note := event_table[chan].note OR keyoff_flag
      else If (event.note in [fixed_note_flag+1..fixed_note_flag+12*8+1]) then
             event.note := event.note-fixed_note_flag;

      If (event.note <> 0) or
         (event.effect_def <> 0) or
         (event.effect_def2 <> 0) or
         ((event.effect_def = 0) and (event.effect <> 0)) or
         ((event.effect_def2 = 0) and (event.effect2 <> 0)) then
        event_new[chan] := TRUE
      else event_new[chan] := FALSE;

      If (event.note <> 0) or
         (event.instr_def <> 0) then
        begin
          event_table[chan].effect_def := event.effect_def;
          event_table[chan].effect := event.effect;
          event_table[chan].effect_def2 := event.effect_def2;
          event_table[chan].effect2 := event.effect2;
        end;

      If (event.instr_def <> 0) then
        If NOT nul_data(songdata.instr_data[event.instr_def],
                        INSTRUMENT_SIZE) then
          set_ins_data(event.instr_def,chan)
        else begin
               release_sustaining_sound(chan);
               set_ins_data(event.instr_def,chan);
             end;

      If NOT (event.effect_def in [ef_Vibrato,ef_ExtraFineVibrato,
                                   ef_VibratoVolSlide,ef_VibratoVSlideFine]) then
        FillChar(vibr_table[chan],SizeOf(vibr_table[chan]),0);

      If NOT (event.effect_def2 in [ef_Vibrato,ef_ExtraFineVibrato,
                                    ef_VibratoVolSlide,ef_VibratoVSlideFine]) then
        FillChar(vibr_table2[chan],SizeOf(vibr_table2[chan]),0);

      If NOT (event.effect_def in [ef_RetrigNote,ef_MultiRetrigNote]) then
        FillChar(retrig_table[chan],SizeOf(retrig_table[chan]),0);

      If NOT (event.effect_def2 in [ef_RetrigNote,ef_MultiRetrigNote]) then
        FillChar(retrig_table2[chan],SizeOf(retrig_table2[chan]),0);

      If NOT (event.effect_def in [ef_Tremolo,ef_ExtraFineTremolo]) then
        FillChar(trem_table[chan],SizeOf(trem_table[chan]),0);

      If NOT (event.effect_def2 in [ef_Tremolo,ef_ExtraFineTremolo]) then
        FillChar(trem_table2[chan],SizeOf(trem_table2[chan]),0);

      eLo  := LO(last_effect[chan]);
      eHi  := HI(last_effect[chan]);
      eLo2 := LO(last_effect2[chan]);
      eHi2 := HI(last_effect2[chan]);

      If (arpgg_table[chan].state <> 1) and
         (event.effect_def <> ef_ExtraFineArpeggio) then
        begin
          arpgg_table[chan].state := 1;
          change_frequency(chan,nFreq(arpgg_table[chan].note-1)+
            SHORTINT(ins_parameter(event_table[chan].instr_def,12)));
        end;

      If (arpgg_table2[chan].state <> 1) and
         (event.effect_def2 <> ef_ExtraFineArpeggio) then
        begin
          arpgg_table2[chan].state := 1;
          change_frequency(chan,nFreq(arpgg_table2[chan].note-1)+
            SHORTINT(ins_parameter(event_table[chan].instr_def,12)));
        end;

      If (tremor_table[chan].pos <> 0) and
         (event.effect_def <> ef_Tremor) then
        begin
          tremor_table[chan].pos := 0;
          set_ins_volume(LO(tremor_table[chan].volume),
                         HI(tremor_table[chan].volume),chan);
        end;

      If (tremor_table2[chan].pos <> 0) and
         (event.effect_def2 <> ef_Tremor) then
        begin
          tremor_table2[chan].pos := 0;
          set_ins_volume(LO(tremor_table2[chan].volume),
                         HI(tremor_table2[chan].volume),chan);
        end;

      If NOT (pattern_break and (next_line AND $0f0 = pattern_loop_flag)) and
             (current_order <> last_order) then
        begin
          FillChar(loopbck_table,SizeOf(loopbck_table),NONE);
          FillChar(loop_table,SizeOf(loop_table),NONE);
          last_order := current_order;
        end;

      Case event.effect_def of
        ef_Arpeggio,
        ef_ExtraFineArpeggio,
        ef_ArpggVSlide,
        ef_ArpggVSlideFine:
          If (event.effect_def <> ef_Arpeggio) or
             (event.effect <> 0) then
            begin
              Case event.effect_def of
                ef_Arpeggio:
                  effect_table[chan] := concw(ef_Arpeggio+ef_fix1,event.effect);

                ef_ExtraFineArpeggio:
                  effect_table[chan] := concw(ef_ExtraFineArpeggio,event.effect);

                ef_ArpggVSlide,
                ef_ArpggVSlideFine:
                  If (event.effect <> 0) then
                    effect_table[chan] := concw(event.effect_def,event.effect)
                  else If (eLo in [ef_ArpggVSlide,ef_ArpggVSlideFine]) and
                          (eHi <> 0) then
                         effect_table[chan] := concw(event.effect_def,eHi)
                       else effect_table[chan] := effect_table[chan] AND $0ff00;
              end;

              If (event.note AND $7f in [1..12*8+1]) then
                begin
                  arpgg_table[chan].state := 0;
                  arpgg_table[chan].note := event.note AND $7f;
                  If (event.effect_def in [ef_Arpeggio,ef_ExtraFineArpeggio]) then
                    begin
                      arpgg_table[chan].add1 := event.effect DIV 16;
                      arpgg_table[chan].add2 := event.effect MOD 16;
                    end;
                end
              else If (event.note = 0) and
                      (event_table[chan].note AND $7f in [1..12*8+1]) then
                     begin
                       If NOT (eLo in [ef_Arpeggio+ef_fix1,ef_ExtraFineArpeggio,
                                       ef_ArpggVSlide,ef_ArpggVSlideFine]) then
                         arpgg_table[chan].state := 0;

                       arpgg_table[chan].note := event_table[chan].note AND $7f;
                       If (event.effect_def in [ef_Arpeggio,ef_ExtraFineArpeggio]) then
                         begin
                           arpgg_table[chan].add1 := event.effect DIV 16;
                           arpgg_table[chan].add2 := event.effect MOD 16;
                         end;
                     end
                   else effect_table[chan] := 0;
            end;

        ef_FSlideUp,
        ef_FSlideDown,
        ef_FSlideUpFine,
        ef_FSlideDownFine:
          begin
            effect_table[chan] := concw(event.effect_def,event.effect);
            fslide_table[chan] := event.effect;
          end;

        ef_FSlideUpVSlide,
        ef_FSlUpVSlF,
        ef_FSlideDownVSlide,
        ef_FSlDownVSlF,
        ef_FSlUpFineVSlide,
        ef_FSlUpFineVSlF,
        ef_FSlDownFineVSlide,
        ef_FSlDownFineVSlF:
          If (event.effect <> 0) then
            effect_table[chan] := concw(event.effect_def,event.effect)
          else If (eLo in [ef_FSlideUpVSlide,ef_FSlUpVSlF,ef_FSlideDownVSlide,
                           ef_FSlDownVSlF,ef_FSlUpFineVSlide,ef_FSlUpFineVSlF,
                           ef_FSlDownFineVSlide,ef_FSlDownFineVSlF]) and
                  (eHi <> 0) then
                 effect_table[chan] := concw(event.effect_def,eHi)
               else effect_table[chan] := effect_table[chan] AND $0ff00;

        ef_TonePortamento:
          If (event.note in [1..12*8+1]) then
            begin
              If (event.effect <> 0) then
                effect_table[chan] := concw(ef_TonePortamento,event.effect)
              else If (eLo = ef_TonePortamento) and
                      (eHi <> 0) then
                     effect_table[chan] := concw(ef_TonePortamento,eHi)
                   else effect_table[chan] := ef_TonePortamento;

              porta_table[chan].speed := HI(effect_table[chan]);
              porta_table[chan].freq := nFreq(event.note-1)+
                SHORTINT(ins_parameter(event_table[chan].instr_def,12));
            end
          else If (eLo = ef_TonePortamento) then
                 begin
                   If (event.effect <> 0) then
                     effect_table[chan] := concw(ef_TonePortamento,event.effect)
                   else If (eLo = ef_TonePortamento) and
                           (eHi <> 0) then
                          effect_table[chan] := concw(ef_TonePortamento,eHi)
                        else effect_table[chan] := ef_TonePortamento;
                   porta_table[chan].speed := HI(effect_table[chan]);
                 end;

        ef_TPortamVolSlide,
        ef_TPortamVSlideFine:
          If (event.effect <> 0) then
            effect_table[chan] := concw(event.effect_def,event.effect)
          else If (eLo in [ef_TPortamVolSlide,ef_TPortamVSlideFine]) and
                  (eHi <> 0) then
                 effect_table[chan] := concw(event.effect_def,eHi)
               else effect_table[chan] := effect_table[chan] AND $0ff00;

        ef_Vibrato,
        ef_ExtraFineVibrato:
          begin
            If (event.effect <> 0) then
              effect_table[chan] := concw(event.effect_def,event.effect)
            else If (eLo in [ef_Vibrato,ef_ExtraFineVibrato]) and
                    (eHi <> 0) then
                   effect_table[chan] := concw(event.effect_def,eHi)
                 else effect_table[chan] := event.effect_def;

            If (event.effect_def2 = ef_Extended) and
               (event.effect2 = ef_ex_ExtendedCmd*16+ef_ex_cmd_FineVibr) then
              vibr_table[chan].fine := TRUE;

            vibr_table[chan].speed := HI(effect_table[chan]) DIV 16;
            vibr_table[chan].depth := HI(effect_table[chan]) MOD 16;
          end;

        ef_Tremolo,
        ef_ExtraFineTremolo:
          begin
            If (event.effect <> 0) then
              effect_table[chan] := concw(event.effect_def,event.effect)
            else If (eLo in [ef_Tremolo,ef_ExtraFineTremolo]) and
                    (eHi <> 0) then
                   effect_table[chan] := concw(event.effect_def,eHi)
                 else effect_table[chan] := event.effect_def;

            If (event.effect_def2 = ef_Extended) and
               (event.effect2 = ef_ex_ExtendedCmd*16+ef_ex_cmd_FineTrem) then
              trem_table[chan].fine := TRUE;

            trem_table[chan].speed := HI(effect_table[chan]) DIV 16;
            trem_table[chan].depth := HI(effect_table[chan]) MOD 16;
          end;

        ef_VibratoVolSlide,
        ef_VibratoVSlideFine:
          begin
            If (event.effect <> 0) then
              effect_table[chan] := concw(event.effect_def,event.effect)
            else If (eLo in [ef_VibratoVolSlide,ef_VibratoVSlideFine]) and
                    (HI(effect_table[chan]) <> 0) then
                   effect_table[chan] := concw(event.effect_def,HI(effect_table[chan]))
                 else effect_table[chan] := effect_table[chan] AND $0ff00;

            If (event.effect_def2 = ef_Extended) and
               (event.effect2 = ef_ex_ExtendedCmd*16+ef_ex_cmd_FineVibr) then
              vibr_table[chan].fine := TRUE;
          end;

        ef_SetCarrierVol:
          set_ins_volume(NONE,63-event.effect,chan);

        ef_SetModulatorVol:
          set_ins_volume(63-event.effect,NONE,chan);

        ef_SetInsVolume:
          If percussion_mode and (chan in [17..20]) then
            set_ins_volume(63-event.effect,NONE,chan)
          else If (ins_parameter(voice_table[chan],10) AND 1 = 0) then
                 set_ins_volume(NONE,63-event.effect,chan)
               else set_ins_volume(63-event.effect,63-event.effect,chan);

        ef_ForceInsVolume:
          If percussion_mode and (chan in [17..20]) then
            set_ins_volume(63-event.effect,NONE,chan)
          else set_ins_volume(scale_volume(ins_parameter(voice_table[chan],2) AND $3f,63-event.effect),63-event.effect,chan);

        ef_PositionJump:
          If no_loop(chan,current_line) then
            begin
              pattern_break := TRUE;
              next_line := pattern_break_flag+chan;
            end;

        ef_PatternBreak:
          If no_loop(chan,current_line) then
            begin
              pattern_break := TRUE;
              next_line := max(event.effect,PRED(songdata.pattlen));
            end;

        ef_SetSpeed:
          speed := event.effect;

        ef_SetTempo:
          update_timer(event.effect);

        ef_SetWaveform:
          begin
            If (event.effect DIV 16 in [0..7]) then
              begin
                fmpar_table[chan].adsrw_car.wform := event.effect DIV 16;
                update_carrier_adsrw(chan);
              end;

            If (event.effect MOD 16 in [0..7]) then
              begin
                fmpar_table[chan].adsrw_mod.wform := event.effect MOD 16;
                update_modulator_adsrw(chan);
              end;
          end;

        ef_VolSlide:
          effect_table[chan] := concw(ef_VolSlide,event.effect);

        ef_VolSlideFine:
          effect_table[chan] := concw(ef_VolSlideFine,event.effect);

        ef_RetrigNote:
          If (event.effect <> 0) then
            begin
              If NOT (eLo in [ef_RetrigNote,ef_MultiRetrigNote]) then
                retrig_table[chan] := 1;
              effect_table[chan] := concw(ef_RetrigNote,event.effect);
            end;

        ef_SetGlobalVolume:
          begin
            global_volume := event.effect;
            set_global_volume;
          end;

        ef_MultiRetrigNote:
          If (event.effect DIV 16 <> 0) then
            begin
              If NOT (eLo in [ef_RetrigNote,ef_MultiRetrigNote]) then
                retrig_table[chan] := 1;
              effect_table[chan] := concw(ef_MultiRetrigNote,event.effect);
            end;

        ef_Tremor:
          If (event.effect DIV 16 <> 0) and
             (event.effect MOD 16 <> 0) then
          begin
            If (eLo <> ef_Tremor) then
              begin
                tremor_table[chan].pos := 0;
                tremor_table[chan].volume := volume_table[chan];
              end;
            effect_table[chan] := concw(ef_Tremor,event.effect);
          end;

        ef_Extended:
          Case (event.effect DIV 16) of
            ef_ex_SetTremDepth:
              Case (event.effect MOD 16) of
                0: begin
                     opl3out(_instr[11],misc_register AND $07f);
                     current_tremolo_depth := 0;
                   end;

                1: begin
                     opl3out(_instr[11],misc_register OR $080);
                     current_tremolo_depth := 1;
                   end;
              end;

            ef_ex_SetVibDepth:
              Case (event.effect MOD 16) of
                0: begin
                     opl3out(_instr[11],misc_register AND $0bf);
                     current_vibrato_depth := 0;
                   end;

                1: begin
                     opl3out(_instr[11],misc_register OR $040);
                     current_vibrato_depth := 1;
                   end;
              end;

            ef_ex_SetAttckRateM:
              begin
                fmpar_table[chan].adsrw_mod.attck := event.effect MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetDecayRateM:
              begin
                fmpar_table[chan].adsrw_mod.dec := event.effect MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetSustnLevelM:
              begin
                fmpar_table[chan].adsrw_mod.sustn := event.effect MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetRelRateM:
              begin
                fmpar_table[chan].adsrw_mod.rel := event.effect MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetAttckRateC:
              begin
                fmpar_table[chan].adsrw_car.attck := event.effect MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetDecayRateC:
              begin
                fmpar_table[chan].adsrw_car.dec := event.effect MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetSustnLevelC:
              begin
                fmpar_table[chan].adsrw_car.sustn := event.effect MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetRelRateC:
              begin
                fmpar_table[chan].adsrw_car.rel := event.effect MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetFeedback:
              begin
                fmpar_table[chan].feedb := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex_SetPanningPos:
              begin
                panning_table[chan] := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex_PatternLoop,
            ef_ex_PatternLoopRec:
              If (event.effect MOD 16 = 0) then
                loopbck_table[chan] := current_line
              else If (loopbck_table[chan] <> NONE) then
                     begin
                       If (loop_table[chan][current_line] = NONE) then
                         loop_table[chan][current_line] := event.effect MOD 16;
                       If (loop_table[chan][current_line] <> 0) then
                         begin
                           pattern_break := TRUE;
                           next_line := pattern_loop_flag+chan;
                         end
                       else If (event.effect DIV 16 = ef_ex_PatternLoopRec) then
                              loop_table[chan][current_line] := NONE;
                     end;

            ef_ex_MacroKOffLoop:
              If (event.effect MOD 16 <> 0) then
                keyoff_loop[chan] := TRUE
              else keyoff_loop[chan] := FALSE;

            ef_ex_ExtendedCmd:
              Case (event.effect MOD 16) of
                ef_ex_cmd_RSS:        release_sustaining_sound(chan);
                ef_ex_cmd_ResetVol:   reset_ins_volume(chan);
                ef_ex_cmd_LockVol:    volume_lock  [chan] := TRUE;
                ef_ex_cmd_UnlockVol:  volume_lock  [chan] := FALSE;
                ef_ex_cmd_LockVP:     peak_lock    [chan] := TRUE;
                ef_ex_cmd_UnlockVP:   peak_lock    [chan] := FALSE;
                ef_ex_cmd_VSlide_def: volslide_type[chan] := 0;
                ef_ex_cmd_LockPan:    pan_lock     [chan] := TRUE;
                ef_ex_cmd_UnlockPan:  pan_lock     [chan] := FALSE;
                ef_ex_cmd_VibrOff:    change_frequency(chan,freq_table[chan]);
                ef_ex_cmd_TremOff:    set_ins_volume(LO(volume_table[chan]),
                                                     HI(volume_table[chan]),chan);
                ef_ex_cmd_VSlide_car:
                  If (event.effect_def2 = ef_Extended) and
                     (event.effect2 = ef_ex_ExtendedCmd*16+
                                      ef_ex_cmd_VSlide_mod) then
                    volslide_type[chan] := 3
                  else volslide_type[chan] := 1;

                ef_ex_cmd_VSlide_mod:
                  If (event.effect_def2 = ef_Extended) and
                     (event.effect2 = ef_ex_ExtendedCmd*16+
                                      ef_ex_cmd_VSlide_car) then
                    volslide_type[chan] := 3
                  else volslide_type[chan] := 2;
              end;
          end;

        ef_Extended2:
          Case (event.effect DIV 16) of
            ef_ex2_PatDelayFrame,
            ef_ex2_PatDelayRow:
              begin
                pattern_delay := TRUE;
                If (event.effect DIV 16 = ef_ex2_PatDelayFrame) then
                  tickD := (event.effect MOD 16)
                else tickD := speed*(event.effect MOD 16);
              end;

            ef_ex2_NoteDelay:
              begin
                effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_NoteDelay,0);
                notedel_table[chan] := event.effect MOD 16;
              end;

            ef_ex2_NoteCut:
              begin
                effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_NoteCut,0);
                notecut_table[chan] := event.effect MOD 16;
              end;

            ef_ex2_FineTuneUp:
              Inc(ftune_table[chan],event.effect MOD 16);

            ef_ex2_FineTuneDown:
              Dec(ftune_table[chan],event.effect MOD 16);

            ef_ex2_GlVolSlideUp:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideUp,
                                          event.effect MOD 16);
            ef_ex2_GlVolSlideDn:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideDn,
                                          event.effect MOD 16);
            ef_ex2_GlVolSlideUpF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideUpF,
                                          event.effect MOD 16);
            ef_ex2_GlVolSlideDnF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideDnF,
                                          event.effect MOD 16);
            ef_ex2_GlVolSldUpXF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSldUpXF,
                                          event.effect MOD 16);
            ef_ex2_GlVolSldDnXF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSldDnXF,
                                          event.effect MOD 16);
            ef_ex2_VolSlideUpXF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_VolSlideUpXF,
                                          event.effect MOD 16);
            ef_ex2_VolSlideDnXF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_VolSlideDnXF,
                                          event.effect MOD 16);
            ef_ex2_FreqSlideUpXF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_FreqSlideUpXF,
                                          event.effect MOD 16);
            ef_ex2_FreqSlideDnXF:
              effect_table[chan] := concw(ef_extended2+ef_fix2+ef_ex2_FreqSlideDnXF,
                                          event.effect MOD 16);
          end;

        ef_Extended3:
          Case (event.effect DIV 16) of
            ef_ex3_SetConnection:
              begin
                fmpar_table[chan].connect := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetMultipM:
              begin
                fmpar_table[chan].multipM := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKslM:
              begin
                fmpar_table[chan].kslM := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetTremoloM:
              begin
                fmpar_table[chan].tremM := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetVibratoM:
              begin
                fmpar_table[chan].vibrM := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKsrM:
              begin
                fmpar_table[chan].ksrM := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetSustainM:
              begin
                fmpar_table[chan].sustM := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetMultipC:
              begin
                fmpar_table[chan].multipC := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKslC:
              begin
                fmpar_table[chan].kslC := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetTremoloC:
              begin
                fmpar_table[chan].tremC := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetVibratoC:
              begin
                fmpar_table[chan].vibrC := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKsrC:
              begin
                fmpar_table[chan].ksrC := event.effect MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetSustainC:
              begin
                fmpar_table[chan].sustC := event.effect MOD 16;
                update_fmpar(chan);
              end;
          end;
      end;

      Case event.effect_def2 of
        ef_Arpeggio,
        ef_ExtraFineArpeggio,
        ef_ArpggVSlide,
        ef_ArpggVSlideFine:
          If (event.effect_def2 <> ef_Arpeggio) or
             (event.effect2 <> 0) then
            begin
              Case event.effect_def2 of
                ef_Arpeggio:
                  effect_table2[chan] := concw(ef_Arpeggio+ef_fix1,event.effect2);

                ef_ExtraFineArpeggio:
                  effect_table2[chan] := concw(ef_ExtraFineArpeggio,event.effect2);

                ef_ArpggVSlide,
                ef_ArpggVSlideFine:
                  If (event.effect2 <> 0) then
                    effect_table2[chan] := concw(event.effect_def2,event.effect2)
                  else If (eLo2 in [ef_ArpggVSlide,ef_ArpggVSlideFine]) and
                          (eHi2 <> 0) then
                         effect_table2[chan] := concw(event.effect_def2,eHi2)
                       else effect_table2[chan] := effect_table2[chan] AND $0ff00;
              end;

              If (event.note AND $7f in [1..12*8+1]) then
                begin
                  arpgg_table2[chan].state := 0;
                  arpgg_table2[chan].note := event.note AND $7f;
                  If (event.effect_def2 in [ef_Arpeggio,ef_ExtraFineArpeggio]) then
                    begin
                      arpgg_table2[chan].add1 := event.effect2 DIV 16;
                      arpgg_table2[chan].add2 := event.effect2 MOD 16;
                    end;
                end
              else If (event.note = 0) and
                      (event_table[chan].note AND $7f in [1..12*8+1]) then
                     begin
                       If NOT (eLo2 in [ef_Arpeggio+ef_fix1,ef_ExtraFineArpeggio,
                                       ef_ArpggVSlide,ef_ArpggVSlideFine]) then
                         arpgg_table2[chan].state := 0;

                       arpgg_table2[chan].note := event_table[chan].note AND $7f;
                       If (event.effect_def2 in [ef_Arpeggio,ef_ExtraFineArpeggio]) then
                         begin
                           arpgg_table2[chan].add1 := event.effect2 DIV 16;
                           arpgg_table2[chan].add2 := event.effect2 MOD 16;
                         end;
                     end
                   else effect_table2[chan] := 0;
            end;

        ef_FSlideUp,
        ef_FSlideDown,
        ef_FSlideUpFine,
        ef_FSlideDownFine:
          begin
            effect_table2[chan] := concw(event.effect_def2,event.effect2);
            fslide_table2[chan] := event.effect2;
          end;

        ef_FSlideUpVSlide,
        ef_FSlUpVSlF,
        ef_FSlideDownVSlide,
        ef_FSlDownVSlF,
        ef_FSlUpFineVSlide,
        ef_FSlUpFineVSlF,
        ef_FSlDownFineVSlide,
        ef_FSlDownFineVSlF:
          If (event.effect2 <> 0) then
            effect_table2[chan] := concw(event.effect_def2,event.effect2)
          else If (eLo2 in [ef_FSlideUpVSlide,ef_FSlUpVSlF,ef_FSlideDownVSlide,
                           ef_FSlDownVSlF,ef_FSlUpFineVSlide,ef_FSlUpFineVSlF,
                           ef_FSlDownFineVSlide,ef_FSlDownFineVSlF]) and
                  (eHi2 <> 0) then
                 effect_table2[chan] := concw(event.effect_def2,eHi2)
               else effect_table2[chan] := effect_table2[chan] AND $0ff00;

        ef_TonePortamento:
          If (event.note in [1..12*8+1]) then
            begin
              If (event.effect2 <> 0) then
                effect_table2[chan] := concw(ef_TonePortamento,event.effect2)
              else If (eLo2 = ef_TonePortamento) and
                      (eHi2 <> 0) then
                     effect_table2[chan] := concw(ef_TonePortamento,eHi2)
                   else effect_table2[chan] := ef_TonePortamento;

              porta_table2[chan].speed := HI(effect_table2[chan]);
              porta_table2[chan].freq := nFreq(event.note-1)+
                SHORTINT(ins_parameter(event_table[chan].instr_def,12));
            end
          else If (eLo2 = ef_TonePortamento) then
                 begin
                   If (event.effect2 <> 0) then
                     effect_table2[chan] := concw(ef_TonePortamento,event.effect2)
                   else If (eLo2 = ef_TonePortamento) and
                           (eHi2 <> 0) then
                          effect_table2[chan] := concw(ef_TonePortamento,eHi2)
                        else effect_table2[chan] := ef_TonePortamento;
                   porta_table2[chan].speed := HI(effect_table2[chan]);
                 end;

        ef_TPortamVolSlide,
        ef_TPortamVSlideFine:
          If (event.effect2 <> 0) then
            effect_table2[chan] := concw(event.effect_def2,event.effect2)
          else If (eLo2 in [ef_TPortamVolSlide,ef_TPortamVSlideFine]) and
                  (eHi2 <> 0) then
                 effect_table2[chan] := concw(event.effect_def2,eHi2)
               else effect_table2[chan] := effect_table2[chan] AND $0ff00;

        ef_Vibrato,
        ef_ExtraFineVibrato:
          begin
            If (event.effect2 <> 0) then
              effect_table2[chan] := concw(event.effect_def2,event.effect2)
            else If (eLo2 in [ef_Vibrato,ef_ExtraFineVibrato]) and
                    (eHi2 <> 0) then
                   effect_table2[chan] := concw(event.effect_def2,eHi2)
                 else effect_table2[chan] := event.effect_def2;

            If (event.effect_def = ef_Extended) and
               (event.effect = ef_ex_ExtendedCmd*16+ef_ex_cmd_FineVibr) then
              vibr_table2[chan].fine := TRUE;

            vibr_table2[chan].speed := HI(effect_table2[chan]) DIV 16;
            vibr_table2[chan].depth := HI(effect_table2[chan]) MOD 16;
          end;

        ef_Tremolo,
        ef_ExtraFineTremolo:
          begin
            If (event.effect2 <> 0) then
              effect_table2[chan] := concw(event.effect_def2,event.effect2)
            else If (eLo2 in [ef_Tremolo,ef_ExtraFineTremolo]) and
                    (eHi2 <> 0) then
                   effect_table2[chan] := concw(event.effect_def2,eHi2)
                 else effect_table2[chan] := event.effect_def2;

            If (event.effect_def = ef_Extended) and
               (event.effect = ef_ex_ExtendedCmd*16+ef_ex_cmd_FineTrem) then
              trem_table2[chan].fine := TRUE;

            trem_table2[chan].speed := HI(effect_table2[chan]) DIV 16;
            trem_table2[chan].depth := HI(effect_table2[chan]) MOD 16;
          end;

        ef_VibratoVolSlide,
        ef_VibratoVSlideFine:
          begin
            If (event.effect2 <> 0) then
              effect_table2[chan] := concw(event.effect_def2,event.effect2)
            else If (eLo2 in [ef_VibratoVolSlide,ef_VibratoVSlideFine]) and
                    (HI(effect_table2[chan]) <> 0) then
                   effect_table2[chan] := concw(event.effect_def2,HI(effect_table2[chan]))
                 else effect_table2[chan] := effect_table2[chan] AND $0ff00;

            If (event.effect_def = ef_Extended) and
               (event.effect = ef_ex_ExtendedCmd*16+ef_ex_cmd_FineVibr) then
              vibr_table2[chan].fine := TRUE;
          end;

        ef_SetCarrierVol:
          set_ins_volume(NONE,63-event.effect2,chan);

        ef_SetModulatorVol:
          set_ins_volume(63-event.effect2,NONE,chan);

        ef_SetInsVolume:
          If percussion_mode and (chan in [17..20]) then
            set_ins_volume(63-event.effect2,NONE,chan)
          else If (ins_parameter(voice_table[chan],10) AND 1 = 0) then
                 set_ins_volume(NONE,63-event.effect2,chan)
               else set_ins_volume(63-event.effect2,63-event.effect2,chan);

        ef_ForceInsVolume:
          If percussion_mode and (chan in [17..20]) then
            set_ins_volume(63-event.effect2,NONE,chan)
          else set_ins_volume(scale_volume(ins_parameter(voice_table[chan],2) AND $3f,63-event.effect2),63-event.effect2,chan);

        ef_PositionJump:
          If no_loop(chan,current_line) then
            begin
              pattern_break := TRUE;
              next_line := pattern_break_flag+chan;
            end;

        ef_PatternBreak:
          If no_loop(chan,current_line) then
            begin
              pattern_break := TRUE;
              next_line := event.effect2;
            end;

        ef_SetSpeed:
          speed := event.effect2;

        ef_SetTempo:
          update_timer(event.effect2);

        ef_SetWaveform:
          begin
            If (event.effect2 DIV 16 in [0..7]) then
              begin
                fmpar_table[chan].adsrw_car.wform := event.effect2 DIV 16;
                update_carrier_adsrw(chan);
              end;

            If (event.effect2 MOD 16 in [0..7]) then
              begin
                fmpar_table[chan].adsrw_mod.wform := event.effect2 MOD 16;
                update_modulator_adsrw(chan);
              end;
          end;

        ef_VolSlide:
          effect_table2[chan] := concw(ef_VolSlide,event.effect2);

        ef_VolSlideFine:
          effect_table2[chan] := concw(ef_VolSlideFine,event.effect2);

        ef_RetrigNote:
          If (event.effect2 <> 0) then
            begin
              If NOT (eLo2 in [ef_RetrigNote,ef_MultiRetrigNote]) then
                retrig_table2[chan] := 1;
              effect_table2[chan] := concw(ef_RetrigNote,event.effect2);
            end;

        ef_SetGlobalVolume:
          begin
            global_volume := event.effect2;
            set_global_volume;
          end;

        ef_MultiRetrigNote:
          If (event.effect2 DIV 16 <> 0) then
            begin
              If NOT (eLo2 in [ef_RetrigNote,ef_MultiRetrigNote]) then
                retrig_table2[chan] := 1;
              effect_table2[chan] := concw(ef_MultiRetrigNote,event.effect2);
            end;

        ef_Tremor:
          If (event.effect2 DIV 16 <> 0) and
             (event.effect2 MOD 16 <> 0) then
          begin
            If (eLo2 <> ef_Tremor) then
              begin
                tremor_table2[chan].pos := 0;
                tremor_table2[chan].volume := volume_table[chan];
              end;
            effect_table2[chan] := concw(ef_Tremor,event.effect2);
          end;

        ef_Extended:
          Case (event.effect2 DIV 16) of
            ef_ex_SetTremDepth:
              Case (event.effect2 MOD 16) of
                0: begin
                     opl3out(_instr[11],misc_register AND $07f);
                     current_tremolo_depth := 0;
                   end;

                1: begin
                     opl3out(_instr[11],misc_register OR $080);
                     current_tremolo_depth := 1;
                   end;
              end;

            ef_ex_SetVibDepth:
              Case (event.effect2 MOD 16) of
                0: begin
                     opl3out(_instr[11],misc_register AND $0bf);
                     current_vibrato_depth := 0;
                   end;

                1: begin
                     opl3out(_instr[11],misc_register OR $040);
                     current_vibrato_depth := 1;
                   end;
              end;

            ef_ex_SetAttckRateM:
              begin
                fmpar_table[chan].adsrw_mod.attck := event.effect2 MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetDecayRateM:
              begin
                fmpar_table[chan].adsrw_mod.dec := event.effect2 MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetSustnLevelM:
              begin
                fmpar_table[chan].adsrw_mod.sustn := event.effect2 MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetRelRateM:
              begin
                fmpar_table[chan].adsrw_mod.rel := event.effect2 MOD 16;
                update_modulator_adsrw(chan);
              end;

            ef_ex_SetAttckRateC:
              begin
                fmpar_table[chan].adsrw_car.attck := event.effect2 MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetDecayRateC:
              begin
                fmpar_table[chan].adsrw_car.dec := event.effect2 MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetSustnLevelC:
              begin
                fmpar_table[chan].adsrw_car.sustn := event.effect2 MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetRelRateC:
              begin
                fmpar_table[chan].adsrw_car.rel := event.effect2 MOD 16;
                update_carrier_adsrw(chan);
              end;

            ef_ex_SetFeedback:
              begin
                fmpar_table[chan].feedb := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex_SetPanningPos:
              begin
                panning_table[chan] := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex_PatternLoop,
            ef_ex_PatternLoopRec:
              If (event.effect2 MOD 16 = 0) then
                loopbck_table[chan] := current_line
              else If (loopbck_table[chan] <> NONE) then
                     begin
                       If (loop_table[chan][current_line] = NONE) then
                         loop_table[chan][current_line] := event.effect2 MOD 16;
                       If (loop_table[chan][current_line] <> 0) then
                         begin
                           pattern_break := TRUE;
                           next_line := pattern_loop_flag+chan;
                         end
                       else If (event.effect2 DIV 16 = ef_ex_PatternLoopRec) then
                              loop_table[chan][current_line] := NONE;
                     end;

            ef_ex_MacroKOffLoop:
              If (event.effect2 MOD 16 <> 0) then
                keyoff_loop[chan] := TRUE
              else keyoff_loop[chan] := FALSE;

            ef_ex_ExtendedCmd:
              Case (event.effect2 MOD 16) of
                ef_ex_cmd_RSS:        release_sustaining_sound(chan);
                ef_ex_cmd_ResetVol:   reset_ins_volume(chan);
                ef_ex_cmd_LockVol:    volume_lock  [chan] := TRUE;
                ef_ex_cmd_UnlockVol:  volume_lock  [chan] := FALSE;
                ef_ex_cmd_LockVP:     peak_lock    [chan] := TRUE;
                ef_ex_cmd_UnlockVP:   peak_lock    [chan] := FALSE;
                ef_ex_cmd_VSlide_def: volslide_type[chan] := 0;
                ef_ex_cmd_LockPan:    pan_lock     [chan] := TRUE;
                ef_ex_cmd_UnlockPan:  pan_lock     [chan] := FALSE;
                ef_ex_cmd_VibrOff:    change_frequency(chan,freq_table[chan]);
                ef_ex_cmd_TremOff:    set_ins_volume(LO(volume_table[chan]),
                                                     HI(volume_table[chan]),chan);
                ef_ex_cmd_VSlide_car:
                  If NOT ((event.effect_def = ef_Extended) and
                          (event.effect = ef_ex_ExtendedCmd*16+
                                          ef_ex_cmd_VSlide_mod)) then
                    volslide_type[chan] := 1;

                ef_ex_cmd_VSlide_mod:
                  If NOT ((event.effect_def = ef_Extended) and
                          (event.effect = ef_ex_ExtendedCmd*16+
                                          ef_ex_cmd_VSlide_car)) then
                    volslide_type[chan] := 2;
              end;
          end;

        ef_Extended2:
          Case (event.effect2 DIV 16) of
            ef_ex2_PatDelayFrame,
            ef_ex2_PatDelayRow:
              begin
                pattern_delay := TRUE;
                If (event.effect2 DIV 16 = ef_ex2_PatDelayFrame) then
                  tickD := (event.effect2 MOD 16)
                else tickD := speed*(event.effect2 MOD 16);
              end;

            ef_ex2_NoteDelay:
              begin
                effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_NoteDelay,0);
                notedel_table[chan] := event.effect2 MOD 16;
              end;

            ef_ex2_NoteCut:
              begin
                effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_NoteCut,0);
                notecut_table[chan] := event.effect2 MOD 16;
              end;

            ef_ex2_FineTuneUp:
              Inc(ftune_table[chan],event.effect2 MOD 16);

            ef_ex2_FineTuneDown:
              Dec(ftune_table[chan],event.effect2 MOD 16);

            ef_ex2_GlVolSlideUp:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideUp,
                                           event.effect2 MOD 16);
            ef_ex2_GlVolSlideDn:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideDn,
                                           event.effect2 MOD 16);
            ef_ex2_GlVolSlideUpF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideUpF,
                                           event.effect2 MOD 16);
            ef_ex2_GlVolSlideDnF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSlideDnF,
                                           event.effect2 MOD 16);
            ef_ex2_GlVolSldUpXF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSldUpXF,
                                           event.effect2 MOD 16);
            ef_ex2_GlVolSldDnXF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_GlVolSldDnXF,
                                           event.effect2 MOD 16);
            ef_ex2_VolSlideUpXF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_VolSlideUpXF,
                                           event.effect2 MOD 16);
            ef_ex2_VolSlideDnXF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_VolSlideDnXF,
                                           event.effect2 MOD 16);
            ef_ex2_FreqSlideUpXF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_FreqSlideUpXF,
                                           event.effect2 MOD 16);
            ef_ex2_FreqSlideDnXF:
              effect_table2[chan] := concw(ef_extended2+ef_fix2+ef_ex2_FreqSlideDnXF,
                                           event.effect2 MOD 16);
          end;

        ef_Extended3:
          Case (event.effect2 DIV 16) of
            ef_ex3_SetConnection:
              begin
                fmpar_table[chan].connect := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetMultipM:
              begin
                fmpar_table[chan].multipM := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKslM:
              begin
                fmpar_table[chan].kslM := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetTremoloM:
              begin
                fmpar_table[chan].tremM := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetVibratoM:
              begin
                fmpar_table[chan].vibrM := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKsrM:
              begin
                fmpar_table[chan].ksrM := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetSustainM:
              begin
                fmpar_table[chan].sustM := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetMultipC:
              begin
                fmpar_table[chan].multipC := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKslC:
              begin
                fmpar_table[chan].kslC := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetTremoloC:
              begin
                fmpar_table[chan].tremC := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetVibratoC:
              begin
                fmpar_table[chan].vibrC := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetKsrC:
              begin
                fmpar_table[chan].ksrC := event.effect2 MOD 16;
                update_fmpar(chan);
              end;

            ef_ex3_SetSustainC:
              begin
                fmpar_table[chan].sustC := event.effect2 MOD 16;
                update_fmpar(chan);
              end;
          end;
      end;

      If (event.effect_def+event.effect = 0) then effect_table[chan] := 0
      else begin
             event_table[chan].effect_def := event.effect_def;
             event_table[chan].effect := event.effect;
           end;

      If (event.effect_def2+event.effect2 = 0) then effect_table2[chan] := 0
      else begin
             event_table[chan].effect_def2 := event.effect_def2;
             event_table[chan].effect2 := event.effect2;
           end;

      If (event.note = event.note OR keyoff_flag) then key_off(chan)
      else If NOT (LO(effect_table[chan]) in  [ef_TonePortamento,
                                               ef_TPortamVolSlide,
                                               ef_TPortamVSlideFine,
                                               ef_extended2+ef_fix2+ef_ex2_NoteDelay]) and
              NOT (LO(effect_table2[chan]) in [ef_TonePortamento,
                                               ef_TPortamVolSlide,
                                               ef_TPortamVSlideFine,
                                               ef_extended2+ef_fix2+ef_ex2_NoteDelay]) then
             If NOT (((event.effect_def2 = ef_SwapArpeggio) or
                      (event.effect_def2 = ef_SwapVibrato)) and
                     (event.effect_def = ef_Extended) and
                     (event.effect DIV 16 = ef_ex_ExtendedCmd) and
                     (event.effect MOD 16 = ef_ex_cmd_NoRestart)) and
                NOT (((event.effect_def = ef_SwapArpeggio) or
                      (event.effect_def = ef_SwapVibrato)) and
                     (event.effect_def2 = ef_Extended) and
                     (event.effect2 DIV 16 = ef_ex_ExtendedCmd) and
                     (event.effect2 MOD 16 = ef_ex_cmd_NoRestart)) then
               output_note(event.note,voice_table[chan],chan,TRUE)
             else output_note_NR(event.note,voice_table[chan],chan,TRUE)
          else If (event.note <> 0) then
                 event_table[chan].note := event.note;

      Case event.effect_def of
        ef_SwapArpeggio:
          begin
            If (event.effect_def2 = ef_Extended) and
               (event.effect2 DIV 16 = ef_ex_ExtendedCmd) and
               (event.effect2 MOD 16 = ef_ex_cmd_NoRestart) then
              begin
                If (macro_table[chan].arpg_pos >
                    songdata.macro_table[event.effect].arpeggio.length) then
                  macro_table[chan].arpg_pos :=
                    songdata.macro_table[event.effect].arpeggio.length;
                macro_table[chan].arpg_table := event.effect;
              end
            else begin
                   macro_table[chan].arpg_count := 1;
                   macro_table[chan].arpg_pos := 0;
                   macro_table[chan].arpg_table := event.effect;
                   macro_table[chan].arpg_note := event_table[chan].note;
                 end;
          end;

        ef_SwapVibrato:
          begin
            If (event.effect_def2 = ef_Extended) and
               (event.effect2 DIV 16 = ef_ex_ExtendedCmd) and
               (event.effect2 MOD 16 = ef_ex_cmd_NoRestart) then
              begin
                If (macro_table[chan].vib_table >
                    songdata.macro_table[event.effect].vibrato.length) then
                  macro_table[chan].vib_pos :=
                    songdata.macro_table[event.effect].vibrato.length;
                macro_table[chan].vib_table := event.effect;
              end
            else begin
                   macro_table[chan].vib_count := 1;
                   macro_table[chan].vib_pos := 0;
                   macro_table[chan].vib_table := event.effect;
                   macro_table[chan].vib_delay := songdata.macro_table[macro_table[chan].vib_table].vibrato.delay;
                 end;
          end;
      end;

      Case event.effect_def2 of
        ef_SwapArpeggio:
          begin
            If (event.effect_def = ef_Extended) and
               (event.effect DIV 16 = ef_ex_ExtendedCmd) and
               (event.effect MOD 16 = ef_ex_cmd_NoRestart) then
              begin
                If (macro_table[chan].arpg_pos >
                    songdata.macro_table[event.effect2].arpeggio.length) then
                  macro_table[chan].arpg_pos :=
                    songdata.macro_table[event.effect2].arpeggio.length;
                macro_table[chan].arpg_table := event.effect2;
              end
            else begin
                   macro_table[chan].arpg_count := 1;
                   macro_table[chan].arpg_pos := 0;
                   macro_table[chan].arpg_table := event.effect2;
                   macro_table[chan].arpg_note := event_table[chan].note;
                 end;
          end;

        ef_SwapVibrato:
          begin
            If (event.effect_def = ef_Extended) and
               (event.effect DIV 16 = ef_ex_ExtendedCmd) and
               (event.effect MOD 16 = ef_ex_cmd_NoRestart) then
              begin
                If (macro_table[chan].vib_table >
                    songdata.macro_table[event.effect2].vibrato.length) then
                  macro_table[chan].vib_pos :=
                    songdata.macro_table[event.effect2].vibrato.length;
                macro_table[chan].vib_table := event.effect2;
              end
            else begin
                   macro_table[chan].vib_count := 1;
                   macro_table[chan].vib_pos := 0;
                   macro_table[chan].vib_table := event.effect2;
                   macro_table[chan].vib_delay := songdata.macro_table[macro_table[chan].vib_table].vibrato.delay;
                 end;
          end;
      end;

      update_fine_effects(chan);
    end;
end;

procedure portamento_up(chan: Byte; slide: Word; limit: Word);

var
  freq: Word;

begin
  freq := calc_freq_shift_up(freq_table[chan] AND $1fff,slide);
  If (freq <= limit) then change_frequency(chan,freq)
  else change_frequency(chan,limit);
end;

procedure portamento_down(chan: Byte; slide: Word; limit: Word);

var
  freq: Word;

begin
  freq := calc_freq_shift_down(freq_table[chan] AND $1fff,slide);
  If (freq >= limit) then change_frequency(chan,freq)
  else change_frequency(chan,limit);
end;

procedure macro_vibrato__porta_up(chan: Byte; depth: Byte);

var
  freq: Word;

begin
  freq := calc_freq_shift_up(macro_table[chan].vib_freq AND $1fff,depth);
  If (freq <= nFreq(12*8+1)) then change_freq(chan,freq)
  else change_freq(chan,nFreq(12*8+1));
end;

procedure macro_vibrato__porta_down(chan: Byte; depth: Byte);

var
  freq: Word;

begin
  freq := calc_freq_shift_down(macro_table[chan].vib_freq AND $1fff,depth);
  If (freq >= nFreq(0)) then change_freq(chan,freq)
  else change_freq(chan,nFreq(0));
end;

procedure tone_portamento(chan: Byte);
begin
  If (freq_table[chan] AND $1fff > porta_table[chan].freq) then
    portamento_down(chan,porta_table[chan].speed,porta_table[chan].freq)
  else If (freq_table[chan] AND $1fff < porta_table[chan].freq) then
         portamento_up(chan,porta_table[chan].speed,porta_table[chan].freq);
end;

procedure tone_portamento2(chan: Byte);
begin
  If (freq_table[chan] AND $1fff > porta_table2[chan].freq) then
    portamento_down(chan,porta_table2[chan].speed,porta_table2[chan].freq)
  else If (freq_table[chan] AND $1fff < porta_table2[chan].freq) then
         portamento_up(chan,porta_table2[chan].speed,porta_table2[chan].freq);
end;

procedure slide_volume_up(chan,slide: Byte);

var
  temp: Word;
  limit1,limit2,vLo,vHi: Byte;


begin
  If NOT peak_lock[chan] then limit1 := 0
  else limit1 := ins_parameter(event_table[chan].instr_def,3) AND $3f;

  If NOT peak_lock[chan] then limit2 := 0
  else limit2 := ins_parameter(event_table[chan].instr_def,2) AND $3f;
  temp := volume_table[chan];

  Case volslide_type[chan] of
    0: begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vHi-slide >= limit1) then temp := concw(vLo,vHi-slide)
         else temp := concw(vLo,limit1);
         set_ins_volume(NONE,HI(temp),chan);
         volume_table[chan] := temp;
         If (ins_parameter(voice_table[chan],10) AND 1 = 1) or
            (percussion_mode and (chan in [17..20])) then
           begin
             vLo := LO(temp);
             vHi := HI(temp);
             If (vLo-slide >= limit2) then temp := concw(vLo-slide,vHi)
             else temp := concw(limit2,vHi);
             set_ins_volume(LO(temp),NONE,chan);
             volume_table[chan] := temp;
           end;
       end;
    1:
       begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vHi-slide >= limit1) then temp := concw(vLo,vHi-slide)
         else temp := concw(vLo,limit1);
         set_ins_volume(NONE,HI(temp),chan);
         volume_table[chan] := temp;
       end;
    2:
       begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vLo-slide >= limit2) then temp := concw(vLo-slide,vHi)
         else temp := concw(limit2,vHi);
         set_ins_volume(LO(temp),NONE,chan);
         volume_table[chan] := temp;
       end;
    3: begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vHi-slide >= limit1) then temp := concw(vLo,vHi-slide)
         else temp := concw(vLo,limit1);
         set_ins_volume(NONE,HI(temp),chan);
         volume_table[chan] := temp;
         vLo := LO(temp);
         vHi := HI(temp);
         If (vLo-slide >= limit2) then temp := concw(vLo-slide,vHi)
         else temp := concw(limit2,vHi);
         set_ins_volume(LO(temp),NONE,chan);
         volume_table[chan] := temp;
       end;
  end;
end;

procedure slide_volume_down(chan,slide: Byte);

var
  temp: Word;
  vLo,vHi: Byte;

begin
  temp := volume_table[chan];
  Case volslide_type[chan] of
    0: begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vHi+slide <= 63) then temp := concw(vLo,vHi+slide)
         else temp := concw(vLo,63);
         set_ins_volume(NONE,HI(temp),chan);
         volume_table[chan] := temp;
         If (ins_parameter(voice_table[chan],10) AND 1 = 1) or
            (percussion_mode and (chan in [17..20])) then
           begin
             vLo := LO(temp);
             vHi := HI(temp);
             If (vLo+slide <= 63) then temp := concw(vLo+slide,vHi)
             else temp := concw(63,vHi);
             set_ins_volume(LO(temp),NONE,chan);
             volume_table[chan] := temp;
           end;
       end;
    1:
       begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vHi+slide <= 63) then temp := concw(vLo,vHi+slide)
         else temp := concw(vLo,63);
         set_ins_volume(NONE,HI(temp),chan);
         volume_table[chan] := temp;
       end;
    2:
       begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vLo+slide <= 63) then temp := concw(vLo+slide,vHi)
         else temp := concw(63,vHi);
         set_ins_volume(LO(temp),NONE,chan);
         volume_table[chan] := temp;
       end;
    3: begin
         vLo := LO(temp);
         vHi := HI(temp);
         If (vHi+slide <= 63) then temp := concw(vLo,vHi+slide)
         else temp := concw(vLo,63);
         set_ins_volume(NONE,HI(temp),chan);
         volume_table[chan] := temp;
         vLo := LO(temp);
         vHi := HI(temp);
         If (vLo+slide <= 63) then temp := concw(vLo+slide,vHi)
         else temp := concw(63,vHi);
         set_ins_volume(LO(temp),NONE,chan);
         volume_table[chan] := temp;
       end;
  end;
end;

procedure volume_slide(chan,up_speed,down_speed: Byte);
begin
  If (up_speed <> 0) then slide_volume_up(chan,up_speed)
  else If (down_speed <> 0) then slide_volume_down(chan,down_speed);
end;

procedure global_volume_slide(up_speed,down_speed: Byte);
begin
  If (up_speed <> NONE) then
    global_volume := max(global_volume+up_speed,63);
  If (down_speed <> NONE) then
    If (global_volume >= down_speed) then Dec(global_volume,down_speed)
    else global_volume := 0;
  set_global_volume;
end;

procedure arpeggio(chan: Byte);

const
  arpgg_state: array[0..2] of Byte = (1,2,0);

var
  freq: Word;

begin
  Case arpgg_table[chan].state of
    0: freq := nFreq(arpgg_table[chan].note-1);
    1: freq := nFreq(arpgg_table[chan].note-1 +arpgg_table[chan].add1);
    2: freq := nFreq(arpgg_table[chan].note-1 +arpgg_table[chan].add2);
  end;

  arpgg_table[chan].state := arpgg_state[arpgg_table[chan].state];
  change_frequency(chan,freq+
    SHORTINT(ins_parameter(event_table[chan].instr_def,12)));
end;

procedure arpeggio2(chan: Byte);

const
  arpgg_state: array[0..2] of Byte = (1,2,0);

var
  freq: Word;

begin
  Case arpgg_table2[chan].state of
    0: freq := nFreq(arpgg_table2[chan].note-1);
    1: freq := nFreq(arpgg_table2[chan].note-1 +arpgg_table2[chan].add1);
    2: freq := nFreq(arpgg_table2[chan].note-1 +arpgg_table2[chan].add2);
  end;

  arpgg_table2[chan].state := arpgg_state[arpgg_table2[chan].state];
  change_frequency(chan,freq+
    SHORTINT(ins_parameter(event_table[chan].instr_def,12)));
end;

procedure vibrato(chan: Byte);

var
  freq,old_freq: Word;
  direction: Byte;

begin
  Inc(vibr_table[chan].pos,vibr_table[chan].speed);
  freq := calc_vibrato_shift(vibr_table[chan].depth,vibr_table[chan].pos);
  direction := freq AND 1;
  freq := freq SHR 1;
  old_freq := freq_table[chan];
  If (direction = 0) then portamento_down(chan,freq,nFreq(0))
  else portamento_up(chan,freq,nFreq(12*8+1));
  freq_table[chan] := old_freq;
end;

procedure vibrato2(chan: Byte);

var
  freq,old_freq: Word;
  direction: Byte;

begin
  Inc(vibr_table2[chan].pos,vibr_table2[chan].speed);
  freq := calc_vibrato_shift(vibr_table2[chan].depth,vibr_table2[chan].pos);
  direction := freq AND 1;
  freq := freq SHR 1;
  old_freq := freq_table[chan];
  If (direction = 0) then portamento_down(chan,freq,nFreq(0))
  else portamento_up(chan,freq,nFreq(12*8+1));
  freq_table[chan] := old_freq;
end;

procedure tremolo(chan: Byte);

var
  vol,old_vol: Word;
  direction: Byte;

begin
  Inc(trem_table[chan].pos,trem_table[chan].speed);
  vol := calc_vibrato_shift(trem_table[chan].depth,trem_table[chan].pos);
  direction := vol AND 1;
  vol := vol SHR 1;
  old_vol := volume_table[chan];
  If (direction = 0) then slide_volume_down(chan,vol)
  else slide_volume_up(chan,vol);
  volume_table[chan] := old_vol;
end;

procedure tremolo2(chan: Byte);

var
  vol,old_vol: Word;
  direction: Byte;

begin
  Inc(trem_table2[chan].pos,trem_table2[chan].speed);
  vol := calc_vibrato_shift(trem_table2[chan].depth,trem_table2[chan].pos);
  direction := vol AND 1;
  vol := vol SHR 1;
  old_vol := volume_table[chan];
  If (direction = 0) then slide_volume_down(chan,vol)
  else slide_volume_up(chan,vol);
  volume_table[chan] := old_vol;
end;

procedure update_effects;

var
  chan,eLo,eHi,
  eLo2,eHi2: Byte;

function chanvol(chan: Byte): Byte;
begin
  If (ins_parameter(voice_table[chan],10) AND 1 = 0) then chanvol := 63-HI(volume_table[chan])
  else chanvol := 63-Round((LO(volume_table[chan])+HI(volume_table[chan]))/2);
end;

begin
  For chan := 1 to 20 do
    begin
      eLo  := LO(effect_table[chan]);
      eHi  := HI(effect_table[chan]);
      eLo2 := LO(effect_table2[chan]);
      eHi2 := HI(effect_table2[chan]);

      Case eLo of
        ef_Arpeggio+ef_fix1:
          arpeggio(chan);

        ef_ArpggVSlide:
          begin
            volume_slide(chan,eHi DIV 16,eHi MOD 16);
            arpeggio(chan);
          end;

        ef_ArpggVSlideFine:
          arpeggio(chan);

        ef_FSlideUp:
          portamento_up(chan,eHi,nFreq(12*8+1));

        ef_FSlideDown:
          portamento_down(chan,eHi,nFreq(0));

        ef_FSlideUpVSlide:
          begin
            portamento_up(chan,fslide_table[chan],nFreq(12*8+1));
            volume_slide(chan,eHi DIV 16,eHi MOD 16);
          end;

        ef_FSlUpVSlF:
          portamento_up(chan,fslide_table[chan],nFreq(12*8+1));

        ef_FSlideDownVSlide:
          begin
            portamento_down(chan,fslide_table[chan],nFreq(0));
            volume_slide(chan,eHi DIV 16,eHi MOD 16);
          end;

        ef_FSlDownVSlF:
          portamento_down(chan,fslide_table[chan],nFreq(0));

        ef_FSlUpFineVSlide:
          volume_slide(chan,eHi DIV 16,eHi MOD 16);

        ef_FSlDownFineVSlide:
          volume_slide(chan,eHi DIV 16,eHi MOD 16);

        ef_TonePortamento:
          tone_portamento(chan);

        ef_TPortamVolSlide:
          begin
            volume_slide(chan,eHi DIV 16,eHi MOD 16);
            tone_portamento(chan);
          end;

        ef_TPortamVSlideFine:
          tone_portamento(chan);

        ef_Vibrato:
          If NOT vibr_table[chan].fine then
            vibrato(chan);

        ef_Tremolo:
          If NOT trem_table[chan].fine then
            tremolo(chan);

        ef_VibratoVolSlide:
          begin
            volume_slide(chan,eHi DIV 16,eHi MOD 16);
            If NOT vibr_table[chan].fine then
              vibrato(chan);
          end;

        ef_VibratoVSlideFine:
          If NOT vibr_table[chan].fine then
            vibrato(chan);

        ef_VolSlide:
          volume_slide(chan,eHi DIV 16,eHi MOD 16);

        ef_RetrigNote:
          If (retrig_table[chan] >= eHi) then
            begin
              retrig_table[chan] := 0;
              output_note(event_table[chan].note,
                          event_table[chan].instr_def,chan,TRUE);
            end
          else Inc(retrig_table[chan]);

        ef_MultiRetrigNote:
          If (retrig_table[chan] >= eHi DIV 16) then
            begin
              Case eHi MOD 16 of
                0,8: ;

                1: slide_volume_down(chan,1);
                2: slide_volume_down(chan,2);
                3: slide_volume_down(chan,4);
                4: slide_volume_down(chan,8);
                5: slide_volume_down(chan,16);

                9: slide_volume_up(chan,1);
               10: slide_volume_up(chan,2);
               11: slide_volume_up(chan,4);
               12: slide_volume_up(chan,8);
               13: slide_volume_up(chan,16);


                6: slide_volume_down(chan,chanvol(chan)-
                                          Round(chanvol(chan)*2/3));
                7: slide_volume_down(chan,chanvol(chan)-
                                          Round(chanvol(chan)*1/2));

               14: slide_volume_up(chan,max(Round(chanvol(chan)*3/2)-
                                            chanvol(chan),63));
               15: slide_volume_up(chan,max(Round(chanvol(chan)*2)-
                                            chanvol(chan),63));
              end;

             retrig_table[chan] := 0;
             output_note(event_table[chan].note,
                          event_table[chan].instr_def,chan,TRUE);
            end
          else Inc(retrig_table[chan]);

        ef_Tremor:
          If (tremor_table[chan].pos >= 0) then
            begin
              If (SUCC(tremor_table[chan].pos) <= eHi DIV 16) then
                Inc(tremor_table[chan].pos)
              else begin
                     slide_volume_down(chan,63);
                     tremor_table[chan].pos := -1;
                   end;
            end
          else If (PRED(tremor_table[chan].pos) >= -(eHi MOD 16)) then
                 Dec(tremor_table[chan].pos)
               else begin
                      set_ins_volume(LO(tremor_table[chan].volume),
                                     HI(tremor_table[chan].volume),chan);
                      tremor_table[chan].pos := 1;
                    end;

        ef_extended2+ef_fix2+ef_ex2_NoteDelay:
          If (notedel_table[chan] = 0) then
            begin
              notedel_table[chan] := NONE;
              output_note(event_table[chan].note,
                          event_table[chan].instr_def,chan,TRUE);
            end
          else Dec(notedel_table[chan]);

        ef_extended2+ef_fix2+ef_ex2_NoteCut:
          If (notecut_table[chan] = 0) then
            begin
              notecut_table[chan] := NONE;
              key_off(chan);
            end
          else Dec(notecut_table[chan]);

        ef_extended2+ef_fix2+ef_ex2_GlVolSlideUp:
          global_volume_slide(eHi,NONE);

        ef_extended2+ef_fix2+ef_ex2_GlVolSlideDn:
          global_volume_slide(NONE,eHi);
      end;

      Case eLo2 of
        ef_Arpeggio+ef_fix1:
          arpeggio2(chan);

        ef_ArpggVSlide:
          begin
            volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
            arpeggio2(chan);
          end;

        ef_ArpggVSlideFine:
          arpeggio2(chan);

        ef_FSlideUp:
          portamento_up(chan,eHi2,nFreq(12*8+1));

        ef_FSlideDown:
          portamento_down(chan,eHi2,nFreq(0));

        ef_FSlideUpVSlide:
          begin
            portamento_up(chan,fslide_table2[chan],nFreq(12*8+1));
            volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
          end;

        ef_FSlUpVSlF:
          portamento_up(chan,fslide_table2[chan],nFreq(12*8+1));

        ef_FSlideDownVSlide:
          begin
            portamento_down(chan,fslide_table2[chan],nFreq(0));
            volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
          end;

        ef_FSlDownVSlF:
          portamento_down(chan,fslide_table2[chan],nFreq(0));

        ef_FSlUpFineVSlide:
          volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

        ef_FSlDownFineVSlide:
          volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

        ef_TonePortamento:
          tone_portamento2(chan);

        ef_TPortamVolSlide:
          begin
            volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
            tone_portamento2(chan);
          end;

        ef_TPortamVSlideFine:
          tone_portamento2(chan);

        ef_Vibrato:
          If NOT vibr_table2[chan].fine then
            vibrato2(chan);

        ef_Tremolo:
          If NOT trem_table2[chan].fine then
            tremolo2(chan);

        ef_VibratoVolSlide:
          begin
            volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
            If NOT vibr_table2[chan].fine then
              vibrato2(chan);
          end;

        ef_VibratoVSlideFine:
          If NOT vibr_table2[chan].fine then
            vibrato2(chan);

        ef_VolSlide:
          volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

        ef_RetrigNote:
          If (retrig_table2[chan] >= eHi2) then
            begin
              retrig_table2[chan] := 0;
              output_note(event_table[chan].note,
                          event_table[chan].instr_def,chan,TRUE);
            end
          else Inc(retrig_table2[chan]);

        ef_MultiRetrigNote:
          If (retrig_table2[chan] >= eHi2 DIV 16) then
            begin
              Case eHi2 MOD 16 of
                0,8: ;

                1: slide_volume_down(chan,1);
                2: slide_volume_down(chan,2);
                3: slide_volume_down(chan,4);
                4: slide_volume_down(chan,8);
                5: slide_volume_down(chan,16);

                9: slide_volume_up(chan,1);
               10: slide_volume_up(chan,2);
               11: slide_volume_up(chan,4);
               12: slide_volume_up(chan,8);
               13: slide_volume_up(chan,16);


                6: slide_volume_down(chan,chanvol(chan)-
                                          Round(chanvol(chan)*2/3));
                7: slide_volume_down(chan,chanvol(chan)-
                                          Round(chanvol(chan)*1/2));

               14: slide_volume_up(chan,max(Round(chanvol(chan)*3/2)-
                                            chanvol(chan),63));
               15: slide_volume_up(chan,max(Round(chanvol(chan)*2)-
                                            chanvol(chan),63));
              end;

              retrig_table2[chan] := 0;
              output_note(event_table[chan].note,
                          event_table[chan].instr_def,chan,TRUE);
            end
          else Inc(retrig_table2[chan]);

        ef_Tremor:
          If (tremor_table2[chan].pos >= 0) then
            begin
              If (SUCC(tremor_table2[chan].pos) <= eHi2 DIV 16) then
                Inc(tremor_table2[chan].pos)
              else begin
                     slide_volume_down(chan,63);
                     tremor_table2[chan].pos := -1;
                   end;
            end
          else If (PRED(tremor_table2[chan].pos) >= -(eHi2 MOD 16)) then
                 Dec(tremor_table2[chan].pos)
               else begin
                      set_ins_volume(LO(tremor_table2[chan].volume),
                                     HI(tremor_table2[chan].volume),chan);
                      tremor_table2[chan].pos := 1;
                    end;

        ef_extended2+ef_fix2+ef_ex2_NoteDelay:
          If (notedel_table[chan] = 0) then
            begin
              notedel_table[chan] := NONE;
              output_note(event_table[chan].note,
                          event_table[chan].instr_def,chan,TRUE);
            end
          else Dec(notedel_table[chan]);

        ef_extended2+ef_fix2+ef_ex2_NoteCut:
          If (notecut_table[chan] = 0) then
            begin
              notecut_table[chan] := NONE;
              key_off(chan);
            end
          else Dec(notecut_table[chan]);

        ef_extended2+ef_fix2+ef_ex2_GlVolSlideUp:
          global_volume_slide(eHi2,NONE);

        ef_extended2+ef_fix2+ef_ex2_GlVolSlideDn:
          global_volume_slide(NONE,eHi2);
      end;
    end;
end;

procedure update_fine_effects(chan: Byte);

var
  eLo,eHi,
  eLo2,eHi2: Byte;

begin
  eLo  := LO(effect_table[chan]);
  eHi  := HI(effect_table[chan]);
  eLo2 := LO(effect_table2[chan]);
  eHi2 := HI(effect_table2[chan]);

  Case eLo of
    ef_ArpggVSlideFine:
      volume_slide(chan,eHi DIV 16,eHi MOD 16);

    ef_FSlideUpFine:
      portamento_up(chan,eHi,nFreq(12*8+1));

    ef_FSlideDownFine:
      portamento_down(chan,eHi,nFreq(0));

    ef_FSlUpVSlF:
      volume_slide(chan,eHi DIV 16,eHi MOD 16);

    ef_FSlDownVSlF:
      volume_slide(chan,eHi DIV 16,eHi MOD 16);

    ef_FSlUpFineVSlide:
      portamento_up(chan,fslide_table[chan],nFreq(12*8+1));

    ef_FSlUpFineVSlF:
      begin
        portamento_up(chan,fslide_table[chan],nFreq(12*8+1));
        volume_slide(chan,eHi DIV 16,eHi MOD 16);
      end;

    ef_FSlDownFineVSlide:
      portamento_down(chan,fslide_table[chan],nFreq(0));

    ef_FSlDownFineVSlF:
      begin
        portamento_down(chan,fslide_table[chan],nFreq(0));
        volume_slide(chan,eHi DIV 16,eHi MOD 16);
      end;

    ef_TPortamVSlideFine:
      volume_slide(chan,eHi DIV 16,eHi MOD 16);

    ef_Vibrato:
      If vibr_table[chan].fine then
        vibrato(chan);

    ef_Tremolo:
      If trem_table[chan].fine then
        tremolo(chan);

    ef_VibratoVolSlide:
      If vibr_table[chan].fine then
        vibrato(chan);

    ef_VibratoVSlideFine:
      begin
        volume_slide(chan,eHi DIV 16,eHi MOD 16);
        If vibr_table[chan].fine then
          vibrato(chan);
      end;

    ef_VolSlideFine:
      volume_slide(chan,eHi DIV 16,eHi MOD 16);

    ef_extended2+ef_fix2+ef_ex2_GlVolSlideUpF:
      global_volume_slide(eHi,NONE);

    ef_extended2+ef_fix2+ef_ex2_GlVolSlideDnF:
      global_volume_slide(NONE,eHi);
  end;

  Case eLo2 of
    ef_ArpggVSlideFine:
      volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

    ef_FSlideUpFine:
      portamento_up(chan,eHi2,nFreq(12*8+1));

    ef_FSlideDownFine:
      portamento_down(chan,eHi2,nFreq(0));

    ef_FSlUpVSlF:
      volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

    ef_FSlDownVSlF:
      volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

    ef_FSlUpFineVSlide:
      portamento_up(chan,fslide_table2[chan],nFreq(12*8+1));

    ef_FSlUpFineVSlF:
      begin
        portamento_up(chan,fslide_table2[chan],nFreq(12*8+1));
        volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
      end;

    ef_FSlDownFineVSlide:
      portamento_down(chan,fslide_table2[chan],nFreq(0));

    ef_FSlDownFineVSlF:
      begin
        portamento_down(chan,fslide_table2[chan],nFreq(0));
        volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
      end;

    ef_TPortamVSlideFine:
      volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

    ef_Vibrato:
      If vibr_table2[chan].fine then
        vibrato2(chan);

    ef_Tremolo:
      If trem_table2[chan].fine then
        tremolo2(chan);

    ef_VibratoVolSlide:
      If vibr_table2[chan].fine then
        vibrato2(chan);

    ef_VibratoVSlideFine:
      begin
        volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);
        If vibr_table2[chan].fine then
          vibrato2(chan);
      end;

    ef_VolSlideFine:
      volume_slide(chan,eHi2 DIV 16,eHi2 MOD 16);

    ef_extended2+ef_fix2+ef_ex2_GlVolSlideUpF:
      global_volume_slide(eHi2,NONE);

    ef_extended2+ef_fix2+ef_ex2_GlVolSlideDnF:
      global_volume_slide(NONE,eHi2);
  end;
end;

procedure update_extra_fine_effects;

var
  chan,eLo,eHi,
  eLo2,eHi2: Byte;

begin
  For chan := 1 to 20 do
    begin
      eLo  := LO(effect_table[chan]);
      eHi  := HI(effect_table[chan]);
      eLo2 := LO(effect_table2[chan]);
      eHi2 := HI(effect_table2[chan]);

      Case eLo of
        ef_extended2+ef_fix2+ef_ex2_GlVolSldUpXF:
          global_volume_slide(eHi,NONE);

        ef_extended2+ef_fix2+ef_ex2_GlVolSldDnXF:
          global_volume_slide(NONE,eHi);

        ef_extended2+ef_fix2+ef_ex2_VolSlideUpXF:
          volume_slide(chan,eHi,0);

        ef_extended2+ef_fix2+ef_ex2_VolSlideDnXF:
          volume_slide(chan,0,eHi);

        ef_extended2+ef_fix2+ef_ex2_FreqSlideUpXF:
          portamento_up(chan,eHi,nFreq(12*8+1));

        ef_extended2+ef_fix2+ef_ex2_FreqSlideDnXF:
          portamento_down(chan,eHi,nFreq(0));

        ef_ExtraFineArpeggio:
          arpeggio(chan);

        ef_ExtraFineVibrato:
          If NOT vibr_table[chan].fine then
            vibrato(chan);

        ef_ExtraFineTremolo:
          If NOT trem_table[chan].fine then
            tremolo(chan);
      end;

      Case eLo2 of
        ef_extended2+ef_fix2+ef_ex2_GlVolSldUpXF:
          global_volume_slide(eHi2,NONE);

        ef_extended2+ef_fix2+ef_ex2_GlVolSldDnXF:
          global_volume_slide(NONE,eHi2);

        ef_extended2+ef_fix2+ef_ex2_VolSlideUpXF:
          volume_slide(chan,eHi2,0);

        ef_extended2+ef_fix2+ef_ex2_VolSlideDnXF:
          volume_slide(chan,0,eHi2);

        ef_extended2+ef_fix2+ef_ex2_FreqSlideUpXF:
          portamento_up(chan,eHi2,nFreq(12*8+1));

        ef_extended2+ef_fix2+ef_ex2_FreqSlideDnXF:
          portamento_down(chan,eHi2,nFreq(0));

        ef_ExtraFineArpeggio:
          arpeggio2(chan);

        ef_ExtraFineVibrato:
          If NOT vibr_table2[chan].fine then
            vibrato2(chan);

        ef_ExtraFineTremolo:
          If NOT trem_table2[chan].fine then
            tremolo2(chan);
      end;
    end;
end;

function calc_following_order(order: Byte): Integer;

var
//  result: Integer;
  index,jump_count: Byte;

begin
  result := -1;
  index := order;
  jump_count := 0;

  Repeat
    If (songdata.pattern_order[index] < $80) then result := index
    else begin
           index := songdata.pattern_order[index]-$80;
           Inc(jump_count);
         end;
  until (jump_count > $7f) or
        (result <> -1);

  calc_following_order := result;
end;

function calc_order_jump: Integer;

var
  temp: Byte;
//  result: Integer;

begin
  result := 0;
  temp := 0;

  Repeat
    If (songdata.pattern_order[current_order] > $7f) then
      current_order := songdata.pattern_order[current_order]-$80;
    Inc(temp);
  until (temp > $7f) or (songdata.pattern_order[current_order] < $80);

  If (temp > $7f) then begin stop_playing; result := -1; end;
  calc_order_jump := result;
end;

procedure update_song_position;

var
  temp: Byte;

begin
  If (current_line < PRED(songdata.pattlen)) and NOT pattern_break then Inc(current_line)
  else begin
         If NOT (pattern_break and (next_line AND $0f0 = pattern_loop_flag)) and
                (current_order < $7f) then
           begin
             FillChar(loopbck_table,SizeOf(loopbck_table),NONE);
             FillChar(loop_table,SizeOf(loop_table),NONE);
             Inc(current_order);
           end;

         If pattern_break and (next_line AND $0f0 = pattern_loop_flag) then
           begin
             temp := next_line-pattern_loop_flag;
             next_line := loopbck_table[temp];
             If (loop_table[temp][current_line] <> 0) then
               Dec(loop_table[temp][current_line]);
           end
         else If pattern_break and (next_line AND $0f0 = pattern_break_flag) then
                begin
                  current_order := event_table[next_line-pattern_break_flag].effect;
                  pattern_break := FALSE;
                end
              else If (current_order >= $7f) then
                     current_order := 0;

         If (songdata.pattern_order[current_order] > $7f) then
           If (calc_order_jump = -1) then EXIT;

         current_pattern := songdata.pattern_order[current_order];
         If NOT pattern_break then current_line := 0
         else begin
                pattern_break := FALSE;
                current_line := next_line;
              end;
       end;

  If (current_line = 0) and
     (current_order = calc_following_order(0)) and speed_update then
    begin
      tempo := songdata.tempo;
      speed := songdata.speed;
      update_timer(tempo);
    end;
end;

procedure poll_proc;
begin
  If NOT pattern_delay and (ticks-tick0+1 >= speed) then
    begin
      If (songdata.pattern_order[current_order] > $7f) then
        If (calc_order_jump = -1) then EXIT;

      current_pattern := songdata.pattern_order[current_order];
      play_line;
      update_effects;
      If NOT pattern_delay then update_song_position;
      tick0 := ticks;
    end
  else
    begin
      update_effects;
      Inc(ticks);
      If pattern_delay and (tickD > 1) then Dec(tickD)
      else begin
             If pattern_delay then
               begin
                 tick0 := ticks;
                 update_song_position;
               end;
             pattern_delay := FALSE;
           end;
    end;

  Inc(tickXF);
  If (tickXF MOD 4 = 0) then
    begin
      update_extra_fine_effects;
      Dec(tickXF,4);
    end;
end;

procedure macro_poll_proc;

const
  IDLE = $0fff;
  FINISHED = $0ffff;

var
  chan: Byte;
  finished_flag: Word;

begin
  For chan := 1 to 20 do
    begin
      If NOT keyoff_loop[chan] then finished_flag := FINISHED
      else finished_flag := IDLE;

      With macro_table[chan] do
        begin
          With songdata.instr_macros[fmreg_table] do
            If (fmreg_table <> 0) and (speed <> 0) then
              If (fmreg_duration > 1) then Dec(fmreg_duration)
              else begin
                     fmreg_count := 1;
                     If (fmreg_pos <= length) then
                       If (loop_begin <> 0) and (loop_length <> 0) then
                         If (fmreg_pos = loop_begin+PRED(loop_length)) then
                           fmreg_pos := loop_begin
                         else If (fmreg_pos < length) then Inc(fmreg_pos)
                              else fmreg_pos := finished_flag
                       else If (fmreg_pos < length) then Inc(fmreg_pos)
                            else fmreg_pos := finished_flag
                     else fmreg_pos := finished_flag;

                     If (freq_table[chan] OR $2000 = freq_table[chan]) and
                        (keyoff_pos <> 0) and
                        (fmreg_pos >= keyoff_pos) then
                       fmreg_pos := IDLE
                     else If (freq_table[chan] OR $2000 <> freq_table[chan]) and
                             (fmreg_pos <> 0) and (keyoff_pos <> 0) and
                             ((fmreg_pos < keyoff_pos) or (fmreg_pos = IDLE)) then
                            fmreg_pos := keyoff_pos;

                     If (fmreg_pos <> 0) and
                        (fmreg_pos <> IDLE) and (fmreg_pos <> finished_flag) then
                       begin
                         fmreg_duration := data[fmreg_pos].duration;
                         If (fmreg_duration <> 0) then
                           With data[fmreg_pos] do
                             begin
                               If NOT songdata.dis_fmreg_col[fmreg_table][0] then
                                 fmpar_table[chan].adsrw_mod.attck := fm_data.ATTCK_DEC_modulator SHR 4;

                               If NOT songdata.dis_fmreg_col[fmreg_table][1] then
                                 fmpar_table[chan].adsrw_mod.dec := fm_data.ATTCK_DEC_modulator AND $0f;

                               If NOT songdata.dis_fmreg_col[fmreg_table][2] then
                                 fmpar_table[chan].adsrw_mod.sustn := fm_data.SUSTN_REL_modulator SHR 4;

                               If NOT songdata.dis_fmreg_col[fmreg_table][3] then
                                 fmpar_table[chan].adsrw_mod.rel := fm_data.SUSTN_REL_modulator AND $0f;

                               If NOT songdata.dis_fmreg_col[fmreg_table][4] then
                                 fmpar_table[chan].adsrw_mod.wform := fm_data.WAVEFORM_modulator AND $07;

                               If NOT songdata.dis_fmreg_col[fmreg_table][6] then
                                 fmpar_table[chan].kslM := fm_data.KSL_VOLUM_modulator SHR 6;

                               If NOT songdata.dis_fmreg_col[fmreg_table][7] then
                                 fmpar_table[chan].multipM := fm_data.AM_VIB_EG_modulator AND $0f;

                               If NOT songdata.dis_fmreg_col[fmreg_table][8] then
                                 fmpar_table[chan].tremM := fm_data.AM_VIB_EG_modulator SHR 7;

                               If NOT songdata.dis_fmreg_col[fmreg_table][9] then
                                 fmpar_table[chan].vibrM := fm_data.AM_VIB_EG_modulator SHR 6 AND 1;

                               If NOT songdata.dis_fmreg_col[fmreg_table][10] then
                                 fmpar_table[chan].ksrM := fm_data.AM_VIB_EG_modulator SHR 4 AND 1;

                               If NOT songdata.dis_fmreg_col[fmreg_table][11] then
                                 fmpar_table[chan].sustM := fm_data.AM_VIB_EG_modulator SHR 5 AND 1;

                               If NOT songdata.dis_fmreg_col[fmreg_table][12] then
                                 fmpar_table[chan].adsrw_car.attck := fm_data.ATTCK_DEC_carrier SHR 4;

                               If NOT songdata.dis_fmreg_col[fmreg_table][13] then
                                 fmpar_table[chan].adsrw_car.dec := fm_data.ATTCK_DEC_carrier AND $0f;

                               If NOT songdata.dis_fmreg_col[fmreg_table][14] then
                                 fmpar_table[chan].adsrw_car.sustn := fm_data.SUSTN_REL_carrier SHR 4;

                               If NOT songdata.dis_fmreg_col[fmreg_table][15] then
                                 fmpar_table[chan].adsrw_car.rel := fm_data.SUSTN_REL_carrier AND $0f;

                               If NOT songdata.dis_fmreg_col[fmreg_table][16] then
                                 fmpar_table[chan].adsrw_car.wform := fm_data.WAVEFORM_carrier AND $07;

                               If NOT songdata.dis_fmreg_col[fmreg_table][18] then
                                 fmpar_table[chan].kslC := fm_data.KSL_VOLUM_carrier SHR 6;

                               If NOT songdata.dis_fmreg_col[fmreg_table][19] then
                                 fmpar_table[chan].multipC := fm_data.AM_VIB_EG_carrier AND $0f;

                               If NOT songdata.dis_fmreg_col[fmreg_table][20] then
                                 fmpar_table[chan].tremC := fm_data.AM_VIB_EG_carrier SHR 7;

                               If NOT songdata.dis_fmreg_col[fmreg_table][21] then
                                 fmpar_table[chan].vibrC := fm_data.AM_VIB_EG_carrier SHR 6 AND 1;

                               If NOT songdata.dis_fmreg_col[fmreg_table][22] then
                                 fmpar_table[chan].ksrC := fm_data.AM_VIB_EG_carrier SHR 4 AND 1;

                               If NOT songdata.dis_fmreg_col[fmreg_table][23] then
                                 fmpar_table[chan].sustC := fm_data.AM_VIB_EG_carrier SHR 5 AND 1;

                               If NOT songdata.dis_fmreg_col[fmreg_table][24] then
                                 fmpar_table[chan].connect := fm_data.FEEDBACK_FM AND 1;

                               If NOT songdata.dis_fmreg_col[fmreg_table][25] then
                                 fmpar_table[chan].feedb := fm_data.FEEDBACK_FM SHR 1 AND 7;

                               If NOT songdata.dis_fmreg_col[fmreg_table][27] then
                                 If NOT pan_lock[chan] then
                                   panning_table[chan] := panning;

                               If NOT songdata.dis_fmreg_col[fmreg_table][5] then
                                 set_ins_volume(63-fm_data.KSL_VOLUM_modulator AND $3f,
                                                NONE,chan);

                               If NOT songdata.dis_fmreg_col[fmreg_table][17] then
                                 set_ins_volume(NONE,
                                                63-fm_data.KSL_VOLUM_carrier AND $3f,chan);

                               update_modulator_adsrw(chan);
                               update_carrier_adsrw(chan);
                               update_fmpar(chan);

                               If NOT (fm_data.FEEDBACK_FM OR $80 <> fm_data.FEEDBACK_FM) then
                                 output_note(event_table[chan].note,
                                             event_table[chan].instr_def,chan,FALSE);

                               If NOT songdata.dis_fmreg_col[fmreg_table][26] then
                                 If (freq_slide > 0) then
                                   portamento_up(chan,freq_slide,nFreq(12*8+1))
                                 else If (freq_slide < 0) then
                                        portamento_down(chan,Abs(freq_slide),nFreq(0));
                             end;
                       end;
                   end;

          With songdata.macro_table[arpg_table].arpeggio do
            If (arpg_table <> 0) and (speed <> 0) then
              If (arpg_count = speed) then
                begin
                  arpg_count := 1;
                  If (arpg_pos <= length) then
                    If (loop_begin <> 0) and (loop_length <> 0) then
                      If (arpg_pos = loop_begin+PRED(loop_length)) then
                        arpg_pos := loop_begin
                      else If (arpg_pos < length) then Inc(arpg_pos)
                           else arpg_pos := finished_flag
                    else If (arpg_pos < length) then Inc(arpg_pos)
                         else arpg_pos := finished_flag
                  else arpg_pos := finished_flag;

                  If (freq_table[chan] OR $2000 = freq_table[chan]) and
                     (keyoff_pos <> 0) and
                     (arpg_pos >= keyoff_pos) then
                    arpg_pos := IDLE
                  else If (freq_table[chan] OR $2000 <> freq_table[chan]) and
                          (keyoff_pos <> 0) and (keyoff_pos <> 0) and
                          ((arpg_pos < keyoff_pos) or (arpg_pos = IDLE)) then
                         arpg_pos := keyoff_pos;

                  If (arpg_pos <> 0) and
                     (arpg_pos <> IDLE) and (arpg_pos <> finished_flag) then
                    Case data[arpg_pos] of
                      0: change_frequency(chan,
                           nFreq(arpg_note-1)+
                           SHORTINT(ins_parameter(event_table[chan].instr_def,12)));

                      1..96:
                        change_frequency(chan,
                          nFreq(max(arpg_note+data[arpg_pos],97)-1)+
                          SHORTINT(ins_parameter(event_table[chan].instr_def,12)));

                      $80..$80+12*8+1:
                        change_frequency(chan,nFreq(data[arpg_pos]-$80-1)+
                          SHORTINT(ins_parameter(event_table[chan].instr_def,12)));
                    end;
                end
              else Inc(arpg_count);

          With songdata.macro_table[vib_table].vibrato do
            If (vib_table <> 0) and (speed <> 0) then
              If (vib_count = speed) then
                If (vib_delay <> 0) then Dec(vib_delay)
                else begin
                       vib_count := 1;
                       If (vib_pos <= length) then
                         If (loop_begin <> 0) and (loop_length <> 0) then
                           If (vib_pos = loop_begin+PRED(loop_length)) then
                             vib_pos := loop_begin
                           else If (vib_pos < length) then Inc(vib_pos)
                                else vib_pos := finished_flag
                         else If (vib_pos < length) then Inc(vib_pos)
                              else vib_pos := finished_flag
                       else vib_pos := finished_flag;

                       If (freq_table[chan] OR $2000 = freq_table[chan]) and
                          (keyoff_pos <> 0) and
                          (vib_pos >= keyoff_pos) then
                         vib_pos := IDLE
                       else If (freq_table[chan] OR $2000 <> freq_table[chan]) and
                               (vib_pos <> 0) and (keyoff_pos <> 0) and
                               ((vib_pos < keyoff_pos) or (vib_pos = IDLE)) then
                              vib_pos := keyoff_pos;

                       If (vib_pos <> 0) and
                          (vib_pos <> IDLE) and (vib_pos <> finished_flag) then
                         If (data[vib_pos] > 0) then
                           macro_vibrato__porta_up(chan,data[vib_pos])
                         else If (data[vib_pos] < 0) then
                                macro_vibrato__porta_down(chan,Abs(data[vib_pos]))
                              else change_freq(chan,vib_freq);
                     end
              else Inc(vib_count);
        end;
    end;
end;

var
  ticklooper,
  macro_ticklooper: Longint;

procedure newtimer;
begin

  If (ticklooper = 0) then
    If irq_mode then poll_proc;

  If (macro_ticklooper = 0) then
    If irq_mode then macro_poll_proc;

  Inc(ticklooper);
  If (ticklooper >= IRQ_freq DIV tempo) then
    ticklooper := 0;

  Inc(macro_ticklooper);
  If (macro_ticklooper >= IRQ_freq DIV (tempo*_macro_speedup)) then
    macro_ticklooper := 0;

end;

procedure init_irq;
begin
  If irq_initialized then EXIT;
  irq_initialized := TRUE;
  update_timer(50);
end;

procedure done_irq;
begin
  If NOT irq_initialized then EXIT;
  irq_initialized := FALSE;
  irq_mode := TRUE;
  update_timer(0);
  irq_mode := FALSE;
end;

procedure init_buffers;

var
  temp: Byte;

begin
  FillChar(fmpar_table,SizeOf(fmpar_table),0);
  FillChar(pan_lock,SizeOf(pan_lock),BYTE(panlock));
  FillChar(volume_table,SizeOf(volume_table),0);
  FillChar(vscale_table,SizeOf(vscale_table),0);
  FillChar(modulator_vol,SizeOf(modulator_vol),0);
  FillChar(carrier_vol,SizeOf(carrier_vol),0);
  FillChar(event_table,SizeOf(event_table),0);
  FillChar(freq_table,SizeOf(freq_table),0);
  FillChar(effect_table,SizeOf(effect_table),0);
  FillChar(effect_table2,SizeOf(effect_table2),0);
  FillChar(fslide_table,SizeOf(fslide_table),0);
  FillChar(fslide_table2,SizeOf(fslide_table2),0);
  FillChar(porta_table,SizeOf(porta_table),0);
  FillChar(porta_table2,SizeOf(porta_table2),0);
  FillChar(arpgg_table,SizeOf(arpgg_table),0);
  FillChar(arpgg_table2,SizeOf(arpgg_table2),0);
  FillChar(vibr_table,SizeOf(vibr_table),0);
  FillChar(vibr_table2,SizeOf(vibr_table2),0);
  FillChar(trem_table,SizeOf(trem_table),0);
  FillChar(trem_table2,SizeOf(trem_table2),0);
  FillChar(retrig_table,SizeOf(retrig_table),0);
  FillChar(retrig_table2,SizeOf(retrig_table2),0);
  FillChar(tremor_table,SizeOf(tremor_table),0);
  FillChar(tremor_table2,SizeOf(tremor_table2),0);
  FillChar(panning_table,SizeOf(panning_table),0);
  FillChar(last_effect,SizeOf(last_effect),0);
  FillChar(last_effect2,SizeOf(last_effect2),0);
  FillChar(voice_table,SizeOf(voice_table),0);
  FillChar(event_new,SizeOf(event_new),0);
  FillChar(notedel_table,SizeOf(notedel_table),NONE);
  FillChar(notecut_table,SizeOf(notecut_table),NONE);
  FillChar(ftune_table,SizeOf(ftune_table),0);
  FillChar(loopbck_table,SizeOf(loopbck_table),NONE);
  FillChar(loop_table,SizeOf(loop_table),NONE);
  FillChar(reset_chan,SizeOf(reset_chan),BYTE(FALSE));
  FillChar(keyoff_loop,SizeOf(keyoff_loop),BYTE(FALSE));
  FillChar(macro_table,SizeOf(macro_table),0);

  If NOT lockvol then FillChar(volume_lock,SizeOf(volume_lock),0)
  else For temp := 1 to 20 do volume_lock[temp] := BOOLEAN(songdata.lockflags[temp] SHR 4 AND 1);

  If NOT panlock then FillChar(panning_table,SizeOf(panning_table),0)
  else For temp := 1 to 20 do panning_table[temp] := songdata.lockflags[temp] AND 3;

  If NOT lockVP then FillChar(peak_lock,SizeOf(peak_lock),0)
  else For temp := 1 to 20 do peak_lock[temp] := BOOLEAN(songdata.lockflags[temp] SHR 5 AND 1);

  For temp := 1 to 20 do
    volslide_type[temp] := songdata.lockflags[temp] SHR 2 AND 3;
end;

procedure init_player;

var
  temp: Byte;

begin
  opl2out($01,0);

  For temp := 1 to 18 do opl2out($0b0+_chan_n[temp],0);
  For temp := $080 to $08d do opl2out(temp,NONE);
  For temp := $090 to $095 do opl2out(temp,NONE);

  misc_register := tremolo_depth SHL 7+
                   vibrato_depth SHL 6+
                   BYTE(percussion_mode) SHL 5;

  opl2out($01,$20);
  opl2out($08,$40);
  opl3exp($0105);
  opl3exp($04+songdata.op4flags SHL 8);

  key_off(17);
  key_off(18);
  opl2out(_instr[11],misc_register);
  init_buffers;

  If NOT percussion_mode then
    begin
      _chan_n := _chmm_n;
      _chan_m := _chmm_m;
      _chan_c := _chmm_c;
    end
  else
    begin
      _chan_n := _chpm_n;
      _chan_m := _chpm_m;
      _chan_c := _chpm_c;
    end;

  current_tremolo_depth := tremolo_depth;
  current_vibrato_depth := vibrato_depth;
  global_volume := 63;

  For temp := 1 to 20 do
    begin
      arpgg_table[temp].state := 1;
      voice_table[temp] := temp;
    end;
end;

procedure a2t_data_import(var data; size: Word); forward;
procedure start_playing(var data; size: Word);
begin
  stop_playing;
  a2t_data_import(data,size);

  If (error_code <> 0) then EXIT
  else init_player;

  current_order := 0;
  If (songdata.pattern_order[current_order] > $7f) then
    If (calc_order_jump = -1) then EXIT;

  current_pattern := songdata.pattern_order[current_order];
  current_line := 0;
  pattern_break := FALSE;
  pattern_delay := FALSE;
  tickXF := 0;
  ticks := 0;
  tick0 := 0;
  next_line := 0;
  irq_mode := TRUE;
  play_status := isPlaying;

  ticklooper := 0;
  macro_ticklooper := 0;
  speed := songdata.speed;
  macrospeedup := songdata.macrospeedup;
  update_timer(songdata.tempo);
end;

procedure stop_playing;

var
  temp: Byte;

begin
  irq_mode := FALSE;
  play_status := isStopped;
  global_volume := 63;
  current_tremolo_depth := tremolo_depth;
  current_vibrato_depth := vibrato_depth;
  pattern_break := FALSE;
  current_order := 0;
  current_pattern := 0;
  current_line := 0;

  For temp := 1 to 20 do release_sustaining_sound(temp);
  opl2out(_instr[11],0);
  opl3exp($0004);
  opl3exp($0005);
  init_buffers;

  speed := songdata.speed;
  update_timer(songdata.tempo);
end;

procedure init_songdata;
begin
  If (play_status <> isStopped) then stop_playing
  else init_buffers;

  FillChar(songdata,SizeOf(songdata),0);
  FillChar(songdata.pattern_order,SizeOf(songdata.pattern_order),$080);
  FillChar(pattdata^,PATTERN_SIZE*max_patterns,0);

  songdata.pattlen := 64;
  songdata.noftracks := 18;
  songdata.tempo := tempo;
  songdata.speed := speed;
  songdata.macrospeedup := 1;
  speed_update := FALSE;
  lockvol := FALSE;
  panlock := FALSE;
  lockVP  := FALSE;
  tremolo_depth := 0;
  vibrato_depth := 0;
  volume_scaling := FALSE;

  If (songdata.noftracks <= 18) then
    begin
      percussion_mode := FALSE;
      _chan_n := _chmm_n;
      _chan_m := _chmm_m;
      _chan_c := _chmm_c;
    end
  else
    begin
      percussion_mode := TRUE;
      _chan_n := _chpm_n;
      _chan_m := _chpm_m;
      _chan_c := _chpm_c;
    end;
end;

procedure make_CRC32_table;

var
  crc: Dword;
  n,index: Byte;

begin
  For index := 0 to 255 do
    begin
      crc := index;
      For n := 1 to 8 do
        If Odd(crc) then crc := crc SHR 1 XOR $0edb88320
        else crc := crc SHR 1;
      CRC32_table[index] := crc;
    end;
end;

function update_CRC32(var buf; size: Longint; crc: Longint): Longint; 
begin
asm
        mov     esi,[buf]
        lea     edi,[CRC32_table]
        mov     ebx,crc
        mov     ecx,size
        jecxz   @@2
@@1:    xor     eax,eax
        lodsb
        xor     ebx,eax
        mov     edx,ebx
        and     ebx,000000ffh
        shl     ebx,2
        mov     ebx,[edi+ebx]
        shr     edx,8
        and     edx,00ffffffh
        xor     ebx,edx
        loop    @@1
@@2:    mov     eax,ebx
end ['eax','ebx','ecx','edx','esi','edi']
end;

function APACK_decompress(var source,dest): Longint; 
var dummy:dword;
begin
asm
        mov     esi,[source]
        mov     edi,[dest]
        cld
        mov     dl,80h
@@1:    movsb
@@2:    add     dl,dl
        jnz     @@3
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@3:    jnc     @@1
        xor     ecx,ecx
        add     dl,dl
        jnz     @@4
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@4:    jnc     @@8
        xor     eax,eax
        add     dl,dl
        jnz     @@5
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@5:    jnc     @@15
        inc     ecx
        mov     al,10h
@@6:    add     dl,dl
        jnz     @@7
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@7:    adc     al,al
        jnc     @@6
        jnz     @@24
        stosb
        jmp     @@2
@@8:    inc     ecx
@@9:    add     dl,dl
        jnz     @@10
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@10:   adc     ecx,ecx
        add     dl,dl
        jnz     @@11
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@11:   jc      @@9
        dec     ecx
        loop    @@16
        xor     ecx,ecx
        inc     ecx
@@12:   add     dl,dl
        jnz     @@13
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@13:   adc     ecx,ecx
        add     dl,dl
        jnz     @@14
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@14:   jc      @@12
        jmp     @@23
@@15:   lodsb
        shr     eax,1
        jz      @@25
        adc     ecx,ecx
        jmp     @@20
@@16:   xchg    eax,ecx
        dec     eax
        shl     eax,8
        lodsb
        xor     ecx,ecx
        inc     ecx
@@17:   add     dl,dl
        jnz     @@18
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@18:   adc     ecx,ecx
        add     dl,dl
        jnz     @@19
        mov     dl,[esi]
        inc     esi
        adc     dl,dl
@@19:   jc      @@17
        cmp     eax,32000
        jae     @@20
        cmp     ah,5
        jae     @@21
        cmp     eax,7fh
        ja      @@22
@@20:   inc     ecx
@@21:   inc     ecx
@@22:   xchg    eax,dummy
@@23:   mov     eax,dummy
@@24:   push    esi
        mov     esi,edi
        sub     esi,eax
        rep     movsb
        pop     esi
        jmp     @@2
@@25:   sub     edi,[dest]
        mov     eax,edi
        jmp     @ret

@ret:   
end ['eax','ebx','ecx','edx','esi','edi'];
end;

procedure a2t_data_import(var data; size: Word);

type
  tHEADER = Record
              ident: array[1..15] of Char;
              crc32: Longint;
              ffver: Byte;
              patts: Byte;
              tempo: Byte;
              speed: Byte;
              cflag: Byte;
              patln: Word;
              nmtrk: Byte;
              mcspd: Word;
              is4op: Byte;
              locks: array[1..20] of Byte;
              b0len: Longint;
              b1len: Longint;
              b2len: Longint;
              b3len: Longint;
              b4len: Longint;
              b5len: array[0..15] of Longint;
            end;
const
  id = '_A2tiny_module_';

var
  temp,temp2,len: Longint;
  crc: Longint;

begin
  If (error_code = -2) then EXIT;
  error_code := -1;
  If NOT ((size >= SizeOf(tHEADER)+tHEADER(data).b0len
                                  +tHEADER(data).b1len
                                  +tHEADER(data).b2len
                                  +tHEADER(data).b3len
                                  +tHEADER(data).b4len) and
     (tHEADER(data).ident = id) and
     (tHEADER(data).ffver = 11)) then EXIT;

  crc := NOT 0;
  crc := update_CRC32(tDUMMY_BUFF(data)[SizeOf(tHEADER)],
                      size-SizeOf(tHEADER),crc);

  crc := update_CRC32(tHEADER(data).b0len,2,crc);
  crc := update_CRC32(tHEADER(data).b1len,2,crc);
  crc := update_CRC32(tHEADER(data).b2len,2,crc);
  crc := update_CRC32(tHEADER(data).b3len,2,crc);
  crc := update_CRC32(tHEADER(data).b4len,2,crc);

  For temp2 := 0 to 15 do
    crc := update_CRC32(tHEADER(data).b5len[temp2],2,crc);

  If (crc <> tHEADER(data).crc32) then EXIT;
  init_songdata;

  songdata.tempo := tHEADER(data).tempo;
  songdata.speed := tHEADER(data).speed;
  songdata.flags := tHEADER(data).cflag;
  songdata.pattlen := tHEADER(data).patln;
  songdata.noftracks := tHEADER(data).nmtrk;
  songdata.macrospeedup := tHEADER(data).mcspd;
  songdata.op4flags:= tHEADER(data).is4op;
  Move(tHEADER(data).locks,songdata.lockflags,SizeOf(songdata.lockflags));

  temp := APACK_decompress(tDUMMY_BUFF(data)[SizeOf(tHEADER)],
                           songdata.instr_data);
  If (temp = 0) then EXIT;

  temp := APACK_decompress(tDUMMY_BUFF(data)[SizeOf(tHEADER)+tHEADER(data).b0len],
                           songdata.instr_macros);
  If (temp = 0) then EXIT;

  temp := APACK_decompress(tDUMMY_BUFF(data)[SizeOf(tHEADER)+tHEADER(data).b0len
                                                            +tHEADER(data).b1len],
                           songdata.macro_table);
  If (temp = 0) then EXIT;

  temp := APACK_decompress(tDUMMY_BUFF(data)[SizeOf(tHEADER)+tHEADER(data).b0len
                                                            +tHEADER(data).b1len
                                                            +tHEADER(data).b2len],
                           songdata.dis_fmreg_col);
  If (temp = 0) then EXIT;

  temp := APACK_decompress(tDUMMY_BUFF(data)[SizeOf(tHEADER)+tHEADER(data).b0len
                                                            +tHEADER(data).b1len
                                                            +tHEADER(data).b2len
                                                            +tHEADER(data).b3len],
                           songdata.pattern_order);
  If (temp = 0) then EXIT;

  temp := APACK_decompress(tDUMMY_BUFF(data)[SizeOf(tHEADER)+tHEADER(data).b0len
                                                            +tHEADER(data).b1len
                                                            +tHEADER(data).b2len
                                                            +tHEADER(data).b3len
                                                            +tHEADER(data).b4len],
                           pattdata^[0]);
  If (temp = 0) then EXIT;

  len := SizeOf(tHEADER)+tHEADER(data).b0len
                        +tHEADER(data).b1len
                        +tHEADER(data).b2len
                        +tHEADER(data).b3len
                        +tHEADER(data).b4len
                        +tHEADER(data).b5len[0];

  For temp2 := 1 to 15 do
    begin
      If (PRED(tHEADER(data).patts) DIV 8 > PRED(temp2)) then
        begin
          temp := 0;
          If (temp2*8+8 <= max_patterns) then
            temp := APACK_decompress(tDUMMY_BUFF(data)[len],
                                     pattdata^[temp2]);
          If (temp = 0) then EXIT;
        end;
      Inc(len,tHEADER(data).b5len[temp2]);
    end;

  error_code := 0;
  tempo := songdata.tempo;
  speed := songdata.speed;

  speed_update    := BOOLEAN(songdata.flags AND 1);
  lockvol         := BOOLEAN(songdata.flags SHR 1 AND 1);
  lockVP          := BOOLEAN(songdata.flags SHR 2 AND 1);
  tremolo_depth   :=         songdata.flags SHR 3 AND 1;
  vibrato_depth   :=         songdata.flags SHR 4 AND 1;
  panlock         := BOOLEAN(songdata.flags SHR 5 AND 1);
  percussion_mode := BOOLEAN(songdata.flags SHR 6 AND 1);
  volume_scaling  := BOOLEAN(songdata.flags SHR 7 AND 1);
end;

var
  old_exit_proc: Pointer;
  temp: Byte;

procedure new_exit_proc;
begin
  stop_playing;
  done_irq;
  FreeMem(pattdata,PATTERN_SIZE*max_patterns);
end;

begin
  old_exit_proc := ExitProc;
  ExitProc := @new_exit_proc;

  error_code := 0;
  temp := $80;
{  Repeat
    If (MemAvail > PATTERN_SIZE*temp) then
      begin}
        max_patterns := temp;
{        BREAK;
      end
    else If (temp-$10 >= $10) then Dec(temp,$10)
         else begin
                error_code := -2;
                BREAK;
              end;
  until FALSE;}

  If (error_code <> -2) then
    GetMem(pattdata,PATTERN_SIZE*max_patterns);

  play_status := isStopped;
  init_songdata;
  init_irq;
  make_CRC32_table;
end.

-- The replay routine is pretty large, imnho :) No time to make
-- 32-bit PMODE ASM port of it... anyway, you can still use some kind
-- of exe-packer (UPX 1.20 or newer is recommended ;)
-- After compression, your target file should increase
-- cca 13k when linked with TMTPC, and cca 14k when linked with FPC,
-- depending on used exe-packer (reference values were rated for UPX 1.20d)
