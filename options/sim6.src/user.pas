{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit USER;

Interface

Uses
  Crt,
  INITIAL,
  TRADING,
  DATA,
  MS,
  BLACK,
  DRAW2,
  MISC,
  MISC2,
  IO;


Procedure InitColors;

Function GetStatus : boolean;

Procedure StatusInit;

Procedure InitData;

Procedure InitDisplayWindows;

Procedure GetVol;

Procedure StatusPage;

Procedure AlterColors;

Procedure DoRecall;

Procedure test;               {a less primitive user interface}

{===========================================================================}

Implementation


Procedure InitColors;

begin
with status do
colors := HardWare.DefaultColors;
end;


Function GetStatus : boolean;

var
  statsavefile : file of statustype;
begin
assign(statsavefile,'Status.sav');
{$I-}
reset(statsavefile);
{$i+}
if IOresult = 0 then
  begin
  GetStatus := true;
  read(statsavefile,status);
  close(statsavefile);
  end
 else
   getstatus := false;
end;


Procedure StatusInit;

var
  dummyday,err,startday,startmonth,startyear : integer;
  startmonthstr : monthtype;
  possavefile : file of positionrec;
begin
with status do
begin
CrashDay := trunc(2000*random);
position.RecalcUser := true;
PalNo := 2;
OptTranscost := 2;
ShareTransCost := 5;
GotoMain := false;
currentday := 0;
xAxis := stockA;
currenttime := 1;
PosGraphLim := 50;  {% up and down for pos graph}
startmonthstr := copy(startdate,3,3);
startmonth := monthnum(startmonthstr);
val(copy(startdate,1,2),startday,err);
val(copy(startdate,6,2),startyear,err);
DummyDay :=
   ((trunc(SerialDate(startyear-40,startmonth,startday)) + 6) mod 7);
if dummyday > 4 then
begin
currentday := 7 - dummyday;
DayOfWeek := 0;
end else DayOfWeek := DummyDay;
end;
If Exist('Position.sav') then
begin
  assign(possavefile,'position.sav');  {now get rid of any existing records}
  rewritePos;
  close(possavefile);
end;
end;


Procedure InitData;

VAR
  PRICE : REAL;
  START : INTEGER;
  answer : char;
begin
with position do
with status do
begin
repeat writeln('Company Name?');
readln(companyname);
until length(companyname) < 12;
if length(companyname) <> 0 then
begin
price := 0;
repeat
  WRITELN('Start Price?');
  price := InputReal(1);
  if not between(price,0.5,10000.0) then
    begin
    writeln;
    writeln('Price must be between 50c and $10,000');
    end;
until between(price,0.5,10000.0);
volatility := 10 + random * 40;
writeln;
writeln('Creating Data....');
wait;
hide;
makedata(price,volatility,riskfreerate);
NumStrikes := 0;
createmarket;
recalc_BS_values;
ActiveCompany := true;
position.VolMethod := '30 Days';
Position.workingvol := 10;
recalcUser := true;
RecalcGraphs := true;
CompanyMargin := 0;
Capital := 1000000.00;
PendingCost := 0;
end;
end;
end;


Procedure InitDisplayWindows;

begin
textmode(Status.HardWare.TextModeNo);
Hide;
Preparewindowone;
Preparewindowtwo;
Preparewindowthree;
end;


Procedure GetVol;

begin
with position do
with status do
begin
if volmethod = 'Implied' then
begin
if VolWithPuts then
workingVol := ImplicitVol(Both) else
workingVol := ImplicitVol(Calls);
end
else if copy(volmethod,4,4) = 'Days' then
WorkingVol := HistVol;
 if workingVol <= 0 then
 begin
  burp;
  volmethod  := '30 Days';
  workingVol := Histvol;
 end;
end;
end;


Procedure StatusPage;

var
  ch : char;
  n : integer;
  CommandToggle : ToggleRec;

Procedure DisplayValues;
begin
with status do
begin
textbackground(colors.centreback);
textcolor(colors.centrefore);
gotoXY(50,3);
write(riskfreerate:5:2,'%');
gotoXY(50,5);
write(position.workingvol:5:2,'%',' (exact ',
      position.intravol[currenttime]:5:2,'% )');
gotoXY(50,7);
write(position.VolMethod);
gotoXY(50,9);
clrEol;
if VolWithPuts then write('With Puts')
 else write('Without Puts');
gotoXY(50,11);
clreol;
if GotoMain then write('Yes') else write('No');
gotoXY(50,13);
clreol;
if xAxis = stockA then write('Stock') else write('Volatility');
gotoXY(50,15);
clreol;
write('Exact StockPrice is ',position.intraarray[status.currenttime]:7:2);
end;
end;

begin
PrepareWholeScreen;
PrepareWindowTwo;
gotoXY(1,3);
writeln('Working Interest Rate');
Writeln;
Writeln('Working Volatility');
writeln;
writeln('Volatility Calculation');
writeln;
writeln('Implied Vol Calculation');
writeln;
writeln('Goto Main Menu :');
writeln;
writeln('x-Axis of position graphs');
 if position.RecalcUser then getvol;
repeat
wait;
displayvalues;
ready;
      PrepareWindowOne;
      ready;
      with commandtoggle do
          begin
          MenuStr := '  Volatility  Puts  GotoMain  Accounting  Quit ';
          NoItems  := 5;
          ItemSet  := 'VPGAQ';
          Line     := 1;
          end;
          Get_MenuItem(CommandToggle,ch);
case upcase(ch) of
  'V' : begin
      PrepareWindowOne;
      ready;
      with commandtoggle do
          begin
          MenuStr := '  Implied   Specified   n-Days  Quit ';
          NoItems  := 4;
          ItemSet  := 'ISNQD';
          Line     := 1;
          end;
        Repeat
          PrepareWindowOne;
          ready;
          Get_MenuItem(CommandToggle,ch);
        case upcase(ch) of
          'I' : begin
                wait;
                with position do
                begin
                volmethod := 'Implied';
                RecalcUser := true;
                RecalcGraphs := true;
                end;
                getvol;
                DisplayValues;
                end;
          'S' : begin
                repeat
                ShowCommands('Working Volatility : ');
                gotoXY(22,1);
                unhide;
                position.workingvol := inputreal(22);
                hide;
                if not between(position.workingvol,5,100) then burp;
                until between(position.workingVol,5,100);
                position.volmethod := 'Specify';
                position.RecalcUser := true;
                DisplayValues;
                end;
      'D','N' : begin
                repeat
                ShowCommands('Number of days : ');
                gotoXY(19,1);
                unhide;
                n := inputinteger(19);
                hide;
                if not between(n,10,100) then burp;
                until between(n,10,100);
                str(n:2,position.volmethod);
                position.volmethod := position.volmethod + ' Days';
                position.RecalcUser := true;
                wait;
                getvol;
                DisplayValues;
                end;
           'Q' :;
           else burp;
        end;
        DisplayValues;
        until EndMenu(ch);
        wait;
        if position.recalcuser then
        Recalc_USER_values;
        end;
   'G' : status.GotoMain := not Status.GotoMain;
   'P' : status.VolWithPuts := not Status.VolWithPuts;
   'Q' : ;
   else burp;
   end;
 until upcase(ch) = 'Q';
wait;
 if position.RecalcUser then
           Recalc_USER_Values;
ready;
end;


Procedure AlterColors;

var
  ColorsArray : array[0..11] of byte absolute Status;
  index,Dummy : integer;
  action : char;
  forecolor,backcolor : byte;
  CommandToggle : ToggleRec;

Procedure FillCommodScreens;
var
 MainAttr,HeadAttr : byte;
 i,j : integer;
 commod : commodtype;
begin
with status.colors do
begin
MainAttr := GetAttr(CentreFore,CentreBack);
HeadAttr := GetAttr(HeadingFore,CentreBack);
end;
for commod := Call to Put do
begin
for i := 1 to 80 do
CommodScreens[commod,0,i,2] := HeadAttr;
for j := 1 to (position.numstrikes*3 + 2) do
begin
for i := 1 to 2 do
CommodScreens[commod,j,i,2] := HeadAttr;
for i := 3 to 80 do
CommodScreens[commod,j,i,2] := MainAttr;
end;
end;
end;

Procedure ShowColors;

Procedure PrepareWindowFour;
begin
InitWindow(60,5,80,9,15,0);
TextColor(15);
TextBackground(0);
gotoXY(61,7);
end;

begin
if index in [0,1,3,4,5,8,9,10,11] then
    PrepareWindowFour;
case index of
0 : begin
    write('Command Background');
    end;
1 : begin
    write('Highlight Background');
    end;
2 : begin
    FillCommodScreens;
    showsheet(Call);
    PrepareWindowFour;
    write('Centre Background');
    end;
3 : begin
    write('Status Background');
    ShowStatus;
    end;
4 : begin
    write('Command Foreground');
    end;
5 : begin
    write('Command Highlight');
    end;
6 : begin
    FillCommodScreens;
    showsheet(Call);
    PrepareWindowFour;
    write('Centre Foreground');
    end;
7 : begin
    FillCommodScreens;
    showsheet(Call);
    PrepareWindowFour;
    write('Heading Foreground');
    end;
8 : begin
    write('Stock Foreground');
    ShowStatus;
    end;
9 : begin
    write('Price Foreground');
    ShowStatus;
    end;
10 : begin
    write('Days Foreground');
    ShowStatus;
    end;
11 : begin
    write('Account Foreground');
    ShowStatus;
    end;

end;
end;


begin
position.statusdisplay := accounts;
preparewindowtwo;
index := 0;
      with commandtoggle do
          begin
          MenuStr := '  Next   Previous   color(+  -)   Default   Quit ';
          NoItems  := 6;
          ItemSet  := 'NP+-DQ ';
          Line     := 1;
          end;
showcolors;
repeat
begin
PrepareWindowOne;
ready;
Get_MenuItem(CommandToggle,action);
case upcase(action) of
  '+' : if index in [0,1,2,3] then
          ColorsArray[index] := (colorsarray[index] + 1) mod 8
         else
          ColorsArray[index] := (colorsarray[index] + 1) mod 16;
  '-' : ColorsArray[index] := colorsarray[index] - 1;
  ' ','N' : index := (index + 1) mod 12;
      'P' : index := index - 1;
      'D' : begin
            InitColors;
 if not (index in [1,4,5]) then
 begin
 FillCommodScreens;
 ShowSheet(Call);
 end;
 ShowStatus;
            end;
end;
if index = -1 then index := 11;
if colorsarray[index] = -1 then
     case index of
   0,1,2,3 : colorsarray[index] := 7;
        else colorsarray[index] := 15;
     end;
ShowColors;
end until upcase(action) = 'Q';
end;


Procedure DoRecall;

var
  count,companynum :  integer;
  PosSaveFile : file of positionRec;
  dummypos : positionrec;
  numOK : boolean;
  centered : boolean;
begin
wait;
PrepareWindowTwo;
gotoXY(1,2);
centered := false;
assign(PosSaveFile, 'Position.sav');
If DriveReady then
{$I-} reset(possavefile) {$I+};
if IOresult = 0 then
begin
count := 0;
while not eof(possavefile) do
begin
read(possavefile,dummypos);
count := count + 1;
if centered then gotoXY(40,whereY) else gotoXY(1,whereY + 1);
write(count:2,'  ',dummypos.companyname);
centered := not centered;
end;
close(possavefile);
if count = 0 then burp
else
begin
repeat
numOK := false;
ready;
ShowCommands('Which Company? : ');
unhide;
gotoXY(19,1);
companynum := inputinteger(19);
hide;
numOK := (companynum > 0) and (companynum <= count);
if companynum = 0 then numOK := true;
until numOK;
if not (companynum = 0) then
begin
wait;
RecallPosition(companynum);
if activecompany then
  begin
  Check4Limit;
  Check4Expy;
  Recalc_BS_Values;
  Recalc_USER_Values;
  CalculateMargin;
  end;
ready;
end;
end;
end else
begin
 burp;
 ActiveCompany := false;
end;
end;



Procedure test;               {a less primitive user interface}

var
  CommandToggle : ToggleRec;
  answer,ch : char;
  month : monthtype;
  strike : real;
  number,endplot : integer;
  pathname,params : linetype;
  code,zoom : integer;
  Key : KeyType;
  Dir : ArrowType;
begin
with position do
begin
randomize;
ActiveCompany := false;
InitDisplayWindows;
currentdisplay := Call;
If Exist('Position.sav') then
begin
Hide;
ready;
  with commandtoggle do
          begin
          MenuStr := '  Recall Initialise ';
          NoItems  := 2;
          ItemSet  := 'RI';
          Line     := 1;
          end;
repeat
preparewindowone;
ready;
Get_MenuItem(CommandToggle,answer);
case upcase(answer) of
 'R' : begin
       currentdisplay := Call;
       DoRecall;
       if ActiveCompany then
       begin
       MakeList;
       showsheet(currentdisplay);
       ShowStatus;
       end;
       end;
 'I' :  begin
                PrepareWindowtwo;
                gotoXY(1,3);
                UnHide;
                initdata;
                wait;
                ShowStatus;
                currentdisplay := Call;
                getvol;
                Recalc_USER_values;
                MakeList;
                showsheet(currentdisplay);
                DoPosArray;
                ActiveCompany := true;
        end;
end;
if not ActiveCompany then
   ShowMessage('No Save Files Found');
until (upcase(answer) in ['R','I']) and  ActiveCompany;
end
else     {if no save files}
begin
          PrepareWindowthree;
          PrepareWindowtwo;
          gotoXY(1,3);
          UnHide;
          write('Start Date? :               [ eg. 01Jan80 ]');
          status.startdate := inputdate(15);
          writeln;
          write('Risk Free Interest Rate?  ');
          with status do
          repeat
          riskfreerate := inputreal(27);
          if not (between(riskfreerate,5,50)) then
                begin
                writeln;
                burp;
                writeln('Rate must be between 5% and 50%');
                gotoXY(1,5);
                end;
          until between(riskfreerate,5,50);
          writeln;
          statusinit;
          initdata;
          ActiveCompany := true;
          StatusDisplay := Accounts;
          ShowStatus;
          currentdisplay := Call;
          getvol;
          recalc_user_values;
          MakeList;
          showsheet(currentdisplay);
          DoPosArray;
          SaveStatus;
          gotoXY(1,6);
          with status.colors do
          begin
            TextColor(CentreFore);
            TextBackground(CentreBack);
          end;
end;
StatusDisplay := Accounts;
repeat
begin
PrepareWindowOne;
ready;
  with commandtoggle do
          begin
          MenuStr := '  Trade  Next  Graph  Utilities  Status  Company   Quit ';
          NoItems  := 7;
          ItemSet  := 'TNGUSCQ +28';
          Line     := 1;
          end;
Get_MenuItem(CommandToggle,answer);

case upcase(answer) of
'T' : begin
      PrepareWindowOne;
      ready;
      with commandtoggle do
          begin
          MenuStr := '  Call  Put  Share   Quit ';
          NoItems  := 4;
          ItemSet  := 'CPSQ+ 28';
          Line     := 1;
          end;
          repeat
          PrepareWindowOne;
          ready;
      Get_MenuItem(CommandToggle,answer);
          with OptionList[currentdisplay,code] do
          case upcase(answer) of
          'C' : begin
                unhide;
                if not (currentdisplay = Call) then
                begin
                currentdisplay := Call;
                showsheet(currentdisplay);
                end;
                repeat
                ShowCommands('Code : ');
                gotoXY(9,1);
                code := InputInteger(9);
                until code <= NumStrikes*3;
                if code <> 0 then
                begin
                ShowCommands('No. of lots : ');
                gotoXY(16,1);
                number := InputInteger(16);
                hide;
                wait;
                RecalcGraphs := true;
                with OptionList[Call,code] do
                add_to_pending(month,strike,Call,number);
                end;
                ShowStatus;
                end;
          'P' : begin
                unhide;
                if not (currentdisplay = Put) then
                begin
                currentdisplay := Put;
                showsheet(currentdisplay);
                end;
                repeat
                ShowCommands('Code : ');
                gotoXY(9,1);
                code := InputInteger(9);
                until code <= NumStrikes*3;
                if code <> 0 then
                begin
                ShowCommands('No. of lots : ');
                gotoXY(16,1);
                number := InputInteger(16);
                RecalcGraphs := true;
                hide;
                wait;
                with OptionList[Put,code] do
                add_to_pending(month,strike,Put,number);
                end;
                ShowStatus;
                end;
          'S' : begin
                unhide;
                ShowCommands('No. of shares : ');
                gotoXY(18,1);
                number := InputInteger(18);
                hide;
                wait;
                RecalcGraphs := true;
                add_to_pending(month,strike,Share,number);
                ShowStatus;
                end;
          ' ' : begin
                if currentdisplay = Call then
                currentdisplay := Put
                else
                currentdisplay := Call;
                ShowSheet(currentdisplay);
                end;
          '+' : begin
                with position do
                if StatusDisplay = Totals then
                StatusDisplay := Accounts else
                StatusDisplay := Totals;
                ShowStatus;
                end;
          'Q' :;
          else burp;
          end;
          if upcase(answer) in ['C','P'] then
          begin
          MakeList;
          showsheet(currentdisplay);
          end;
          until EndMenu(answer);
          end;
' ' :  begin
       if currentdisplay = Call then
       currentdisplay := Put
       else
       currentdisplay := Call;
       ShowSheet(currentdisplay);
       end;

'G' : begin
      with commandtoggle do
          begin
          MenuStr := '  History  Value  Delta  Gamma  decaY  Expry  Quit ';
          NoItems  :=  7;
          ItemSet  := 'HVDGYEQ+ 28';
          Line     := 1;
          end;
      repeat
      PrepareWindowOne;
      ready;
      Get_MenuItem(CommandToggle,ch);
      if (upcase(ch) in ['V','G','D','Y','E']) AND RecalcGraphs then
       begin
       wait;
       DoPosArray;
       ready;
       end;
          case upcase(ch) of
      ' ' :  begin
             if currentdisplay = Call then
             currentdisplay := Put
             else
             currentdisplay := Call;
             end;
          'H' : repeat
                plothistory(status.currentday);
                until not redraw;
          'V' : repeat
                graphpos(valueG);
                until not redraw;
          'D' : repeat
                graphpos(deltaG);
                until not redraw;
          'G' : repeat
                graphpos(gammaG);
                until not redraw;
          'Y' : repeat
                graphpos(decayG);
                until not redraw;
          'E' : repeat
                GraphPos(expryG);
                until not redraw;
          'Q' :;
          '+' :  begin
                 with position do
                 if StatusDisplay = Totals then
                 StatusDisplay := Accounts else
                 StatusDisplay := Totals;
                 ShowStatus;
                 end;
          else burp;
          end;
          hide;
          if upcase(ch) in ['H','V','G','D','Y','E',' '] then
          begin
          if upcase(ch) <> ' ' then
          ShowStatus;
          showsheet(currentdisplay);
          end;
          until EndMenu(ch) and (upcase(ch) <> 'X');
          end;
'N' : begin
      PrepareWindowOne;
      ready;
      with commandtoggle do
          begin
          MenuStr := '  Half-Hour   Day   Week   Quit ';
          NoItems  := 4;
          ItemSet  := 'HDWQ+ 28';
          Line     := 1;
          end;
          repeat
          PrepareWindowOne;
          ready;
          Get_MenuItem(CommandToggle,ch);
          case upcase(ch) of
      ' ' :  begin
             if currentdisplay = Call then
             currentdisplay := Put
             else
             currentdisplay := Call;
             end;
          'H' : begin
                 wait;
                 ShowMessage('Updating data....');
                 newhour;
                 RecalcUser := true;
                 makelist;
                 RecalcGraphs := true;
                end;
          'D' : begin
                 ShowMessage('Updating data....');
                 wait;
                 newday;
                 RecalcUser := true;
                 makelist;
                 RecalcGraphs := true;
                end;
          'W' : begin
                 ShowMessage('Updating data....');
                 wait;
                 newweek;
                 RecalcUser := true;
                 makelist;
                 RecalcGraphs := true;
                end;
          'Q' :;
          '+' :  begin
                 with position do
                 if StatusDisplay = Totals then
                 StatusDisplay := Accounts else
                 StatusDisplay := Totals;
                 ShowStatus;
                 end;
          else burp;
          end;
          if upcase(ch) in ['H','D','W',' '] then
          begin
          showsheet(currentdisplay);
          if upcase(ch) <> ' ' then
          ShowStatus;
          end;
          until EndMenu(ch);
          if recalcUser then
          begin
          wait;
          getvol;
          recalc_USER_values;
          makelist;
          showsheet(currentdisplay);
          end;
          end;
'C' : begin
      prepareWindowOne;
      ready;
      with commandtoggle do
          begin
          MenuStr := '  Save  Recall  Initialise   Quit ';
          NoItems  := 4;
          ItemSet  := 'SRIQ+ 28';
          Line     := 1;
          end;
          repeat
          PrepareWindowOne;
          ready;
          Get_MenuItem(CommandToggle,ch);
          case upcase(ch) of
      ' ' :  begin
             if currentdisplay = Call then
             currentdisplay := Put
             else
             currentdisplay := Call;
             showsheet(currentdisplay);
             end;
          'S' : begin
                wait;
                SaveGame;
                end;
          'R' : begin
                currentdisplay := Call;
                wait;
                DoRecall;
                MakeList;
                showsheet(currentdisplay);
                showstatus;
                end;
          'I' : begin
                PrepareWindowtwo;
                gotoXY(1,3);
                UnHide;
                initdata;
                wait;
                ShowStatus;
                currentdisplay := Call;
                getvol;
                Recalc_USER_values;
                MakeList;
                showsheet(currentdisplay);
                DoPosArray;
                end;
          'Q' :;
          '+' :  begin
                 with position do
                 if StatusDisplay = Totals then
                 StatusDisplay := Accounts else
                 StatusDisplay := Totals;
                 ShowStatus;
                 end;
          else burp;
          end;
          until EndMenu(ch);
      end;
'Q' :  begin
          ShowMessage('Shall I SAVE? [y/n]');
          Ch := ReadKey;
          if upcase(ch) = 'Y' then
                               begin
                               wait;
                               savegame;
                               end;
          if upcase(ch) in ['Y','N'] then
          answer := 'q'
          else  begin
                showstatus;
                answer := ' ';
                end;
       end;
'+' :  begin
       with position do
       if StatusDisplay = Totals then
          StatusDisplay := Accounts else
          StatusDisplay := Totals;
       ShowStatus;
       end;
'S' : begin
      StatusPage;
      ShowStatus;
      MakeList;
      showsheet(currentdisplay);
      end;
'U' : begin
      PrepareWindowOne;
      ready;
      with commandtoggle do
          begin
          MenuStr := '  Colors  Execute  Quit ';
          NoItems  := 3;
          ItemSet  := 'CEQ+ 28';
          Line     := 1;
          end;
          repeat
          PrepareWindowOne;
          ready;
          Get_MenuItem(CommandToggle,ch);
      case upcase(ch) of
       ' ' : begin
             if currentdisplay = Call then
             currentdisplay := Put
             else
             currentdisplay := Call;
             ShowSheet(currentdisplay);
             end;
       'E' : begin
             UnHide;
             ShowCommands('Full path name : ');
             gotoXY(18,2);
             readln(pathname);
             ShowCommands('Parameters : ');
             gotoXY(14,1);
             readln(params);
             wait;
             Execute(pathname,params);
             hide;
             ShowSheet(currentdisplay);
             ShowStatus;
             end;
       'C' :begin
            AlterColors;
            ShowSheet(currentdisplay);
            Showstatus;
            end;
       'Q' :;
       '+' :     begin
                 with position do
                 if StatusDisplay = Totals then
                 StatusDisplay := Accounts else
                 StatusDisplay := Totals;
                 ShowStatus;
                 end;
       else burp;
       end;
       until EndMenu(ch);
     end;
else burp;
end;
end;
until answer = 'q';
  PrepareWholeScreen;         {make display normal again}
  TextBackground(0);
  Textcolor(white);
  ClrScr;
  UnHide;
end;
end;




End.
