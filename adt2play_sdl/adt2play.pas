//  This file is part of Adlib Tracker II (AT2).
//
//  AT2 is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  AT2 is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with AT2.  If not, see <http://www.gnu.org/licenses/>.

program AdT2_Player;
{$S-,Q-,R-,V-,B-,X+}
{$PACKRECORDS 1}
uses
  SDL,SDL_Timer,SDL_Video,SDL_Keyboard,SDL_Events,SysUtils,
  A2player,A2fileIO,A2scrIO,A2data,AdT2play_sdl_audio,
  StringIO,TxtScrIO;

const
  VERSION_STR = '0.47';

const
  modname: array[1..15] of String[39] = (
    '/�DLiB TR/�CK3R ][ module',
    '/�DLiB TR/�CK3R ][ G3 module',
    '/�DLiB TR/�CK3R ][ tiny module',
    '/�DLiB TR/�CK3R ][ G3 tiny module',
    'Amusic module',
    'XMS-Tracker module',
    'BoomTracker 4.0 module',
    'Digital-FM module',
    'HSC AdLib Composer / HSC-Tracker module',
    'MPU-401 tr�kk�r module',
    'Reality ADlib Tracker module',
    'Scream Tracker 3.x module',
    'FM-Kingtracker module',
    'Surprise! AdLib Tracker module',
    'Surprise! AdLib Tracker 2.0 module');

const
  SCREEN_W = 640;
  SCREEN_H = 400;

const
  kBkSPC  = SDLK_BACKSPACE;
  kESC    = SDLK_ESCAPE;
  kENTER  = SDLK_RETURN;

type
  PByteArray = ^TByteArray;
  TByteArray = array[0..$FFFF] of Byte;

  tFontBytes = array[0..4095] of Byte;

var
  fkey: Longint;
  space_down: Boolean;
  index,last_order: Byte;
  dirinfo: TSearchRec;

var
  temp,temp2: Byte;
  _ParamStr: array[0..255] of String[80];

const
  jukebox: Boolean = FALSE;

var
  screen: PSDL_Surface;

const
  vga_palette: array[0..15,0..2] of Byte = (
    (  0,  0,  0), (  0,  0,170), (  0,170,  0), (  0,170,170),
    (170,  0,  0), (170,  0,170), (170, 85,  0), (170,170,170),
    ( 85, 85, 85), ( 85, 85,255), ( 85,255, 85), ( 85,255,255),
    (255, 85, 85), (255, 85,255), (255,255, 85), (255,255,255));

function _gfx_mode: Boolean;

var
  result: Boolean;
  temp: Byte;

begin
  result := FALSE;
  For temp := 1 to ParamCount do
    If (Lower(_ParamStr[temp]) = '/gfx') then
      begin
        result := TRUE;
        BREAK;
      end;
  _gfx_mode := result;
end;

procedure _list_title;
begin
  If iVGA then
    begin
      CWriteLn('',$07,0);
      CWriteLn('   subz3ro''s',$09,0);
      CWriteLn('       ���       ��',$09,0);
      CWriteLn('  /�DLiB�R/�CK3R �� PLAYER',$09,0);
      CWriteLn('   �       �     ��   '+VERSION_STR,$09,0);
      CWriteLn('',$07,0);
    end
  else begin
         WriteLn;
         WriteLn('   subz3ro''s');
         WriteLn('       ���       ��');
         WriteLn('  /�DLiB�R/�CK3R �� PLAYER');
         WriteLn('   �       �     ��   '+VERSION_STR);
         WriteLn;
       end;
end;

procedure set_sdl_palette;

var
  i: Byte;

begin
  For i := 0 to 15 do
    begin
      screen^.format^.palette^.colors^[i].r := vga_palette[i,0];
      screen^.format^.palette^.colors^[i].g := vga_palette[i,1];
      screen^.format^.palette^.colors^[i].b := vga_palette[i,2];
      screen^.format^.palette^.colors^[i].unused := 0;
    end;
end;

procedure draw_text_screen;

var
  x,y: Byte;
  byte_pos,bit: Byte;
  ch,attr: Byte;
  b: Byte;
  pixels: PByteArray;
  pitch: Word;
  fb: ^tFontBytes;

begin
  SDL_LockSurface(screen);
  pixels := screen^.pixels;
  pitch := screen^.pitch;
  fb := @font8x16;

  For y := 0 to PRED(MaxLn) do
    For byte_pos := 0 to 15 do
      For x := 0 to PRED(MaxCol) do
        begin
          ch := text_screen_shadow[(x + y*Longint(MaxCol))*2];
          attr := text_screen_shadow[(x + y*Longint(MaxCol))*2 + 1];
          b := fb^[ch*16 + byte_pos];
          For bit := 0 to 7 do
            If (b AND (1 SHL (7 - bit))) <> 0 then
              pixels^[x*8 + bit + (y*16 + byte_pos)*pitch] := attr AND $0f
            else
              pixels^[x*8 + bit + (y*16 + byte_pos)*pitch] := attr SHR 4;
        end;

  SDL_UnlockSurface(screen);
  SDL_Flip(screen);
end;

procedure draw_picture_screen;

var
  x,y: Word;
  pixels: PByteArray;
  pitch: Word;

begin
  SDL_LockSurface(screen);
  pixels := screen^.pixels;
  pitch := screen^.pitch;

  For y := 0 to PRED(200) do
    For x := 0 to PRED(320) do
      pixels^[x + (y + (SCREEN_H - 200) DIV 2)*pitch] :=
        vmem_320x200[x + y*320];

  SDL_UnlockSurface(screen);
  SDL_Flip(screen);
end;

procedure handle_events;

var
  event: SDL_Event;

begin
  fkey := WORD_NULL;
  While (SDL_PollEvent(@event) <> 0) do
    Case event.eventtype of
      SDL_KEYDOWN:
        Case event.key.keysym.sym of
          SDLK_SPACE:    space_down := TRUE;
          SDLK_BACKSPACE: fkey := kBkSPC;
          SDLK_RETURN:    fkey := kENTER;
          SDLK_ESCAPE:    fkey := kESC;
        end;
      SDL_KEYUP:
        If (event.key.keysym.sym = SDLK_SPACE) then
          space_down := FALSE;
      SDL_EVENTQUIT: fkey := kESC;
    end;
end;

procedure vid_init;

var
  sdl_subsys: Longint;

begin
  sdl_subsys := SDL_INIT_VIDEO or SDL_INIT_TIMER or SDL_INIT_AUDIO;
  SDL_Init(sdl_subsys);
  screen := SDL_SetVideoMode(SCREEN_W,SCREEN_H,8,SDL_SWSURFACE);
  If (screen = NIL) then
    begin
      WriteLn('SDL: unable to set video mode');
      HALT(1);
    end;
  set_sdl_palette;
end;

procedure vid_done;
begin
  SDL_Quit;
end;

begin
  For temp := 0 to 255 do
    _ParamStr[temp] := ParamStr(temp);

  If _gfx_mode then _list_title;

  For temp := 1 to ParamCount do
    If (Lower(_ParamStr[temp]) = '/jukebox') then
      jukebox := TRUE;

  For temp := 1 to ParamCount do
    If (Lower(_ParamStr[temp]) = '/latency') then
      opl3out := opl2out;

  index := 0;
  If (ParamCount = 0) then
    begin
      If _gfx_mode then _list_title;
      CWriteLn('Syntax: '+BaseNameOnly(_ParamStr[0])+' files|wildcards [files|wildcards{...}] [options]',$07,0);
      CWriteLn('',$07,0);
      CWriteLn('Command-line options:',$07,0);
      CWriteLn('  /jukebox    play modules w/ no repeat',$07,0);
      CWriteLn('  /gfx        graphical interface',$07,0);
      CWriteLn('  /latency    compatibility mode for OPL3 latency',$07,0);
      HALT;
    end;

  vid_init;

  error_code := 0;

  If (error_code <> -2) then
    GetMem(pattdata,PATTERN_SIZE*128);

  FillChar(decay_bar,SizeOf(decay_bar),0);
  play_status := isStopped;
  init_songdata;
  init_timer_proc;
  snd_init;

  If _gfx_mode then
    toggle_picture_mode
  else begin
         FillWord(screen_ptr^,MAX_SCREEN_MEM_SIZE DIV 2,$0700);
         GotoXY(1,1);
         _list_title;
       end;

  Repeat
    If NOT (index <> 0) then
      begin
        CWriteLn(FilterStr(DietStr('��-�--��   ��-��������������--��       ��-���--�� ��-��-���',
                                   PRED(MaxCol)),
                           '.',' '),$01,0);
        CWriteLn(                  '  ~[~SPACE~]~ Fast-Forward ~[~�~]~ Restart ~[~��~]~ Next ~[~ESC~]~ Quit',$09,$01);
        CWriteLn(FilterStr(DietStr('��-����������--��      ��-�����������������������������-���',
                                   PRED(MaxCol)),
                           '.',' '),$01,0);

        CWriteLn('',$07,0);
        _window_top := WhereY;
      end;

    Inc(index);
    If (_ParamStr[index][1] <> '/') then
      begin
        If (FindFirst(_ParamStr[index],faAnyFile AND NOT faVolumeID AND NOT faDirectory,dirinfo) <> 0) then
          begin
            CWriteLn(DietStr('ERROR(2) - No such file "'+
                             Lower(_ParamStr[index])+'"',
                      PRED(MaxCol)),$07,0);
            CWriteLn('',$07,0);
            FindClose(dirinfo);
            CONTINUE;
          end;

        Repeat
          If (ExtractFilePath(_ParamStr[index]) <> '') then
            songdata_source := Upper(ExtractFilePath(_ParamStr[index])+dirinfo.Name)
          else songdata_source := Upper(dirinfo.Name);

          C3Write(DietStr('Loading "'+songdata_source+'" (please wait)',
                           PRED(MaxCol)),$07,0,0);
          wtext2(_timer_xpos,_timer_ypos,_timer_str,_timer_color);
          wtext(_progress_xpos,_progress_ypos,_progress_str,_progress_color);
          wtext(_pos_str_xpos,_pos_str_ypos,_position_str2+'  ',_pos_str_color);
          wtext2(_fname_xpos,_fname_ypos,ExpStrR(ExtractFileName(songdata_source),12,' '),_fname_color);
          wtext(_pos_str_xpos,_pos_str_ypos,ExpStrR('Loading...',35,' '),_pos_str_color);

          load_flag := BYTE_NULL;
          _decay_bars_initialized := FALSE;

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
          If (load_flag = BYTE_NULL) or
             (load_flag = $7f) then
            begin
              CWriteLn(DietStr(ExpStrR('ERROR(3) - Invalid module ('+songdata_source+')',
                                       PRED(MaxCol),' '),
                               PRED(MaxCol)),$07,0);
              CWriteLn('',$07,0);
              CONTINUE;
            end;

          last_order := 0;
          entries := 0;
          count_order(entries);
          correction := calc_following_order(0);
          entries2 := entries;
          If (correction <> -1) then Dec(entries,correction)
          else entries := 0;
          CWriteLn(DietStr(ExpStrR('Playing '+modname[load_flag]+' "'+
                                   songdata_source+'"',
                                   PRED(MaxCol),' '),
                           PRED(MaxCol)),$07,0);
          temp2 := PRED(WhereY);

          If (entries = 0) then
            begin
              If NOT _picture_mode then GotoXY(1,temp2);
              CWriteLn(DietStr(ExpStrR('Playing '+modname[load_flag]+' "'+
                                       songdata_source+'"',
                                       PRED(MaxCol),' '),
                               PRED(MaxCol)),$08,0);
              CWriteLn(DietStr(ExpStrR(''+ExtractFileName(songdata_source)+' [stopped] ['+
                                       ExpStrL(Num2str(TRUNC(time_playing) DIV 60,10),2,'0')+
                                       ':'+ExpStrL(Num2str(TRUNC(time_playing) MOD 60,10),2,'0')+']',
                                       PRED(MaxCol),' '),
                               PRED(MaxCol)),$07,0);
              CWriteLn('',$07,0);
              CONTINUE;
            end;

          start_playing;
          set_overall_volume(63);

          Repeat
            handle_events;

            If (overall_volume = 63) then
              C3Write(DietStr(_position_str+'  ',PRED(MaxCol)),$0f,0,0);
            wtext2(_timer_xpos,_timer_ypos,_timer_str,_timer_color);
            wtext(_progress_xpos,_progress_ypos,_progress_str,_progress_color);
            wtext(_pos_str_xpos,_pos_str_ypos,_position_str2+'  ',_pos_str_color);

            If space_down then
              begin
                If (overall_volume > 32) then
                  For temp := 63 downto 32 do
                    begin
                      set_overall_volume(temp);
                      delay_counter := 0;
                      While (delay_counter < overall_volume DIV 20) do
                        begin
                          If timer_200hz_flag then
                            begin
                              timer_200hz_flag := FALSE;
                              Inc(delay_counter);
                              C3Write(DietStr(_position_str+'`',PRED(MaxCol)),$0f,0,0);
                              wtext2(_timer_xpos,_timer_ypos,_timer_str,_timer_color);
                              wtext(_progress_xpos,_progress_ypos,_progress_str,_progress_color);
                              wtext(_pos_str_xpos,_pos_str_ypos,_position_str2+'`',_pos_str_color);
                              If timer_50hz_flag then
                                begin
                                  timer_50hz_flag := FALSE;
                                  If NOT fast_forward then
                                    decay_bars_refresh;
                                end;
                            end;
                          SDL_Delay(1);
                        end;
                    end
                else begin
                       If timer_200hz_flag then
                         begin
                           timer_200hz_flag := FALSE;
                           C3Write(DietStr(_position_str+'`',PRED(MaxCol)),$0f,0,0);
                           wtext2(_timer_xpos,_timer_ypos,_timer_str,_timer_color);
                           wtext(_progress_xpos,_progress_ypos,_progress_str,_progress_color);
                           wtext(_pos_str_xpos,_pos_str_ypos,_position_str2+'`',_pos_str_color);
                           If timer_50hz_flag then
                             begin
                               timer_50hz_flag := FALSE;
                               If NOT fast_forward then
                                 decay_bars_refresh;
                             end;
                         end;
                     end;
                fast_forward := TRUE;
              end
            else If NOT space_down and fast_forward then
                   begin
                     fast_forward := FALSE;
                     If (overall_volume < 63) then
                       For temp := 32 to 63 do
                         begin
                           set_overall_volume(temp);
                           delay_counter := 0;
                            While (delay_counter < overall_volume DIV 20) do
                              begin
                                If (timer_200hz_counter = 0) then
                                  begin
                                    Inc(delay_counter);
                                    C3Write(DietStr(_position_str+'  ',PRED(MaxCol)),$0f,0,0);
                                    wtext2(_timer_xpos,_timer_ypos,_timer_str,_timer_color);
                                    wtext(_progress_xpos,_progress_ypos,_progress_str,_progress_color);
                                    wtext(_pos_str_xpos,_pos_str_ypos,_position_str2+'  ',_pos_str_color);
                                    If timer_50hz_flag then
                                      begin
                                        timer_50hz_flag := FALSE;
                                        decay_bars_refresh;
                                      end;
                                  end;
                                SDL_Delay(1);
                              end;
                         end;
                   end;

            If (NOT fast_forward and timer_50hz_flag) or
               (fast_forward and timer_20hz_flag) then
              begin
                If NOT fast_forward then timer_50hz_flag := FALSE
                else timer_20hz_flag := FALSE;
                decay_bars_refresh;
              end;

            If jukebox and (last_order <> current_order) then
              begin
                If (last_order > current_order) and
                   (last_order = PRED(entries2)) then BREAK
                else last_order := current_order;
              end;

            If (fkey = kBkSPC) then
              begin
                fade_out;
                stop_playing;
                set_overall_volume(63);
                start_playing;
              end;

            If _picture_mode then draw_picture_screen
            else draw_text_screen;
            SDL_Delay(20);

          until (fkey = kENTER) or
                (fkey = kESC);

          fade_out;
          stop_playing;
          If NOT _picture_mode then GotoXY(1,temp2);
          CWriteLn(DietStr(ExpStrR('Playing '+modname[load_flag]+' "'+
                                   songdata_source+'"',
                                   PRED(MaxCol),' '),
                           PRED(MaxCol)),$08,0);
          CWriteLn(DietStr(ExpStrR(''+ExtractFileName(songdata_source)+' [stopped] ['+
                                   ExpStrL(Num2str(TRUNC(time_playing) DIV 60,10),2,'0')+
                                   ':'+ExpStrL(Num2str(TRUNC(time_playing) MOD 60,10),2,'0')+']',
                                   PRED(MaxCol),' '),
                           PRED(MaxCol)),$07,0);
          CWriteLn('',$07,0);
          If (fkey = kESC) then BREAK;
        Until (FindNext(dirinfo) <> 0);
        FindClose(dirinfo);
      end;
  until (index = ParamCount);

  done_timer_proc;
  snd_done;
  vid_done;
end.
