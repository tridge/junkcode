{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit INITIAL;

Interface
USES
  Crt;

TYPE
    fontchar = array[0..7] of byte;
    NumFontType = array[0..11] of fontchar;
    linetype = string[80];
    PlotArray = array[1..400] of real;
    Corners = array[1..4] of integer;
    monthtype  = string[3];
    datetype = string[8];
    Str11 = String[11];
    BitMap = array[1..8] of Byte;
    KeyType = (NoKey,ArrowKey,CharKey,ReturnKey,HelpKey);
    CommandStr = String[10];
    ArrowType = (None,Left,Right,Up,Down);
    StatusDisplayType = (Totals,Accounts);
    ToggleRec = record
                  MenuStr : Linetype;
                  NoItems : 1..10;
                  ItemSet : string[14];
                  Posn,Size: Array[1..10] of Byte;
                  Line    : byte;
                end;
    commodtype = (Call,Put,Share);
    CalcType = (Calls,Puts,Both);
    strikemonthtype = array[1..4] of monthtype;
    graphtype = (historyG,valueG,gammaG,deltaG,decayG,expryG);

   PosArrayRec = record
                 stockprice,Volatility : real;
                 ExpyValue,value,gamma,delta,decay : real;
                 ExpyValuePend,ValuePend,GammaPend,DeltaPend,DecayPend : real;
                 end;
   colorrec = record
               CommandBack,HighlightBack,CentreBack,StatusBack : byte;
               CommandFore,Highlight,CentreFore,HeadingFore,StockFore,PriceFore,
               DaysFore,AccountsFore : byte;
              end;
   HardWareRec = record
                 TextModeNo,GraphModeNo : byte;
                 CursorTopHid,CursorBotHid :byte;
                 MaxXgraf,MaxYgraf : integer;
                 GraphicsCard : (CGA,EGA,Hercules);
                 DefaultColors : colorrec;
                 end;
   statustype = record
                 Colors : colorrec;
                 HardWare : HardWareRec;
                 OptTranscost : real;
                 Sharetranscost : real;
                 PosGraphLim : real;      {% up and down for pos graphs}
                 currentday : 0..200;
                 currenttime : 0..8;
                 riskfreerate : real;
                 startdate : datetype;
                 DayOfWeek : 0..4;
                 VolWithPuts : boolean;
                 GotoMain : boolean;
                 CrashDay : integer;
                 PalNo : integer;
                 xAxis : (stockA,volatilityA);
                 end;
   sharerec = record
               amount : integer;
               pending : integer;
               tagged : boolean;
               Bid, Ask : real;
              end;
   transpointer = ^transacrec;
   transacrec = record
                 Month : monthtype;
                 strike : real;
                 amount : integer;
                 pending : integer;
                 value : real;
                 Bider : real;
                 Asker : real;
                 gamma,delta,decay : real;
                 tagged : boolean;
                 nextrec : transpointer;
                 ImpliedVol : real;
                end;
   OptionListType = array[Call..Put,1..99] of transacrec;
   Positionrec = record
                  companyname : string[255];
                  volatility : real;
                  workingvol : real;
                  marginheld : real;
                  VolMethod : string[7];
                  PosArray : array[1..40] of PosArrayRec;
                  callsheet : array[1..4] of transpointer;
                  putsheet : array[1..4] of transpointer;
                  sharesheet : sharerec;
                  closingarray : array[-200..200] of real;
                  volatilityarray : array[-200..200] of real;
                  intraarray : array[1..8] of real;
                  intravol : array[1..8] of real;
                  months : strikemonthtype;
                  numStrikes : integer;
                  UpLimit, DownLimit : real;
                  RecalcUser : Boolean;
                  RecalcGraphs : Boolean;
                  PosValue,PosDelta,PosDecay,PosGamma : real;
                  OptionList : OptionListType;
                  StatusDisplay : StatusDisplayType;
                  CurrentDisplay : commodtype;
                  TopLine : integer;
                  CompanyMargin : real;
                  capital : real;
                  PendingCost : real;
                 end;
       HelpScreenType = array[1..20] of string[50];
       HelpPointer = ^HelpArrayRec;
       HelpArrayRec = record
                      HelpScreen : HelpScreenType;
                      nexthelp : HelpPointer;
                      end;
       DisplayLineType = array[1..80,1..2] of byte;
       ScreenType = array[1..25] of DisplayLineType;
       Window2Type = array[0..20] of DisplayLineType;
       CommodScreenType = array[0..99] of DisplayLineType;
CONST
     monthsarray : array[1..12] of monthtype = ('Jan','Feb','Mar','Apr','May',
                                    'Jun','Jul','Aug','Sep','Oct','Nov','Dec');
     daysarray : array[0..4] of string[9] = ('Monday','Tuesday','Wednesday'
                                                       ,'Thursday','Friday');
     TimeArray : array[1..8] of string[5] = ('10:30','11:00','11:30','12:00',
                                                 '1:30','2:00','2:30','3:00');
     GrafBase = $B800;

NumFont : NumFontType = ((124,198,206,222,246,230,124,0)
                         ,(48,112,48,48,48,48,252,0)
                         ,(120,204,12,56,96,204,252,0)
                         ,(120,204,12,56,12,204,120,0)
                         ,(28,60,108,204,254,12,30,0)
                         ,(252,192,248,12,12,204,120,0)
                         ,(56,96,192,248,204,204,120,0)
                         ,(252,204,12,24,48,48,48,0)
                         ,(120,204,204,120,204,204,120,0)
                         ,(120,204,204,124,12,24,112,0)
                         ,(0,0,0,0,0,48,48,0)
                         ,(0,0,0,126,126,0,0,0));


VAR
   CommodScreens : array[Call..Put] of CommodScreenType;
   position : positionrec;
   status : statustype;
   ActiveCompany : boolean;
   ReDraw : boolean;
   HelpArray : HelpPointer;
   Window2 : Window2Type absolute GrafBase : 160;
   CursorTop,CursorBot : byte;
   PosSaveFile : file of Positionrec;
   StatSaveFile :file of StatusType;


Procedure WaitRetraceH;

Procedure WaitRetraceV;

Function GetAttr(ForeG,BackG : byte) : byte;

Procedure burp;

Procedure beep;

Function uppercase(line : linetype) : linetype;

Function SerialDate(year,month,day : integer) : real;

Function Between(num,top,bott : real) : boolean;

Function transaccost(cost : real; commodity : commodtype) : real;

{===========================================================================}

Implementation

Procedure WaitRetraceH;

begin
while ((Port[$3da] and 1) <> 1) do ;
while ((Port[$3da] and 1) = 1) do ;
end;


Procedure WaitRetraceV;

begin
if status.hardware.graphicscard = CGA then
begin
while ((Port[$3da] and 8) <> 8) do ;
while ((Port[$3da] and 8) = 8) do ;
end;
end;


Function GetAttr(ForeG,BackG : byte) : byte;

begin
GetAttr := (ForeG div 16)*128 + 16*BackG + (ForeG mod 16);
end;


Procedure burp;

begin
sound(100);
delay(300);
nosound;
end;


Procedure beep;

begin
sound(300);
delay(50);
nosound;
end;


Function uppercase(line : linetype) : linetype;

var
  count : integer;
  out : linetype;
begin
out := '';
for count := 1 to length(line) do
out := out + upcase(line[count]);
uppercase := out;
end;


Function SerialDate(year,month,day : integer) : real;
  
const
     monthdays : array[1..12] of integer =
       (0,31,59,90,120,151,181,212,243,273,304,334);
  var
    sdate : real;
begin
  sdate := 365.0 * year + (year + 3) div 4;
  if ((year mod 4 = 0) and (month  > 2)) then
  sdate := sdate + 1;
  SerialDate := sdate + monthdays[month] + day
end;



Function Between(num,top,bott : real) : boolean;

begin
between:=((num<top) and (num>=bott)) or ((num>=top) and (num<bott));
end;



Function transaccost(cost : real; commodity : commodtype) : real;

begin
case commodity of                                             {interim method}
Put,Call : transaccost :=abs(cost) * (status.OptTranscost/100);
Share : transaccost :=abs(cost) * (status.ShareTranscost/100);
end;
end;



End.
