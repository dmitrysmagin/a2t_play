unit ymf262;
interface

  const
     OPL3_SAMPLE_BITS = 16;
     OPL3_INTERNAL_FREQ = 14400000;

  type
     UINT8 = byte;
     UINT16 = word;
     UINT32 = dword;
     INT8 = char;
     INT16 = smallint;
     INT32 = longint;

  type
     OPL3SAMPLE = INT16;
  Type
     PINT16  = ^INT16;

  function YMF262Init(num:longint; clock:longint; rate:longint):longint;cdecl;external 'ymf262.dll';
  procedure YMF262Shutdown;cdecl;external 'ymf262.dll';
  procedure YMF262ResetChip(which:longint);cdecl;external 'ymf262.dll';
  function YMF262Write(which:longint; a:longint; v:longint):longint;cdecl;external 'ymf262.dll';
  function YMF262Read(which:longint; a:longint):byte;cdecl;external 'ymf262.dll';
  function YMF262TimerOver(which:longint; c:longint):longint;cdecl;external 'ymf262.dll';
  procedure YMF262UpdateOne(which:longint; buffer:PINT16; length:longint);cdecl;external 'ymf262.dll';

implementation

end.
