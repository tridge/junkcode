{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit MISC;

Interface

Uses
  DOS,
  CRT,
  INITIAL;

Procedure InitWindow(x1,y1,x2,y2,ForeColor,BackColor : integer);

Procedure ShowMessage(message : linetype);

Procedure PrepareWholeScreen;

Procedure PrepareWindowOne;

Procedure PrepareWindowtwo;

Procedure PrepareWindowthree;

Procedure Wait;

Procedure Ready;

Procedure SaveCursor;

Function DriveReady : Boolean;

Procedure ResetPos;

Procedure RewritePos;

Procedure ResetStat;

Procedure RewriteStat;

Procedure SaveStatus;

Procedure SaveGame;

Procedure RecallPosition(companyIndex : integer);

Procedure DoErrorMessage(message : linetype);

Procedure MakeHelpArray;

Function monthnum(month : monthtype) : integer;

Function days_to_expy(expymonthstr : integer) : integer;

{===========================================================================}

Implementation



Procedure InitWindow;

var
   Regs : Registers;
   Attr : byte;
begin
  Attr := GetAttr(ForeColor,BackColor);
  with Regs do
  begin
  AX := $0600;
  BX := Attr shl 8;
  CX := X1-1 + (Y1-1) shl 8;
  DX := X2-1 + (Y2-1) shl 8;
  end;
  Intr($10,Dos.Registers(Regs));
end;



Procedure PrepareWholeScreen;

begin
with Status.Colors do
InitWindow(1,1,80,25,CentreFore,CentreBack);
end;


Procedure PrepareWindowOne;

begin
with Status.Colors do
begin
InitWindow(1,1,80,1,CommandFore,CommandBack);
TextColor(CommandFore);
TextBackground(CommandBack);
end;
end;


Procedure PrepareWindowtwo;

begin
with Status.Colors do
begin
InitWindow(1,2,80,22,CentreFore,CentreBack);
TextColor(CentreFore);
TextBackground(CentreBack);
end;
end;


Procedure PrepareWindowthree;

begin
with Status.Colors do
begin
InitWindow(1,23,80,25,StockFore,StatusBack);
TextColor(StockFore);
TextBackground(StatusBack);
end;
end;

Procedure ShowMessage(message : linetype);

begin
 PrepareWindowthree;
 gotoXY(25,24);
 write(message);
end;



Procedure DoErrorMessage;

var
  ch : char;
begin
burp;
ShowMessage(message);
Ch := ReadKey;
end;


Procedure Wait;

begin
gotoXY(74,1);
textbackground(status.colors.centreback);
textcolor(status.colors.centrefore + blink);
write(' WAIT  ');
end;


Procedure Ready;

begin
gotoXY(74,1);
textbackground(status.colors.HighLightback);
textcolor(status.colors.HighLight);
write('F1 HELP');
end;

Procedure SaveCursor;

var
  regs : Registers;
begin
with regs do
begin
ax := $0300;
bx := 0;
intr($10,Dos.Registers(regs));
CursorTop := Hi(cx);
CursorBot := Lo(cx);
end;
end;



Function DriveReady;

var
  Regs : Registers;
  DefaultDrive : byte;
begin
with Regs do
begin
  ax := $1900;
  Intr($21,Dos.Registers(Regs));
{! 3. Param^eter to Intr must be of the type Registers defined in DOS unit.}
  DefaultDrive := lo(ax);
 If DefaultDrive <= 1 then
 begin
  AX := $0401;
  CX := $0101;
  DX := DefaultDrive;
  Intr($13,Dos.Registers(Regs));
  if hi(ax) >= 128 then
    DriveReady := false
   else
    DriveReady := True;
 end
  else DriveReady := True;
end;
end;


Procedure ResetPos;

var
  error : boolean;
begin
repeat
if DriveReady then
begin
  {$I-}
  Reset(PosSaveFile);
  {$i+}
  error := IOresult <> 0;
{! 4. IORes^ult now returns different values corresponding to DOS error codes.}
end
 else Error := True;
if error then
  DoErrorMessage('Disk Error : Press any key to retry');
until not error;
end;


Procedure RewritePos;

var
  error : boolean;
begin
repeat
if DriveReady then
begin
{$I-}
Rewrite(PosSaveFile);
{$i+}
error := IOresult = $F1;
{! 5. IOR^esult now returns different values corresponding to DOS error codes.}
end
 else Error := True;
if error then
  DoErrorMessage('Disk Error : Press any key to retry');
until not error;
end;


Procedure ResetStat;

var
  error : boolean;
begin
repeat
if DriveReady then
begin
{$I-}
Reset(StatSaveFile);
{$i+}
Error := IOresult <> 0;
{! 6. IOR^esult now returns different values corresponding to DOS error codes.}
end
 else Error := True;
if error then
  DoErrorMessage('Disk Error : Press any key to retry');
until not error;
end;


Procedure RewriteStat;

var
  error : boolean;
begin
repeat
if DriveReady then
begin
{$I-}
Rewrite(StatSaveFile);
error := IOresult = $F1;
{! 7. IOR^esult now returns different values corresponding to DOS error codes.}
end
 else Error := True;
{$i+}
if error then
  DoErrorMessage('Disk Error : Press any key to retry');
until not error;
end;

Procedure SaveStatus;

begin
assign(statsavefile, 'status.sav');
rewriteStat;
write(statsavefile,status);
close(StatSaveFile);
end;


Procedure SaveGame;

var
  found : boolean;
  dummypos : positionrec;
begin
assign(possavefile, 'position.sav');
while not DriveReady do
  DoErrorMessage('Drive not ready: Press any key to retry');
{$I-} reset(PosSaveFile); {$I+};
if IOresult <> 0 then rewritePos;
{! ^8. IOResult now returns different values corresponding to DOS error codes.}
found := false;
if not eof(possavefile) then
begin
read(possavefile,dummypos);
while not (eof(possavefile) or found) do
if (dummypos.companyname = position.companyname) then
begin
 found := true;
 seek(possavefile,filePos(possavefile)-1);
end
else read(possavefile, dummypos);
if eof(possavefile) and (dummypos.companyname = position.companyname)
then seek(possavefile,filesize(possavefile)-1);
end;
{$I-}
write(possavefile,position);
{$i+}
if IOresult <> 0 then DoErrorMessage('Disk Full : Press any key');
{! ^9. IOResult now returns different values corresponding to DOS error codes.}
close(PosSaveFile);
SaveStatus;
end;



Procedure RecallPosition(companyIndex : integer);

var
  count,index,monthcount : integer;
  commod : commodtype;
  temppointer,temppointer2 : transpointer;

begin
while not DriveReady do
  DoErrorMessage('Drive not ready: Press any key to retry');
assign(possavefile, 'position.sav');
{$I-}
reset(possavefile);
{$I+}
if IOResult = 0 then
{! ^10. IOResult now returns different values corresponding to DOS error codes.}
begin
for count := 1 to companyIndex do
read(possavefile,position);
SaveStatus;
close(possavefile);
with position do
For commod := Call to Put do
begin
index := 1;
for monthcount := 1 to 3 do
begin
new(temppointer);
if commod = Call then
          Callsheet[monthcount] := temppointer
                 else
          Putsheet[monthcount] := temppointer;
for count := 1 to (numstrikes-1) do
begin
temppointer^ := OptionList[commod,index];
index := index + 1;
new(temppointer^.nextrec);
temppointer := temppointer^.nextrec;
end;
temppointer^ := OptionList[commod,index];
index := index + 1;
temppointer^.nextrec := nil;
end;
end;
with position do
begin
new(callsheet[4]);
new(putsheet[4]);
temppointer := callsheet[4];
temppointer2 := putsheet[4];
for count := 1 to (numstrikes-1) do
begin
with temppointer^ do
begin
 strike := OptionList[call,count].strike;
 month := months[4];
 pending := 0;
 amount := 0;
 new(nextrec);
end;
with temppointer2^ do
begin
 strike := OptionList[put,count].strike;
 month := months[4];
 pending := 0;
 amount := 0;
 new(nextrec);
end;
temppointer := temppointer^.nextrec;
temppointer2 := temppointer2^.nextrec;
end;
with temppointer^ do
begin
 strike := OptionList[put,count].strike;
 month := months[4];
 pending := 0;
 amount := 0;
 nextrec := nil;
end;
with temppointer2^ do
begin
 strike := OptionList[put,count].strike;
 month := months[4];
 pending := 0;
 amount := 0;
 nextrec := nil;
end;
end;
ActiveCompany := true;
end;
end;

Procedure MakeHelpArray;

var
  TempHelpPointer : HelpPointer;
  DumLine : linetype;
  i : integer;
  HelpFile : text;
  Error : boolean;

begin
if DriveReady then
begin
 assign(helpfile,'Help.sim');
 {$I-}
 reset(HelpFile);
 {$I+}
 if IOresult <> 0 then HelpArray := nil
{! 1^2. IOResult now returns different values corresponding to DOS error codes.}
else
 begin
  new(HelpArray);
  TempHelpPointer := HelpArray;
  repeat
   repeat
    readln(HelpFile,DumLine);
   until (pos('COMMAND',Dumline) <> 0) or eof(HelpFile);
   i := 1;
   if not eof(HelpFile) then
   repeat
    TempHelpPointer^.HelpScreen[i] := DumLine;
    i := i + 1;
    readln(HelpFile,DumLine);
   until pos('END-OF-HELP',DumLine) <> 0;
   TempHelpPointer^.HelpScreen[i] := DumLine;
   new(TempHelpPointer^.NextHelp);
   TempHelpPointer := TempHelpPointer^.NextHelp;
  until eof(HelpFile);
  TempHelpPointer := nil;
  close(HelpFile);
 end;
end;
end;

Function monthnum;


var
  i : integer;
begin
i := 0;
repeat
i := i + 1;
until monthsarray[i] = month;
monthnum := i;
end;

Function days_to_expy(expymonthstr : integer) : integer;

var
  startday : integer;
  startmonth,expymonth,err : integer;
  startyear,expyyear : integer;
  tempmonth : monthtype;
  dummy : real;
  offset,firstday : integer;

begin
with status do
with position do
begin

val(copy(startdate,1,2),startday,err);
val(copy(startdate,6,2),startyear,err);
tempmonth := copy(startdate,3,3);
startmonth := monthnum(tempmonth);

expymonth := monthnum(months[expymonthstr]);

if (startmonth + round(currentday/30) - expymonth) > 1 then
expyyear := startyear + trunc(currentday/365.25) + 1
else expyyear := startyear + trunc(currentday/365.25);

dummy := serialdate(expyyear-40,((expymonth mod 12) + 1),1);

firstday := ((trunc(dummy) + 6) mod 7);
if firstday = 4 then offset := 7
else offset := (firstday + 3) mod 7;

days_to_expy := trunc(dummy - serialdate(startyear-40,startmonth,startday)
                                                    - currentday - offset -1);
end;
end;


End.
