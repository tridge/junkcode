{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit DRAW2;

Interface

Uses
  Graph,
  Graph3,
  Crt,
  BLACK,
  INITIAL;


Procedure plothistory(currentday : integer);

Procedure GraphPos(TypeOfGraph : graphtype);

{===========================================================================}

Implementation

Const
     GraphColors : array[0..3,1..4] of byte =
                   ((lightblue,yellow,lightcyan,green),
                    (red,lightblue,yellow,lightcyan),
                    (white,green,lightblue,yellow),
                    (yellow,white,green,lightblue));



Procedure EnterGraphic;
var
  GraphMode,GraphDriver : integer;
begin
  GraphDriver := detect;
  InitGraph(GraphDriver,GraphMode,'');
  with status.hardware do
  begin
    MaxXgraf := GetMaxX;
    MaxYgraf := GetMaxY;
  end;
end;


Procedure LeaveGraphic;
begin
  CloseGraph;
  TextMode(3);
end;


Procedure Draw(x1,y1,x2,y2 : integer);

var
  X1S,X2S,Y1S,Y2S : integer;
begin

with status.HardWare do
begin
 x1S := round(x1 / 1000 * MaxXgraf);
 x2S := round(x2 / 1000 * MaxXgraf);
 y1S := round(y1 / 1000 * MaxYgraf);
 y2S := round(y2 / 1000 * MaxYgraf);
end;
Line(x1s,y1s,x2s,y2s);
end;


Procedure WriteXY(x,y : integer; OutStr : linetype);
begin
  with status.hardware do
  begin
    x := round(x/80 * MaxXgraf);
    y := round(y/25 * MaxYgraf);
  end;
  OutTextXY(x,y,OutStr);
end;


Procedure WaitG;

begin
setcolor(GraphColors[status.PalNo,3]);
writeXY(60,1,'WAIT');
end;


Procedure GetFormat(var X : Real; var Fld,Dec : Integer);

const
    MaxLength = 10;
var
   Pos1,Pos2,Pos3,i : Integer;
   XStr   : String[MaxLength];

Begin
     Str(X:MaxLength:3,XStr);
     i := 0;
     if abs(x) > 1 then
     repeat
     i := i + 1
     until xstr[i] in ['1'..'9'];
     Pos1 := i;
     repeat
     i := i + 1
     until ((Xstr[i] in ['.']) or (i = MaxLength));
     Pos2 := i;
     i := MaxLength;
     while Xstr[i] = '0' do
     i := i - 1;
     Pos3 := i;
     Dec := Pos3-Pos2;
     if abs(x) < 1 then
     Fld := Pos3 - Pos2 + 2 else
     Fld := Pos3 - Pos1 + 1;
     if x = 0 then Fld := 1;
end;


Procedure WriteNum(x,y : integer; num : real; Fld,Dec : integer);

var
  NumStr : string[11];
begin
  Str(num:Fld:Dec,NumStr);
with status.HardWare do
begin
x := round(x / 1000 * MaxXgraf);
y := round(y / 1000 * MaxYgraf);
end;
OutTextXY(x,y,NumStr);
end;


Function max(a,b : real) :real;

begin
if a < b then max := b
else max := a;
end;


Function min(a,b : real) :real;

begin
if a < b then min := a
else min := b;
end;


Function GetScaleSep(start,finish : real; MaxTicks : Integer) : real;

var
  diff,sep : real;
  numticks,power : integer;

Function OkScale : boolean;
begin
  NumTicks := trunc(Diff/Sep);
  OkScale := NumTicks <= MaxTicks;
end;

begin
  diff := finish - start;
  if diff > 0.00001 then
  begin
  power := trunc(0.4343 * ln(diff));
  Sep := exp(power * ln(10))/100;
  repeat
   If not OkScale then sep := sep * 2;
   If not OkScale then sep := sep * 2.5;
   If not OkScale then sep := sep * 2;
  until OkScale;
  GetScaleSep := Sep;
  end
  else
  GetScaleSep := 0.2;
end;


Procedure DoGraph(A : PlotArray; NumPts,NumGrs : integer; Mode : GraphType;
                  GraphString,HorizLabel : linetype);

var
  i,X : integer;
  ch : char;
  ZoomStr : string[10];
  Zoom : integer;
Const
     Frame : corners = (66,200,781,875);

Procedure GetLimits(var MinA,MaxA,MinB,MaxB : Real);
Var
   i : Integer;
begin
     MinA := A[1];
     MaxA := A[NumPts];
     MinB := A[NumPts+1];
     MaxB := MinB;
     For i := NumPts+2 to NumPts*(NumGrs+1) do
     begin
          If (A[i] < MinB) then MinB := A[i] else
          If (A[i] > MaxB) then MaxB := A[i];
     end;
end;

Procedure Graph2(Frame : Corners);
Const
     Gap = 15;
Var
   i,j,n,x1,x2,y1,y2 : Integer;
   MinX,MaxX,MinY,MaxY : Real;
   Amax,Amin,Bmax,Bmin : Integer;
   XScale,YScale,xSep,ySep,ScxSep,ScySep : Real;
   FX,DX,FY,DY : Integer;
   MidStock,TickVal : Real;
Begin
     SetColor(GraphColors[Status.PalNo,1]);
     Draw(Frame[1],Frame[2],Frame[3],Frame[2]);
     Draw(Frame[3],Frame[2],Frame[3],Frame[4]);
     Draw(Frame[3],Frame[4],Frame[1],Frame[4]);
     Draw(Frame[1],Frame[4],Frame[1],Frame[2]);

     GetLimits(MinX,MaxX,MinY,MaxY);
     Amin := Frame[1]+Gap;
     Amax := Frame[3]-Gap;
     Bmin := Frame[2]+Gap;
     BMax := Frame[4]-Gap;
     XScale := (Amax-Amin)/(MaxX-MinX);
     if MaxY - MinY > 0.000001  then
        YScale := (Bmax-Bmin)/(MaxY-MinY)
       else
        YScale := Bmax-Bmin;
     xSep := GetScaleSep(MinX, MaxX, 6);
     ySep := GetScaleSep(MinY, MaxY, 7);
     ScxSep := xSep*XScale;
     ScySep := ySep*YScale;
     GetFormat(xSep,FX,DX);
     GetFormat(ySep,FY,DY);
     If (DX = 0) and (FX < 3) and (Mode <> HistoryG) then
     begin
          DX := 1;
          FX := FX + 2;
     end;

                 {This puts in tickmarks}
     TickVal := round(MinX/xSep)*xSep;
     i := Amin + Round((TickVal-MinX)*XScale);
     If i < Frame[1] then
     begin
       TickVal := TickVal + xSep;
       i := Amin + Round((TickVal-MinX)*XScale);
     end;
     Repeat
           SetColor(GraphColors[Status.PalNo,1]);
           Draw(i,Frame[2],i,Frame[2]+12);
           Draw(i,Frame[4],i,Frame[4]-12);

           SetColor(GraphColors[Status.PalNo,2]);
           WriteNum(i - 30,Frame[4] + 20,TickVal,FX,DX);

           TickVal := TickVal + xSep;
           i := Amin + Round((TickVal-MinX)*XScale);
     Until i > Frame[3];
     TickVal := int(MaxY/ySep)*ySep;
     i := Bmax-Round((TickVal-MinY)*YScale);
     If i < Frame[2] then
     begin
         TickVal := TickVal-ySep;
         i := Bmax-Round((TickVal-MinY)*YScale);
     end;
     Repeat
           SetColor(GraphColors[Status.PalNo,1]);
           Draw(Frame[1],i,Frame[1]+12,i);
           Draw(Frame[3],i,Frame[3]-12,i);

           SetColor(GraphColors[Status.PalNo,2]);
           WriteNum(Frame[3] + 10,i - 5,TickVal,FY,DY);

           TickVal := TickVal-ySep;
           i := Bmax-Round((TickVal-MinY)*YScale);
     Until i > Frame[4];

              {...and this adds the zero-line & mid-line}
     If Mode <> HistoryG then
     begin
     if status.Xaxis = stockA then
       With Position.ShareSheet do
          MidStock := (Bid+Ask)/2
      else
       MidStock := position.intravol[status.currenttime];
     y1 := Bmax + round(MinY*yScale);
     If (y1 < Frame[4]) and (y1 > Frame[2]) then
      begin
        SetColor(GraphColors[Status.PalNo,1]);
        Draw(Frame[1],y1,Frame[3],y1);
      end;

     x1 := Amin + Round((MidStock-MinX)*XScale);

     SetColor(GraphColors[Status.PalNo,2]);
     WriteNum(x1-60,Frame[2]-30,MidStock,7,2);

     SetColor(GraphColors[Status.PalNo,1]);
     Draw(x1,Frame[2],x1,Frame[4]);
     end;

       {... now draw the graphs...}
     For n := 1 to NumGrs do
     begin
     j  := n*NumPts +1;
     x1 := Amin + Round((A[1]- MinX)*XScale);
     y1 := Bmax - Round((A[j]-MinY)*YScale);
     i := 1;

     SetColor(GraphColors[Status.PalNo,2+n]);

     Repeat
           i := i+1;
           x2 := Amin + Round((A[i]-MinX)*XScale);
           y2 := BMax - Round((A[i+j-1]-MinY)*YScale);
           Draw(x1,y1,x2,y2);
           x1 := x2;
           y1 := y2;
     Until i = NumPts;
     end;
end;

Begin
           EnterGraphic;
           SetColor(GraphColors[Status.PalNo,2]);
           WriteXY(6,1,GraphString);

           If Mode <> HistoryG then
           begin
                WriteXY(6,3,'Current---- ');

                SetColor(GraphColors[Status.PalNo,3]);
                If Mode = expryG then
                   WriteXY(18,3,'  Expiry----')
                else
                   WriteXY(18,3,' + Pending----');

                str((status.posgraphlim/10):1:0,ZoomStr);
                ZoomStr := 'ZOOM:' + ZoomStr;
                WriteXY(1,24,zoomstr);
           end;
           Graph2(Frame);

           SetColor(GraphColors[Status.PalNo,1]);
           WriteXY(25,24,HorizLabel);
           ReDraw := true;
              Ch := ReadKey;
              with status do
              case upcase(ch) of
              '+' : PalNo := (PalNo+1) mod 4;
              '-' : PalNo := (PalNo-1) mod 4;
              'X' : begin
                    with status do
                    if Xaxis = stockA then
                      Xaxis := volatilityA
                     else
                      Xaxis := StockA;
                    redraw := true;
                    WaitG;
                    position.RecalcGraphs := true;
                    if mode <> HistoryG then
                    DoPosArray;
                    end;
              '1'..'9' : begin
                         zoom := ord(ch) - 48;
                         posgraphlim := zoom * 10;
                         WaitG;
                         position.RecalcGraphs := true;
                         if mode <> HistoryG then
                         begin
                         DoPosArray;
                         redraw := true;
                         end;
                         end;
               else ReDraw := false;
               end;
           LeaveGraphic;
end;


Procedure plothistory(currentday : integer);

const
    numpts = 200;
Var
   HistData : plotarray;

Procedure getdata;
var
  i : integer;
  startday : integer;
begin
with position do
with status do
begin
startday := currentday - numpts;
for i := 1 to numpts do
begin
HistData[i] := i-1;
if xAxis = stockA then
  HistData[i+NumPts] := closingarray[startday + i]
 else
  HistData[i+NumPts] := volatilityarray[startday + i];
end;
end;
end; {getdata}

begin   {...PLOTHISTORY...}
 getdata;
 if status.xAxis = stockA then
   DoGraph(HistData,numpts,1,HistoryG,
         position.companyname + ' 200-Day Price History','No. of Days')
  else
   DoGraph(HistData,numpts,1,HistoryG,
         position.companyname + ' 200-Day Volatility History','No. of Days');
end;    {...PLOTHISTORY...}



Procedure GraphPos(TypeOfGraph : graphtype);

var x:real;
    a : PlotArray;
    heading : string[9];
    PriceStr : string[11];

Procedure DataToArray;
Const
     NumPts = 40;
var
  count : integer;
begin
with position do
for count := 1 to NumPts do
with PosArray[count] do
begin
if status.xAxis = stockA then
  a[count] := stockprice
 else
  a[count] := volatility;
case TypeOfGraph of
valueG : begin
           a[count+NumPts] := valuepend/1000;
           a[count+2*NumPts] := value/1000;
         end;
expryG : begin
           a[count+NumPts] := ExpyValue/1000;
           a[count+2*NumPts] := Value/1000;
         end;
gammaG : begin
           a[count+NumPts] := gammapend/1000;
           a[count+2*NumPts] := gamma/1000;
         end;
deltaG : begin
           a[count+NumPts] := deltapend/1000;
           a[count+2*NumPts] := delta/1000;
         end;
decayG : begin
           a[count+NumPts] := decaypend;
           a[count+2*NumPts] := decay;
         end;
end;
end;
end;

begin
  case TypeOfGraph of
  valueG : heading := 'Value :  ';
  gammaG : heading := 'Gamma :  ';
  deltaG : heading := 'Delta :  ';
  decayG : heading := 'Decay :  ';
  expryG : heading := 'Value :  ';
  end;
  DataToArray;
  if status.xAxis = stockA then
    DoGraph(a,40,2,TypeOfGraph,
          'Position ' + Heading+Position.CompanyName,'Stock-Price')
   else
    DoGraph(a,40,2,TypeOfGraph,
          'Position ' + Heading+Position.CompanyName,'% Volatility');
end;



End.
