{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit IO;

Interface

Uses
  Crt,
  Dos,
  INITIAL,
  MISC,
  BLACK;



Function Abort: Boolean;

Function Exist(FileName : linetype) : boolean;

Function EndMenu(choice : char) : boolean;

Function InputInteger(col : integer) : integer;

Function InputReal(col : integer) : real;

Function InputDate(col : integer) : datetype;

Procedure execute(child,param : linetype);        {executes another program}

Procedure Hide;    {hides cursor}

Procedure UnHide;



Procedure ScrollWindow(n,x1,y1,x2,y2,ForeColor,BackColor : Integer);

Function DisplayRec(commod : commodtype; index : integer) : linetype;

Procedure ShowStatus;

Procedure ShowSheet(commod : commodtype);

Procedure ScrollUp(commod : commodtype);

Procedure ScrollDown(commod : commodtype);

Procedure MakeList;



Procedure ShowCommands(commands : linetype);

Procedure TitleScreen(Title: Str11; StartRow: byte);

Procedure DoTitleScreen;

Procedure ShowStr(var StrItem : Linetype; fgr,bgr,row,col : byte);

Procedure Get_KeyType(Var KeyHit : KeyType;
                      Var Arrow  : ArrowType;
                      Var ChHit  : Char      );

Procedure GetSizes(var toggle : togglerec);

Procedure Get_MenuItem(Toggle : ToggleRec; Var Choice: Char);

{===========================================================================}

Implementation


Function Abort: Boolean;

Const
     Escape = #27;
Var
   Ch : Char;
Begin
     Abort := false;
     If KeyPressed then
     begin
          Ch := ReadKey;
          If (Ch = Escape) and (not KeyPressed)
          then Abort := true
          else Write(#7);
     end;
end;




Function Exist(FileName : linetype) : boolean;

var
  handle : file;
begin
assign(handle,FileName);
  {$i-}
  reset(Handle);
  {$i+}
If IOresult = 0 then
begin
  close(handle);
  Exist := true;
end
else
  Exist := false;
end;


Function EndMenu(choice : char) : boolean;

begin
EndMenu := (upcase(choice) = 'Q')
           or
           (status.GotoMain and not (upcase(choice) in  [' ','+']));
end;


Function InputInteger(col : integer) : integer;

var
  instring : linetype;
  ch : char;
  temp,err : integer;
begin
instring := '';
repeat
 Ch := ReadKey;
 if (ch in ['0'..'9']) or ((length(instring) = 0) and (ch = '-'))  then
 begin
 instring := instring + ch;
 gotoXY(col,whereY);
 write(instring);
 end
 else if (ord(ch) = 8) and (length(instring) > 0) then
 begin
 instring[0] := pred(instring[0]);
 gotoXY(col,whereY);
 write(instring,' ');
 write(ch);
 end
 else if ord(ch) <> 13 then beep;
until ord(ch) = 13;
if instring = '' then InputInteger := 0
else
begin
val(instring,temp,err);
InputInteger := temp;
end;
end;


Function InputReal(col : integer) : real;

var
  instring : linetype;
  ch : char;
  Accepted : set of char;
  temp : real;
  err : integer;
begin
instring := '';
ch := ' ';
repeat
 Ch := ReadKey;
 if pos('.',instring) = 0 then
    Accepted := ['0'..'9','.']
   else
    Accepted := ['0'..'9'];
 if ch in Accepted then
 begin
 instring := instring + ch;
 gotoXY(col,whereY);
 write(instring);
 end
 else if (ord(ch) = 8) and (length(instring) > 0) then
 begin
 instring[0] := pred(instring[0]);
 gotoXY(col,whereY);
 write(instring,' ');
 write(ch);
 end
 else if ord(ch) <> 13 then beep;
until ord(ch) = 13;
if instring = '' then InputReal := 0
else
begin
instring := '0' + instring;
val(instring,temp,err);
InputReal := temp;
end;
end;


Function InputDate(col : integer) : datetype;

var
  instring : linetype;
  ch : char;
  temp : datetype;
  err,count : integer;
  day,year : integer;
  month : monthtype;
  DateOK,monthOK : boolean;
Function LowCase(ch : char) : char;
var
  temp : char;
begin
if ch in ['A'..'Z'] then temp := chr(ord(ch) + 32)
else temp := ch;
LowCase := temp;
end;

begin
repeat
instring := '';
gotoXY(col,whereY);
write('        ');
gotoXY(col,whereY);
repeat
 Ch := ReadKey;
 if upcase(ch) in ['0'..'9','A'..'Z'] then
     case length(instring) of
     0,1,5,6 : if ch in ['0'..'9'] then
           begin
           instring := instring + ch;
           gotoXY(col,whereY);
           write(instring);
           end;
     2  :  if upcase(ch) in ['A'..'Z'] then
           begin
           instring := instring + upcase(ch);
           gotoXY(col,whereY);
           write(instring);
           end;
    3,4 :  if upcase(ch) in ['A'..'Z' ] then
           begin
           instring := instring + lowcase(ch);
           gotoXY(col,whereY);
           write(instring);
           end;
    end
 else if (ord(ch) = 8) and (length(instring) > 0) then
 begin
 instring[0] := pred(instring[0]);
 gotoXY(col,whereY);
 write(instring,' ');
 write(ch);
 end
 else if ord(ch) <> 13 then beep;
until ord(ch) = 13;
if length(instring) < 7 then DateOK := false
else
begin
dateOK := true;
val(copy(instring,1,2),day,err);
val(copy(instring,6,2),year,err);
month := copy(instring,3,3);
monthOK := false;
count := 0;
repeat
count := count + 1;
if monthsarray[count] = month then monthOK := true;
until monthOK or (count = 13);
if not monthOK then dateOK := false
else
case count of
  1,3,5,7,8,10,12 :  if day > 31 then dateOK := false;
         4,6,9,11 :  if day > 30 then dateOK := false;
                2 :  if year mod 4 = 0 then
                        begin
                        if day > 29 then dateOK := false;
                        end
                       else
                        if day > 28 then dateOK := false;
end;
if (day < 1) then dateOK := false;
end;
if not dateOK then burp;
until dateOK;
InputDate := instring;
end;



Procedure execute(child,param : linetype);        {executes another program}

type
     regarray = array[0..6] of integer;
var
  regs : Registers;
  i : integer;
  paramblk : regarray;
  olddir, dir :linetype;
  envblk : linetype;
  dirln : byte absolute dir;

begin
  i := length(child);
  while ((child[i] <> '\') and (i >= 1)) do
  i := i-1;
  if child[i-1] <> ':' then i:= i-1;
  if i=-1 then i:= 0;
  dir := child;
  dirln := lo(i);
  child := child + #0;
  getdir(0,olddir);
  chdir(dir);
  with regs do
    begin
    paramblk[1]:= ofs(param);
    paramblk[2]:= seg(param);
     for i:= 3 to 6 do
     paramblk[i]:=-1;
    ds := seg(child);
    dx := ofs(child) + 1;
    ax := $4b00;
    es := seg(paramblk);
    bx := ofs(paramblk);
    envblk := 'path=' + chr(0) + chr(0);
    paramblk[0] := seg(envblk);
    msdos(Dos.Registers(regs));
    end;
    chdir(olddir);
end;


Procedure Hide;    {hides cursor}

var
  regs : Registers;
begin
with status.hardware do
with regs do
begin
ax := $0100;
cx := CursorTopHid shl 8 + CursorBotHid;
end;
intr($10,Dos.Registers(regs));
end;



Procedure UnHide;

var
  regs : Registers;
begin
with regs do
begin
ax := $100;
cx := CursorTop shl 8 + CursorBot;
end;
Intr($10,Dos.Registers(Regs));
end;




Procedure ScrollWindow(n,x1,y1,x2,y2,ForeColor,BackColor : Integer);

var
   Regs : Registers;
   Attr : byte;
begin
  Attr := GetAttr(ForeColor,BackColor);
  with Regs do
  begin
  if n > 0 then
    AX := $0600 + n
   else
    AX := $0700 - n;
  BX := Attr shl 8;
  CX := X1-1 + (Y1-1) shl 8;
  DX := X2-1 + (Y2-1) shl 8;
  end;
  Intr($10,Dos.Registers(Regs));
end;



Function DisplayRec(commod : commodtype; index : integer) : linetype;

var
  StrikeStr : string[5];
  IndexStr : string[2];
  BidStr : string[8];
  AskStr : string[8];
  ValImpStr : string[8];
  DelStr : string[8];
  GamStr : string[8];
  DecStr : string[8];
  HeldStr : string[7];
  PendStr : string[10];

begin
with position.optionlist[commod,index] do
begin
   Str(index:2,IndexStr);

   if strike < 10 then
   Str(strike:5:2,StrikeStr)
    else
   Str(strike:5:0,StrikeStr);

   Str(Bider:8:2,BidStr);
   Str(Asker:8:2,AskStr);

if position.RecalcUser then
begin
     ValImpStr := '         ';
     DelStr := '        ';
     GamStr := '        ';
     DecStr := '        ';
end
else
begin
     if position.volmethod = 'Implied' then
     begin
     if ImpliedVol > 0 then
      Str(ImpliedVol:8:2,ValImpStr)
     else
      ValImpStr := '          ';
     end;
     if position.volmethod <> 'Implied' then
     Str(Value:8:2,ValImpStr);

     Str(Delta:8:2,DelStr);
     Str(Gamma:8:2,GamStr);
     Str(Decay:8:2,DecStr);
end;
if amount = 0 then
     HeldStr := '       '
    else
     Str(amount:7,HeldStr);
if pending = 0 then
     PendStr := '          '
    else
     Str(pending:10,PendStr);
DisplayRec := IndexStr + ' ' + month + StrikeStr + BidStr + AskStr +
            ValImpStr + DelStr + GamStr + DecStr + HeldStr + PendStr + '     ';
end;
end;



Procedure ShowStatus;

var
  monthcount : integer;
begin
if position.StatusDisplay = Totals then
begin
PrepareWindowThree;
gotoXY(1,23);
textcolor(status.colors.stockfore);
writeln(' POSITION TOTALS');
recalc_Pos_Totals;
textcolor(status.colors.accountsfore);
with position do
begin
write(' Value : ',PosValue:8:2);
gotoXY(35,24);
writeln('Delta : ',PosDelta:8:2);
write(' Gamma : ',PosGamma:8:2);
gotoXY(35,25);
write('Decay : ',PosDecay:8:2);
end;
end
else
with position do
with status do
begin
PrepareWindowthree;
gotoXY(2,23);
textcolor(status.colors.stockfore);
with sharesheet do
begin
writeln(uppercase(companyname),' Stock Held : ',amount,'   Pending : ',pending);
textcolor(status.colors.accountsfore);
gotoXY(57,23);
write('Capital : $',capital:8:2);
gotoXY(57,24);
write('Margins : $',CompanyMargin:8:2);
gotoXY(57,25);
write('Pending : $',PendingCost:8:2);
textcolor(status.colors.pricefore);
gotoXY(2,24);
if intraarray[currenttime] >= 1 then
write('CURRENT PRICES  Bid : $',Bid:6:2,'  Ask : $',Ask:6:2)
else write('CURRENT PRICES  Bid : ',Bid:6:2,'c  Ask : ',Ask:6:2,'c');
end;
textcolor(status.colors.daysfore);
gotoXY(2,25);
write(TimeArray[currenttime],' ',daysarray[DayOfWeek],'  DAYS TO EXPIRATION : ');
for monthcount := 1 to 3 do
write(days_to_expy(monthcount):3,'  ');
end;
end;


Procedure ShowSheet(commod : commodtype);

var
   i,j,NumLines : integer;
begin
with position do
begin
if NumStrikes <= 6 then
   NumLines := NumStrikes*3 + 2
  else
   NumLines := 20;
WaitRetraceV;
Window2[0] := CommodScreens[commod,0];
Window2[1] := CommodScreens[commod,topline];
i := 2;
repeat
WaitRetraceV;
Window2[i] := CommodScreens[commod,i+position.topline-1];
if i+1 <= NumLines then
Window2[i+1] := CommodScreens[commod,i+position.topline];
i := i + 2;
until i > NumLines;
if NumLines < 20 then
  with status.colors do
  InitWindow(1,NumLines+3,80,22,CentreFore,CentreBack);
end;
end;



Procedure ScrollUp(commod : commodtype);

var
  i,j : integer;
begin
if position.topline = 1 then beep
else
with position do
begin
topline := topline - 1;
with status.colors do
ScrollWindow(-1,1,3,80,22,CentreFore,CentreBack);
WaitRetraceV;
Window2[1] := CommodScreens[commod,topline];
end;
end;


Procedure ScrollDown(commod : commodtype);

var
  i,j : integer;
begin
if (position.numstrikes <= 6) or (position.topline >= position.numstrikes*3-17) then
 beep
else
with position do
begin
topline := topline + 1;
with status.colors do
ScrollWindow(1,1,3,80,22,CentreFore,CentreBack);
WaitRetraceV;
Window2[20] := CommodScreens[commod,topline+19];
end;
end;


Procedure MakeList;

var
  i,count,index,monthcount,MainAttr,HeadingAttr : integer;
  commod : commodtype;
  temppointer : transpointer;
  PresentLine : linetype;
  HeadingString : linetype;
begin
with position do
with status do
begin
for commod := Call to Put do
begin
 count :=1;
for monthcount := 1 to 3 do
begin
if commod = Put then
temppointer := putsheet[monthcount]
else temppointer := Callsheet[monthcount];
repeat
OptionList[Commod,count] := temppointer^;
count := count + 1;
temppointer := temppointer^.nextrec;
until temppointer = nil;
end;
end;

with status.colors do
begin
HeadingAttr := GetAttr(HeadingFore,Centreback);
MainAttr := GetAttr(centrefore,centreback);
end;

HeadingString := 'PUTS  Strike    Bid     Ask';
if position.volmethod = 'Implied' then
HeadingString := HeadingString +
                 '    Vol%   Delta   Gamma   Decay   Held   Pending    '
else
HeadingString := HeadingString +
                 '   Value   Delta   Gamma   Decay   Held   Pending    ';
for i := 1 to length(HeadingString) do
         begin
         CommodScreens[Put,0,i,1] := ord(HeadingString[i]);
         CommodScreens[Put,0,i,2] := HeadingAttr;
         end;

HeadingString := 'CALLS Strike    Bid     Ask';
if position.volmethod = 'Implied' then
HeadingString := HeadingString +
                 '    Vol%   Delta   Gamma   Decay   Held   Pending    '
else
HeadingString := HeadingString +
                 '   Value   Delta   Gamma   Decay   Held   Pending    ';
for i := 1 to length(HeadingString) do
         begin
         CommodScreens[Call,0,i,1] := ord(HeadingString[i]);
         CommodScreens[Call,0,i,2] := HeadingAttr;
         end;

for commod := Call to Put do
begin
index := 1;
for count := 1 to (NumStrikes*3 + 2) do
  begin
  if (count = NumStrikes+1) or (count = 2*(NumStrikes+1)) then
  PresentLine := '                                        '
                +'                                        '
  else
  begin
  PresentLine := DisplayRec(commod,index);
  index := index + 1;
  end;
  for i := 1 to 2 do
  begin
  CommodScreens[commod,count,i,1] := ord(PresentLine[i]);
  CommodScreens[commod,count,i,2] := HeadingAttr;
  end;
  for i := 3 to length(PresentLine) do
  begin
  CommodScreens[commod,count,i,1] := ord(PresentLine[i]);
  CommodScreens[commod,count,i,2] := MainAttr;
  end;
  end;
  if NumStrikes < 6 then
  for count := (numstrikes*3 + 3) to 20 do
  CommodScreens[commod,count] := CommodScreens[commod,NumStrikes+1];
  end;
  TopLine := 1;
end;
end;





Procedure ShowCommands(commands : linetype);

var
  count : integer;
begin
 PrepareWindowone;
 gotoXY(2,1);
 write(commands);
end;


Procedure TitleScreen(Title: Str11; StartRow: byte);

Var
   n,len,StartCol,Col: Byte;
   Table : array[0..255] of BitMap absolute $F000:$FA6E;

Procedure PaintBigChar (Entry : BitMap);
Var
   OnePatternLine,Spot : 1..8;
   CurrentLine : Byte;
Begin
     For OnePatternLine := 1 to 8 do
     begin
          CurrentLine := Entry[OnePatternLine];
          For Spot := 8 Downto 1 do
          begin
               If Odd(CurrentLine) then
               begin
                    Gotoxy(Spot+Col-1,OnePatternLine+startRow-1);
                    Write(#219);
               end;
               CurrentLine := CurrentLine div 2;
          end;
     end;
end;

Begin
     len := Length(Title);
     StartCol := ((80-8*len) div 2) and $00FF;
     For n := 1 to len do
     begin
          Col := StartCol + 8*(n-1) + 1;
          PaintBigChar(Table[Ord(Title[n])]);
     end;
end;


Procedure DoTitleScreen;

Const
     Title1: Str11 = 'OPTIONS';
     Title2: Str11 = 'MARKET';
var
  Dum : char;
Begin
     TextColor(LightGreen);
     TextBackGround(Blue);
     ClrScr;
     TitleScreen(Title1,2);
     TitleScreen(Title2,11);
     TextColor(LightRed);
     Gotoxy(10,20);
     Write('Created by : Peter Buchen & Andrew Tridgell (Sydney University)');
     Gotoxy(64,25);
     TextColor(Yellow);
     Write('Hit any key ...');
     Dum := ReadKey;
end;


Procedure ShowStr(var StrItem : Linetype; fgr,bgr,row,col : byte);

Begin
     TextColor(fgr);
     TextBackGround(bgr);
     Gotoxy(Col,Row);
     Write(StrItem);
end;


Procedure Get_KeyType(Var KeyHit : KeyType;
                      Var Arrow  : ArrowType;
                      Var ChHit  : Char      );

Const
     Return = #13;
     Escape = #27;
     Rt_Aro = #77; Lf_Aro = #75;
     Up_Aro = #72; Dn_Aro = #80;
     HelpF1 = #59;
     NulKey = #0;

Var
   InChar : Char;
Begin
     KeyHit := NoKey;
     Arrow  := None;
     ChHit  := ' ';
     Repeat
       InChar := ReadKey;
           If InChar = NulKey then
           begin
                KeyHit := ArrowKey;
                InChar := ReadKey;
                Case InChar of
                  Rt_Aro: Arrow := Right;                   Lf_Aro: Arrow := Left;
                  Up_Aro: Arrow := Up;
                  Dn_Aro: Arrow := Down;
                  HelpF1: KeyHit := HelpKey;
                else
                  KeyHit :=NoKey;
                  Write(#7);
                end;
           end
           else if InChar = Return then KeyHit := ReturnKey
           else
            begin
                 KeyHit := CharKey;
                 ChHit  := UpCase(InChar);
            end;
     Until KeyHit <> NoKey;
end;  {...Get_KeyType...}


Procedure GetSizes(var toggle : togglerec);

var
  ch : char;
  item,pos : integer;
begin
with toggle do
begin
 pos := 1;
 for item := 1 to NoItems do
 begin
 while MenuStr[pos] = ' ' do
  pos := pos + 1;
  Posn[item] := pos;
 while menuStr[pos] <> ' ' do
  pos := pos + 1;
  Size[item] := pos - Posn[item];
 end;
end;
end;

{-------------------------------------------------------}
{                HELP-SCREENS MODULE                    }
{-------------------------------------------------------}

Procedure HelpMessage(Command : CommandStr);

Const
     Cnr1 = 15; Cnr2 = 4; Cnr3 = 66;
Var
   TempHelpPointer : HelpPointer;
   i,j,n : Integer;
   HelpAttr,BorderAttr : byte;
   ThisLine : string[50];
   DisplayLine : array[1..2,Cnr1..Cnr3,1..2] of byte;
   HelpWindow : array[1..20] of DisplayLineType absolute GrafBase : 320;
Begin
     TempHelpPointer := HelpArray;
     While not ((TempHelpPointer^.HelpScreen[1] = 'COMMAND:'+Command) or
                (TempHelpPointer^.HelpScreen[1] = 'COMMAND:ELSE')  or
                (TempHelpPointer = nil)) do
     TempHelpPointer := TempHelpPointer^.NextHelp;
     If TempHelpPointer <> nil then
     begin
          with status.colors do
          begin
          HelpAttr := GetAttr(Commandfore,commandback);
          BorderAttr := GetAttr(highlight,highlightback);
          TextColor(HighLight);
          TextBackground(HighLightBack);
          end;
          DisplayLine[1,Cnr1,1] := 201;
          DisplayLine[1,Cnr1,2] := BorderAttr;
          DisplayLine[2,Cnr1,1] := 186;
          DisplayLine[2,Cnr1,2] := BorderAttr;
          for i := 1 to 2 do
            begin
            DisplayLine[1,Cnr1+i,1] := 205;
            DisplayLine[1,Cnr1+i,2] := BorderAttr;
            DisplayLine[2,Cnr1+i,1] := 0;
            DisplayLine[2,Cnr1+i,2] := HelpAttr;
            end;

          for i := 3 to length(command)+10 do
            begin
            DisplayLine[1,Cnr1+i,1] := 0;
            DisplayLine[1,Cnr1+i,2] := BorderAttr;
            DisplayLine[2,Cnr1+i,1] := 0;
            DisplayLine[2,Cnr1+i,2] := HelpAttr;
            end;
          for i := length(command)+11 to 50 do
            begin
            DisplayLine[1,Cnr1+i,1] := 205;
            DisplayLine[1,Cnr1+i,2] := BorderAttr;
            DisplayLine[2,Cnr1+i,1] := 0;
            DisplayLine[2,Cnr1+i,2] := HelpAttr;
            end;


          DisplayLine[1,Cnr3,1] := 187;
          DisplayLine[1,Cnr3,2] := BorderAttr;
          DisplayLine[2,Cnr3,1] := 186;
          DisplayLine[2,Cnr3,2] := BorderAttr;

           WaitRetraceV;
           Move(DisplayLine[1],HelpWindow[1,Cnr1],104);
           Move(DisplayLine[2],HelpWindow[2,Cnr1],104);

          gotoXY(Cnr1+3,Cnr2-1);
          Write(' HELP: ',Command,' ');

          n := 2;
          repeat
          ThisLine := TempHelpPointer^.HelpScreen[n];
          DisplayLine[1,Cnr1,1] := 186;
          DisplayLine[1,Cnr1,2] := BorderAttr;
          DisplayLine[1,Cnr3,1] := 186;
          DisplayLine[1,Cnr3,2] := BorderAttr;
          DisplayLine[1,Cnr1+1,1] := 0;
          DisplayLine[1,Cnr1+1,2] := HelpAttr;
          for j := 1 to length(ThisLine) do
              begin
              DisplayLine[1,j+Cnr1+1,1] :=
                                  ord(TempHelpPointer^.HelpScreen[n][j]);
              DisplayLine[1,j+Cnr1+1,2] := HelpAttr;
              end;
          for j := (length(ThisLine)+1) to 49 do
              begin
              DisplayLine[1,j+Cnr1+1,1] := 0;
              DisplayLine[1,j+Cnr1+1,2] := HelpAttr;
              end;

          WaitRetraceV;
          Move(DisplayLine[1],HelpWindow[n+1,Cnr1],104);

          n := n + 1;
          until TempHelpPointer^.HelpScreen[n] = 'END-OF-HELP';


          DisplayLine[1,Cnr1,1] := 186;
          DisplayLine[1,Cnr1,2] := BorderAttr;
          DisplayLine[1,Cnr3,1] := 186;
          DisplayLine[1,Cnr3,2] := BorderAttr;
          DisplayLine[2,Cnr1,1] := 200;
          DisplayLine[2,Cnr1,2] := BorderAttr;
          DisplayLine[2,Cnr3,1] := 188;
          DisplayLine[2,Cnr3,2] := BorderAttr;
          for i := 1 to 2 do
            begin
            DisplayLine[1,Cnr1+i,1] := 0;
            DisplayLine[1,Cnr1+i,2] := HelpAttr;
            DisplayLine[2,Cnr1+i,1] := 205;
            DisplayLine[2,Cnr1+i,2] := BorderAttr;
            end;
          for i := 3 to 21 do
            begin
            DisplayLine[1,Cnr1+i,1] := 0;
            DisplayLine[1,Cnr1+i,2] := HelpAttr;
            end;
          for i := 22 to 50 do
            begin
            DisplayLine[1,Cnr1+i,1] := 0;
            DisplayLine[1,Cnr1+i,2] := HelpAttr;
            DisplayLine[2,Cnr1+i,1] := 205;
            DisplayLine[2,Cnr1+i,2] := BorderAttr;
            end;

          WaitRetraceV;
          Move(DisplayLine[1],HelpWindow[n+1,Cnr1],104);
          WaitRetraceV;
          Move(DisplayLine[2],HelpWindow[n+2,Cnr1],104);

          with status.colors do
          InitWindow(Cnr1,n+Cnr2+1,Cnr3,22,CentreFore,CentreBack);
          gotoXY(Cnr1+3,Cnr2-1);
          Write(' HELP: ',Command,' ');
          gotoXY(cnr1+3,Cnr2+n);
          Write(' ',#27,' ',#26,' for more help ');
     end;
End;



Procedure Get_MenuItem(Toggle : ToggleRec; Var Choice: Char);

Var
   Key,Key2 : KeyType;
   Dir : ArrowType;
   i,j : byte;
   ItemStr : LineType;
   Command : CommandStr;
   Dum : char;
Begin
     Hide;
     With Toggle do
     with status.colors do
     begin
          GetSizes(toggle);
          ShowStr(MenuStr,commandfore,commandback,Line,1);
          ItemStr := Copy(MenuStr,Posn[1]-1,Size[1]+2);
          ShowStr(ItemStr,highlight,highlightback,Line,Posn[1]-1);
          i := 1;
          Key := NoKey;
          Repeat
                Get_KeyType(Key,Dir,Choice);
                Case Key of
                ArrowKey:begin
                    case Dir of
                      left :  begin
                         ShowStr(ItemStr,commandfore,commandback,Line,Posn[i]-1);
                         If i = 1 then i := NoItems else i := i-1;
                         ItemStr := Copy(MenuStr,Posn[i]-1,Size[i]+2);
                         ShowStr(ItemStr,highlight,highlightback,Line,Posn[i]-1);
                              end;
                      Right : begin
                         ShowStr(ItemStr,commandfore,commandback,Line,Posn[i]-1);
                         If i = NoItems then i := 1 else i := i+1;
                         ItemStr := Copy(MenuStr,Posn[i]-1,Size[i]+2);
                         ShowStr(ItemStr,highlight,highlightback,Line,Posn[i]-1);
                              end;
                      Up    : with position do
                              if pos('8',ItemSet) <> 0 then
                              ScrollUp(currentdisplay);
                      Down  : with position do
                              if pos('2',ItemSet) <> 0 then
                              ScrollDown(currentdisplay);
                    end;
                    end;
                CharKey:
                    begin
                    j := Pos(Choice,ItemSet);
                    If j <> 0 then
                    begin
                         If j <= NoItems then
                         begin
                         ShowStr(ItemStr,commandfore,commandback,Line,Posn[i]-1);
                         i := j;
                         ItemStr := Copy(MenuStr,Posn[i]-1,Size[i]+2);
                         ShowStr(ItemStr,highlight,highlightback,Line,Posn[i]-1);
                         end;
                         Key := ReturnKey
                    end
                    else burp;
                    end;
                ReturnKey:
                    Choice := ItemSet[i];
                HelpKey : begin
                           PrepareWindowTwo;
                           repeat
                           Command := Uppercase(Copy(MenuStr,Posn[i],Size[i]));
                           HelpMessage(command);
                           Get_KeyType(Key2,Dir,Dum);
                           case Dir of
                      Left :  begin
                         ShowStr(ItemStr,commandfore,commandback,Line,Posn[i]-1);
                         If i = 1 then i := NoItems else i := i-1;
                         ItemStr := Copy(MenuStr,Posn[i]-1,Size[i]+2);
                         ShowStr(ItemStr,highlight,highlightback,Line,Posn[i]-1);
                              end;
                      Right : begin
                         ShowStr(ItemStr,commandfore,commandback,Line,Posn[i]-1);
                         If i = NoItems then i := 1 else i := i+1;
                         ItemStr := Copy(MenuStr,Posn[i]-1,Size[i]+2);
                         ShowStr(ItemStr,highlight,highlightback,Line,Posn[i]-1);
                              end;
                           end;
                           until Key2 <> ArrowKey;
                           ShowSheet(position.currentdisplay);
                           GotoXY(1,line);
                          end;
                 end;
          Until Key = ReturnKey;
     end; {..With Toggle..}
end; {...Get_MenuItem..}






End.
