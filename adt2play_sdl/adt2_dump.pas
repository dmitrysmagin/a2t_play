program AdT2_Dump;
{$S-,Q-,R-,V-,B-,X+}
{$PACKRECORDS 1}

uses
  SysUtils,
  A2player,
  A2fileIO,
  OPL3EMU;

var
  outfd: Longint;
  filename, outfilename: String;
  entries: Byte;
  correction: Integer;
  dump_ticks: Longint;
  max_ticks: Longint;
  s: String;

procedure dump_opl2out(reg, data: Word);
begin
  // s := LowerCase(IntToHex(reg AND $1ff, 3) + ' ' + IntToHex(data AND $ff, 2)) + #13#10;
  // FileWrite(outfd, s[1], Length(s));
  shadow_regs[reg shr 8, reg and $ff] := data;
  OPL3EMU_WriteReg(reg, data);
end;

procedure dump_opl3out(reg, data: Word);
begin
  // s := LowerCase(IntToHex(reg AND $1ff, 3) + ' ' + IntToHex(data AND $ff, 2)) + #13#10;
  // FileWrite(outfd, s[1], Length(s));
  shadow_regs[reg shr 8, reg and $ff] := data;
  OPL3EMU_WriteReg(reg, data);
end;

procedure dump_opl3exp(data: Word);
begin
  // s := LowerCase(IntToHex((data AND $ff) OR $100, 3) + ' ' + IntToHex(data SHR 8, 2)) + #13#10;
  // FileWrite(outfd, s[1], Length(s));
  shadow_regs[1, data and $ff] := data shr 8;
  OPL3EMU_WriteReg((data AND $ff) OR $100, data SHR 8);
end;

procedure dump_snd_settimer(freq: Longint);
begin
end;

begin
  if ParamCount < 1 then
  begin
    WriteLn('Usage: adt2_dump <module_file> [output.reg]');
    Halt(1);
  end;

  filename := ParamStr(1);
  if ParamCount >= 2 then
    outfilename := ParamStr(2)
  else
    outfilename := filename + '.reg';

  OPL3EMU_init;

  opl2out := dump_opl2out;
  opl3out := dump_opl3out;
  opl3exp := dump_opl3exp;
  snd_SetTimer := dump_snd_settimer;

  GetMem(pattdata, PATTERN_SIZE * 128);

  FillChar(decay_bar, SizeOf(decay_bar), 0);
  play_status := isStopped;
  init_songdata;
  init_timer_proc;

  songdata_source := filename;
  load_flag := BYTE_NULL;

  a2m_file_loader;
  If (load_flag = BYTE_NULL) then a2t_file_loader;
  If (load_flag = BYTE_NULL) then amd_file_loader;
  If (load_flag = BYTE_NULL) then cff_file_loader;
  If (load_flag = BYTE_NULL) then dfm_file_loader;
  If (load_flag = BYTE_NULL) then mtk_file_loader;
  If (load_flag = BYTE_NULL) then rad_file_loader;
  If (load_flag = BYTE_NULL) then s3m_file_loader;
  If (load_flag = BYTE_NULL) then fmk_file_loader;
  If (load_flag = BYTE_NULL) then sat_file_loader;
  If (load_flag = BYTE_NULL) then sa2_file_loader;
  If (load_flag = BYTE_NULL) then hsc_file_loader;

  If (load_flag = BYTE_NULL) or (load_flag = $7f) then
  begin
    WriteLn('ERROR: Invalid module file "', filename, '"');
    Halt(2);
  end;

  entries := 0;
  count_order(entries);
  correction := calc_following_order(0);
  If (correction <> -1) then Dec(entries, correction)
  else entries := 0;

  If (entries = 0) then
  begin
    WriteLn('ERROR: No playable orders in "', filename, '"');
    Halt(3);
  end;

  outfd := FileCreate(outfilename);
  if outfd = -1 then
  begin
    WriteLn('ERROR: Cannot create "', outfilename, '"');
    Halt(4);
  end;

  WriteLn('Dumping "', filename, '" -> ', outfilename, ' ...');

  start_playing;
  set_overall_volume(63);

  dump_ticks := 0;
  max_ticks := IRQ_freq * 600;

  while (play_status = isPlaying) and (dump_ticks < max_ticks) do
  begin
    timer_poll_proc;
    Inc(dump_ticks);
  end;

  FileClose(outfd);

  stop_playing;
  done_timer_proc;
  FreeMem(pattdata);
  pattdata := NIL;

  WriteLn('Done: ', dump_ticks, ' IRQ ticks');
end.
