unit TxtScrIO;
{$S-,Q-,R-,V-,B-,X+}
{$PACKRECORDS 1}
interface

const
  Black   = $00;  DGray    = $08;
  Blue    = $01;  LBlue    = $09;
  Green   = $02;  LGreen   = $0a;
  Cyan    = $03;  LCyan    = $0b;
  Red     = $04;  LRed     = $0c;
  Magenta = $05;  LMagenta = $0d;
  Brown   = $06;  Yellow   = $0e;
  LGray   = $07;  White    = $0f;
  Blink   = $80;

const
  MAX_SCREEN_MEM_SIZE = 180*60*2;
  SCREEN_MEM_SIZE: Longint = MAX_SCREEN_MEM_SIZE;

type
  tSCREEN_MEM = array[0..PRED(MAX_SCREEN_MEM_SIZE)] of Byte;
  tSCREEN_MEM_PTR = ^tSCREEN_MEM;

var
  text_screen_shadow: tSCREEN_MEM;

const
  screen_ptr: Pointer = Addr(text_screen_shadow);
  v_seg:  Word = $0b800;
  v_ofs:  Word = 0;
  v_mode: Byte = $03;
  MaxLn:  Byte = 25;
  MaxCol: Byte = 80;

var
  cursor_backup: Longint;
  DispPg: Byte;

type
  tFADE  = (first,fadeOut,fadeIn);
  tDELAY = (fast,delayed);

type
  tFADE_BUF = Record
                action: tFADE;
                pal0: array[0..255] of Record r,g,b: Byte end;
                pal1: array[0..255] of Record r,g,b: Byte end;
              end;
const
  fade_speed: Byte = 63;

type
  tVIDEO_STATE = Record
                   cursor: Longint;
                   font: Byte;
                   MaxLn,MaxCol,v_mode,DispPg: Byte;
                   v_seg,v_ofs: Word;
                   screen: tSCREEN_MEM;
                   data: array[0..PRED(4096)] of Byte;
                 end;

procedure ShowC3Str(var dest; x,y: Byte; str: String; atr1,atr2,atr3: Byte);
function  iVGA:  Boolean;
function  WhereX: Byte;
function  WhereY: Byte;
procedure GotoXY(x,y: Byte);
function  GetCursor: Longint;
procedure SetCursor(cursor: Longint);
procedure ThinCursor;
procedure WideCursor;
procedure HideCursor;
function  GetCursorShape: Word;
procedure SetCursorShape(shape: Word);
procedure GetRGBitem(color: Byte; var red,green,blue: Byte);
procedure SetRGBitem(color: Byte; red,green,blue: Byte);
procedure GetPalette(var pal; first,last: Word);
procedure SetPalette(var pal; first,last: Word);
procedure WaitRetrace;
procedure VgaFade(var data: tFADE_BUF; fade: tFADE; delay: tDELAY);
procedure GetVideoState(var data: tVIDEO_STATE);
procedure SetVideoState(var data: tVIDEO_STATE; restore_screen: Boolean);

implementation

var
  absolute_pos: Word;

const
  _cursor_x: Byte = 1;
  _cursor_y: Byte = 1;

procedure DupChar; assembler;
asm
        pushad
        xor     ebx,ebx
        xchg    ax,bx
        xor     eax,eax
        xchg    ax,bx
        mov     bl,al
        mov     al,MaxCol
        mul     ah
        add     ax,bx
        mov     bl,MaxCol
        sub     ax,bx
        dec     ax
        shl     ax,1
        jecxz   @@1
        add     edi,eax
        xchg    ax,dx
        rep     stosw
        xchg    ax,dx
@@1:    mov     absolute_pos,ax
        popad
end;

procedure ShowC3Str(var dest; x,y: Byte; str: String; atr1,atr2,atr3: Byte);
begin
  asm
        lea     esi,[str]
        mov     edi,dword ptr [dest]
        lodsb
        xor     ecx,ecx
        mov     cl,al
        jecxz   @@3
        push    ecx
        mov     al,x
        mov     ah,y
        xor     ecx,ecx
        call    DupChar
        xor     edx,edx
        mov     dx,absolute_pos
        pop     ecx
        add     edi,edx
        mov     ah,atr1
        mov     bl,atr2
        mov     bh,atr3
@@1:    lodsb
        cmp     al,'~'
        jz      @@2
        cmp     al,'`'
        jz      @@3
        stosw
        loop    @@1
        jmp     @@4
@@2:    xchg    ah,bl
        loop    @@1
        jmp     @@4
@@3:    xchg    ah,bh
        loop    @@1
@@4:
  end;
end;

procedure ScreenMemCopy(source,dest: tSCREEN_MEM_PTR);
begin
  cursor_backup := GetCursor;
  asm
        xor     edx,edx
        mov     eax,SCREEN_MEM_SIZE
        cmp     eax,16
        jb      @@1
        mov     ecx,4
        div     ecx
        mov     ecx,eax
        jecxz   @@1
        mov     esi,dword ptr [source]
        mov     edi,dword ptr [dest]
        cld
        rep     movsd
        mov     ecx,edx
        jecxz   @@2
        rep     movsb
        jmp     @@2
@@1:    mov     ecx,SCREEN_MEM_SIZE
        mov     esi,dword ptr [source]
        mov     edi,dword ptr [dest]
        cld
        rep     movsb
@@2:
  end;
end;

function WhereX: Byte;
begin
  WhereX := _cursor_x;
end;

function WhereY: Byte;
begin
  WhereY := _cursor_y;
end;

procedure GotoXY(x,y: Byte);
begin
  _cursor_x := x;
  _cursor_y := y;
end;

function GetCursor: Longint;
begin
  GetCursor := _cursor_x + _cursor_y SHL 8;
end;

procedure SetCursor(cursor: Longint);
begin
  _cursor_x := Byte(cursor);
  _cursor_y := Byte(cursor SHR 8);
end;

procedure ThinCursor;
begin
end;

procedure WideCursor;
begin
end;

procedure HideCursor;
begin
end;

function GetCursorShape: Word;
begin
  GetCursorShape := $0d0e;
end;

procedure SetCursorShape(shape: Word);
begin
end;

procedure initialize;
begin
  MaxCol := 80;
  MaxLn := 25;
  FillWord(screen_ptr^,MAX_SCREEN_MEM_SIZE DIV 2,$0700);
end;

function iVGA: Boolean;
begin
  iVGA := TRUE;
end;

procedure GetRGBitem(color: Byte; var red,green,blue: Byte);
begin
  red := 0; green := 0; blue := 0;
end;

procedure SetRGBitem(color: Byte; red,green,blue: Byte);
begin
end;

procedure GetPalette(var pal; first,last: Word);
begin
  FillChar(pal,SizeOf(pal),0);
end;

procedure SetPalette(var pal; first,last: Word);
begin
end;

procedure WaitRetrace;
begin
end;

procedure VgaFade(var data: tFADE_BUF; fade: tFADE; delay: tDELAY);
begin
  data.action := fade;
end;

procedure GetVideoState(var data: tVIDEO_STATE);
begin
  FillChar(data,SizeOf(data),0);
end;

procedure SetVideoState(var data: tVIDEO_STATE; restore_screen: Boolean);
begin
end;

begin
  initialize;
end.
