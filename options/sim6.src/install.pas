{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit INSTALL;

Interface

Uses
  Crt,
  Dos,
  INITIAL,
  MISC,
  IO;


Procedure Installation;

{===========================================================================}

Implementation



Procedure Installation;

VAR
  Toggle : toggleRec;
  ch,adapter : char;
  colorscreen,coprocessor,HardDisk : boolean;
  pathname : linetype;

Procedure BlankLine;
begin
gotoXY(1,whereY);
clreol;
end;

Procedure SaveStatus;
var
  StatSaveFile : file of statustype;
  found : boolean;
begin
assign(statsavefile, 'status.sav');
if DriveReady then
rewrite(statsavefile);
write(statsavefile,status);
close(StatSaveFile);
end;

Procedure InitcolorsColor;
begin
with status.HardWare.DefaultColors do
begin
Commandback := 7;
CentreBack := blue;
StatusBack := 7;
CommandFore := 0;
Highlight := lightcyan;
highlightback := red;
CentreFore := white;
HeadingFore := yellow;
StockFore := red;
DaysFore := blue;
AccountsFore := 0;
PriceFore := 0;
end;
Status.Colors := Status.HardWare.DefaultColors;
end;

Procedure InitcolorsMono;
begin
with status.Colors do
begin
Commandback := lightgray;
CentreBack := 0;
StatusBack := lightgray;
CommandFore := 0;
Highlight := white;
highlightback :=  0;
CentreFore := yellow;
HeadingFore := blue;
StockFore := 0;
DaysFore := 0;
AccountsFore := 0;
PriceFore := 0;
end;
Status.HardWare.DefaultColors := Status.Colors;
end;

Procedure InitColorsHerc;
begin
with status.Colors do
begin
Commandback := lightgray;
CentreBack := 0;
StatusBack := lightgray;
CommandFore := 0;
Highlight := white;
highlightback :=  0;
CentreFore := white;
HeadingFore := white;
StockFore := 0;
DaysFore := 0;
AccountsFore := 0;
PriceFore := 0;
end;
Status.HardWare.DefaultColors := Status.Colors;
end;

BEGIN             {install}
textmode(2);
InitColorsMono;
PrepareWholeScreen;
gotoXY(1,1);
Writeln;
writeln('What type of graphics adapter do you have?');
with Toggle do
  begin
   Line := 3;
   MenuStr := '  CGA  EGA  Hercules ';
   NoItems := 3;
   ItemSet := 'CEH';
  end;
Get_MenuItem(Toggle,adapter);
Blankline;
Writeln;
writeln('Do you have a color monitor?');
with Toggle do
  begin
   Line := 5;
   MenuStr := '  Yes  No ';
   NoItems := 2;
   ItemSet := 'YN';
  end;
Get_MenuItem(Toggle,ch);
blankline;
if upcase(ch) = 'Y' then
  colorscreen := true
 else
  colorscreen := false;
writeln;
writeln('Do you have a maths co-processor?');
with Toggle do
  begin
   Line := 7;
   MenuStr := '  Yes  No ';
   NoItems := 2;
   ItemSet := 'YN';
  end;
Get_MenuItem(Toggle,ch);
blankline;
if upcase(ch) = 'Y' then
  coprocessor := true
 else
  coprocessor := false;

writeln;
write('Press plus key until cursor disappears : Then press any key ');
with status.hardware do
begin
CursorTopHid := 0;
CursorBotHid := 0;
repeat
Hide;
Ch := ReadKey;
if ch = '+' then
  begin
  CursorTopHid := CursorTopHid + 1;
  if cursorTop = 16 then
    begin
    CursorTop := 0;
    CursorBot := (CursorBot + 1) mod 16;
    end;
  end;
until Ch <> '+';
end;



with Status.HardWare do
case upcase(adapter) of
  'C' : begin
          GraphicsCard := CGA;
          TextModeNo := 3;
          GraphModeNo := 4;
          MaxXgraf := 320;
          MaxYgraf := 200;
          If colorscreen then
           InitColorsColor
            else
           InitColorsMono;
        end;
  'E' : begin
          GraphicsCard := EGA;
          TextModeNo := 3;
          GraphModeNo := 4;
          MaxXgraf := 320;
          MaxYgraf := 200;
          If colorscreen then
           InitColorsColor
            else
           InitColorsMono;
         end;
   'H' : begin
          GraphicsCard := Hercules;
          TextModeNo := 2;
          GraphModeNo := 0;
          MaxXgraf := 720;
          MaxYgraf := 348;
          InitColorsHerc;
         end;
end;
SaveStatus;
END;     {install}





End.
