{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit BLACK;

Interface

Uses
  Crt,
  Dos,
  Graph3,
  INITIAL,
  MISC;


Procedure NDF(Var x,N,h : real);
{ ...Normal Distribution Function... }

Procedure BS(volpct,s,p,rpct : real; T : integer; commod: commodtype;
                                     var TCV,DLT,GMA,DCY,DRV : real);
                                        {..CALCULATES BLACK SCHOLES FORMULA...}

Function InvertVol(Value,Stock,Strike,IRate :real;
          days:integer;Commod : commodtype) : Real;

Function WeightFactor(Stock,Strike : Real) : Real;
{...Calculates the Weight Factors for Implicit Volatility Calculations...}

Function ImplicitVol(Calc : CalcType) : Real;

Procedure recalc_BS_values;

Procedure recalc_USER_values;

Procedure recalc_Pos_Totals;

Procedure DoPosArray;

{===========================================================================}

Implementation


Procedure NDF(Var x,N,h : real);
{ ...Normal Distribution Function... }

Const
     c : Array[1..5] of real
         = (1.33027,-1.82126,1.78148,-0.35656,0.31938);
     a : Real = 0.39894;
     b : Real = 0.23164;
Var
   u,y : Real;
    i   : Integer;
Begin
     h := 0;
     If x > 5 then N := 1.0
     else
     If x <-5 then N := 0.0
     else
     begin
          u := 1/(1+b*Abs(x));
          h := a * Exp(-0.5*Sqr(x));
          y := 0.0;
          For i := 1 to 5 do
              y := u * (y + c[i]);
          If x > 0 then  N := 1 - h*y
                   else  N := h*y;
     end;
end; {...NDF...}


Procedure BS(volpct,s,p,rpct : real; T : integer; commod: commodtype;
                                     var TCV,DLT,GMA,DCY,DRV : real);
                                        {..CALCULATES BLACK SCHOLES FORMULA...}

var
  Tjul,q,Vt,r,vol : real;

Procedure BlackScholes;
{ ...Black-Scholes Calculations... }
Var
   Z,Z1,Z2,N1,N2,E1,E2 : Real;
Begin
  If Tjul > 0.0025 then
  begin
     Z  := Ln(P/Q)/Vt;
     Z1 := Z + Vt/2;
     Z2 := Z1 - Vt;
     NDF(Z1,N1,E1);
     NDF(Z2,N2,E2);
     TCV := P*N1 - Q*N2;
     DRV := P*Vt/vol*E1/100;
     DLT := N1;
     GMA := 100*E1/P/Vt;
     DCY := Q*(0.5*E2*Vt/Tjul + R*N2)/365;
  end

  else
  begin
     GMA := 0.0;
     If P > S then
     begin
          DLT := 1.0;
          TCV := P-S;
          DCY := S*R/365;
          DRV := 0;
     end
     else
     begin
          DLT := 0.0;
          TCV := 0.0;
          DCY := -S*R/365;
          DRV := 0;
     end;
  end;

end; {..BlackScholes..}

begin   { ...BS...}
if T < 0 then
  begin
  GMA := 0;
  TCV := 0;
  DLT := 0;
  DCY := 0;
  drv := 0;
  end
else
begin
Tjul := T/365;
r := rpct/100;
vol := volpct/100;
q := s*exp(-r*Tjul);
Vt := vol*sqrt(Tjul);
blackscholes;
if commod = Put then
begin
TCV := TCV - P + Q;
DLT := DLT-1;
DCY := DCY - (r*q/365);
end;
end;
end;    {..BS..}


Function InvertVol(Value,Stock,Strike,IRate :real;
          days:integer;Commod : commodtype) : Real;

Var
   PVStrike,Diff,IValue,NewCall,Cvalue : Real;
   Error,Vol,XTime,Slope,Dum,DelVol : Real;
Begin
     Xtime := days/365;
     PVStrike := Strike*Exp(-IRate*XTime/100);
          if commod = Put then
                     Cvalue := value + stock - PVstrike
                     else
                     Cvalue := value;
     Diff     := Stock-PVStrike;
     If Diff > 0 then IValue := Diff else IValue := 0;
     If (CValue >= Stock) or (CValue < 0.008) or (CValue < IValue) then
              InvertVol := -1
     else
     begin
          Vol := 100;
          Repeat
                BS(Vol,Strike,Stock,Irate,days,Call,NewCall,dum,dum,dum,Slope);
                Error := abs(NewCall-CValue)/CValue;
                if slope < 1e-10 then
                  begin
                  Error := 0;
                  DelVol := 1;
                  end
                 else
                  DelVol := (Cvalue - NewCall)/Slope;
                Vol := Vol + DelVol;
          Until (Error < 0.001) ;
          If abs(DelVol) > 0.1 then
            InvertVol := -2
          else
            InvertVol := Vol;
     end;
end;  {...InvertVol...}


Function WeightFactor(Stock,Strike : Real) : Real;
{...Calculates the Weight Factors for Implicit Volatility Calculations...}

Const
     MaxRelDist : Real = 0.5;
Var
   RelDist : Real;
Begin
     RelDist := abs(Stock-Strike)/Stock;
     If (RelDist < MaxRelDist) then
        WeightFactor := sqr(MaxRelDist-RelDist)
     else
        WeightFactor := -1;
end; {...WeightFactor...}


Function ImplicitVol(Calc : CalcType) : Real;

Var
   TempPointer : TransPointer;
   MonthCount,Days  : Integer;
   Stock1,Stock2,
   Vol,VolSum,
   W,WeightSum,
   CValue      : Real;
Begin
     VolSum := 0;
     WeightSum := 0;
     With Position do
     begin
          Stock1 := IntraArray[Status.CurrentTime];
          If (Calc = Calls) or (Calc = Both) then
          begin
               For MonthCount := 1 to 3 do
               begin
                    Days := Days_to_Expy(MonthCount);
                    TempPointer := CallSheet[MonthCount];
                    With TempPointer^ do
                    begin
                         Repeat
                               W := WeightFactor(Stock1,Strike);
                               If W > 0 then
                               begin
                                    CValue := (bider+asker)/2;
          Vol := InvertVol(Cvalue,Stock1,Strike,status.RiskFreeRate,Days,Call);
                                    if Vol > 0 then
                                    begin
                                    VolSum := VolSum + Vol*W;
                                    WeightSum := WeightSum+W;
                                    end;
                               end;
                               TempPointer := TempPointer^.NextRec;
                         Until TempPointer = nil;
                    end; {...With...}
               end; {...For...}
          end; {..if..}

          If (Calc = Puts) or (Calc = Both) then
          begin
               For MonthCount := 1 to 3 do
               begin
                    Days := Days_to_Expy(MonthCount);
                    TempPointer := PutSheet[MonthCount];
                    With TempPointer^ do
                    begin
                         Repeat
                               W := WeightFactor(Stock1,Strike);
                               If W > 0 then
                               begin
                                    CValue := (bider+asker)/2;
         Vol := InvertVol(Cvalue,Stock1,Strike,status.RiskFreeRate,Days,Put);
                                    if Vol > 0 then
                                    begin
                                    VolSum := VolSum + Vol*W;
                                    WeightSum := WeightSum+W;
                                    end;
                               end;
                               TempPointer := TempPointer^.NextRec;
                         Until TempPointer = nil;
                    end; {...With...}
               end; {...For...}
          end; {..if..}
     end; {..With Position..}
     if weightsum = 0 then
       implicitVol := -1
     else
       ImplicitVol := VolSum/WeightSum;
end; {...ImplicitVol...}


Procedure recalc_BS_values;

const
   HighOptionRange = 4;
   LowOptionRange = 2;
   ShareRange = 2;
var
  temppointer : transpointer;
  monthcount,T : integer;
  VolUp,VolDown,Deriv,dumVal,DumGam,
  Vol,DumDec,DumDel,currentprice : real;
begin
  currentprice := position.intraarray[status.currenttime];
  with position.sharesheet do
  begin
  bid := int(100*currentprice*(1 - random*ShareRange/100))/100;
  ask := int(100*currentprice*(1 + random*ShareRange/100))/100;
  if ask-bid < 0.01 then ask := ask + 0.01;
  end;
with position do
with status do
begin
  for monthcount := 1 to 3 do           {do call values}
  begin
  T := days_to_expy(monthcount);
  temppointer := callsheet[monthcount];
  repeat
  begin
  with temppointer^ do
  begin
  VolUp := LowOptionRange + random * HighOptionRange;
  VolDown := LowOptionRange + random * HighOptionRange;
  Vol := intravol[currenttime] + VolUp;
  BS(Vol,strike,currentprice,riskfreerate,T,Call,dumval,dumdel,dumgam,dumdec,Deriv);
  asker := int(100*dumval)/100;
  bider := asker - (VolUp+VolDown)*Deriv;
  if bider < 0 then bider := 0;
  if asker-bider < 0.01 then asker := asker + 0.01;
  end;
  temppointer := temppointer^.nextrec;
  end until temppointer = nil;
  end;

  for monthcount := 1 to 3 do          {now put values}
  begin
  T := days_to_expy(monthcount);
  temppointer := putsheet[monthcount];
  repeat
  begin
  with temppointer^ do
  begin
  VolUp := LowOptionRange + random * HighOptionRange;
  VolDown := LowOptionRange + random * HighOptionRange;
  Vol := intravol[currenttime] + VolUp;
  BS(Vol,strike,currentprice,riskfreerate,T,Put,dumval,dumdel,dumgam,dumdec,Deriv);
  asker := int(100*dumval)/100;
  bider := asker - (VolUp+VolDown)*Deriv;
  if bider < 0 then bider := 0;
  if asker-bider < 0.01 then asker := asker + 0.01;
  end;
  temppointer := temppointer^.nextrec;
  end until temppointer = nil;
  end;
end;
end;                   {....recalc_BS_values....}


Procedure recalc_USER_values;

var
  temppointer : transpointer;
  monthcount,T : integer;
  currentprice : real;
  dum,bidaskDiff,midvalue : real;
begin                         {....recalc_USER_values....}
with position do
with status do
begin
with sharesheet do
currentprice := (Bid + Ask)/2;
  for monthcount := 1 to 3 do           {do call values}
  begin
  T := days_to_expy(monthcount);
  temppointer := callsheet[monthcount];
  repeat
  begin
  with temppointer^ do
  begin
  BS(Workingvol,strike,currentprice,riskfreerate,T,Call,value,delta,gamma,decay,dum);
  if volmethod = 'Implied' then
  begin
  midvalue := (bider + asker)/2;
  ImpliedVol := InvertVol(midvalue,currentprice,strike,riskfreerate,T,Call);
  end;
  gamma := gamma/100;
  decay := decay * 100;
  end;
  temppointer := temppointer^.nextrec;
  end until temppointer = nil;
  end;

  for monthcount := 1 to 3 do          {now put values}
  begin
  T := days_to_expy(monthcount);
  temppointer := putsheet[monthcount];
  repeat
  begin
  with temppointer^ do
  begin
  BS(workingvol,strike,currentprice,riskfreerate,T,Put,value,delta,gamma,decay,dum);
  if volmethod = 'Implied' then
  begin
  midvalue := (bider + asker)/2;
  ImpliedVol := InvertVol(midvalue,currentprice,strike,riskfreerate,T,Put);
  end;
  gamma := gamma/100;
  decay := decay * 100;
  end;
  temppointer := temppointer^.nextrec;
  end until temppointer = nil;
  end;
position.RecalcUser := false;
end;
end;                   {....recalc_USER_values....}


Procedure recalc_Pos_Totals;

var
  temppointer : transpointer;
  monthcount,T : integer;
  currentprice : real;
  bidaskDiff,dum : real;
begin                         {....recalc_Pos_Totals....}
with position do
with status do
begin
  PosDelta := 0;
  PosGamma := 0;
  PosValue := 0;
  PosDecay := 0;
  currentprice := intraarray[currenttime];
  for monthcount := 1 to 3 do           {do call values}
  begin
  T := days_to_expy(monthcount);
  temppointer := callsheet[monthcount];
  repeat
  begin
  with temppointer^ do
  if amount <> 0 then
  begin
  PosDelta := PosDelta + 1000*delta*amount;
  PosGamma := PosGamma + gamma*amount;
  PosValue := PosValue + 1000*value*amount;
  PosDecay := PosDecay + 1000*decay*amount;
  end;
  temppointer := temppointer^.nextrec;
  end until temppointer = nil;
  end;

  for monthcount := 1 to 3 do          {now put values}
  begin
  T := days_to_expy(monthcount);
  temppointer := putsheet[monthcount];
  repeat
  begin
  with temppointer^ do
  if amount <> 0 then
  begin
  PosDelta := PosDelta + 1000*delta*amount;
  PosGamma := PosGamma + gamma*amount;
  PosValue := PosValue + 1000*value*amount;
  PosDecay := PosDecay + 1000*decay*amount;
  end;
  temppointer := temppointer^.nextrec;
  end until temppointer = nil;
  end;

end;
  currentprice := position.intraarray[status.currenttime];
  with position do
  with sharesheet do
  if amount <> 0 then
  begin
  PosDelta := PosDelta + amount;
  PosValue := PosValue + currentprice*amount;
  end;
end;                   {....recalc_Pos_Totals....}


Procedure DoPosArray;

const
    NumDataPts = 40;
var
    lowerlimit, upperlimit : real;
    division,price,vol : real;
    temppointer : transpointer;
    T,count,monthcount : integer;
    totalgamma,totaldelta,totalvalue,totaldecay,totalexpyvalue : real;
    posnvalue,posndelta,posngamma,posndecay,posnexpyvalue : real;
    zdelta,zdecay,zgamma,zvalue,zexpyvalue,dum,midprice : real;    {temporary storage}
    NumContracts : integer;
    AFlag,PFlag  : Boolean;
begin
with position do
with status do
begin
if xAxis = stockA then
 begin
 vol := workingvol;
 midprice := (Sharesheet.bid + Sharesheet.ask) /2;
 end
else
 begin
 price := (Sharesheet.bid + Sharesheet.ask) /2;
 midprice := intravol[currenttime];
 end;
 lowerlimit := midprice*(1-(PosGraphLim/100));
 upperlimit := midprice*(1+(PosGraphLim/100));
 if upperlimit > 95.0 then
   begin
   upperlimit := 95;
   lowerlimit := 2*midprice - upperlimit;
   end;
 division := (upperlimit-lowerlimit)/(numdatapts-1);
 for count := 1 to numdatapts do
 begin
totalvalue := 0; posnvalue := 0;
totalgamma := 0; posngamma := 0;
totaldelta := 0; posndelta := 0;
totaldecay := 0; posndecay := 0;
totalexpyvalue := 0; posnexpyvalue := 0;
 if xAxis = stockA then
 price := (count-1)*division + lowerlimit
 else
 vol := (count-1)*division + lowerlimit;
 if xAxis = stockA then
 PosArray[count].stockprice := price
 else
 PosArray[count].volatility := vol;
  for monthcount := 1 to 3 do           {do call values}
  begin
  T := days_to_expy(monthcount);
  temppointer := callsheet[monthcount];
  repeat
  with temppointer^ do
  begin
  NumContracts := Amount + Pending;
  AFlag := (Amount  <> 0);
  PFlag := (NumContracts <> 0);
  if (AFlag or PFlag) then
  begin
  BS(vol,strike,price,riskfreerate,T,Call,zvalue,zdelta,zgamma,zdecay,dum);
    If Price > Strike then
    begin
      zExpyValue := price-strike;
    end
    else
    begin
      zExpyValue := 0.0;
    end;
  If AFlag then
  begin
  posngamma := posngamma + zgamma*Amount;
  posndelta := posndelta + zdelta*Amount*1000;
  posndecay := posndecay + zdecay*Amount*1000;
  posnvalue := posnvalue + zvalue*Amount*1000;
  posnExpyValue := posnExpyValue + zExpyValue*Amount*1000;
  end;
  If PFlag then
  begin
  totalgamma := totalgamma + zgamma*NumContracts;
  totaldelta := totaldelta + zdelta*NumContracts*1000;
  totaldecay := totaldecay + zdecay*NumContracts*1000;
  totalvalue := totalvalue + zvalue*NumContracts*1000;
  totalExpyValue := totalExpyValue + zExpyValue*NumContracts*1000;
  end;
  end;
  end;
  temppointer := temppointer^.nextrec;
  until temppointer = nil;
 end;   {..MonthCount..}

  for monthcount := 1 to 3 do          {now put values}
  begin
  T := days_to_expy(monthcount);
  temppointer := putsheet[monthcount];
  repeat
  with temppointer^ do
  begin
  NumContracts := Amount + Pending;
  AFlag := (Amount  <> 0);
  PFlag := (NumContracts <> 0);
  if (AFlag or PFlag) then
  begin
  BS(vol,strike,price,riskfreerate,T,Put,zvalue,zdelta,zgamma,zdecay,dum);
    If Price < Strike then
    begin
      zExpyValue := strike-price;
    end
    else
    begin
      zExpyValue := 0.0;
    end;
  If AFlag then
  begin
  posngamma := posngamma + zgamma*Amount;
  posndelta := posndelta + zdelta*Amount*1000;
  posndecay := posndecay + zdecay*Amount*1000;
  posnvalue := posnvalue + zvalue*Amount*1000;
  posnExpyValue := posnExpyValue + zExpyValue*Amount*1000;
  end;
  If PFlag then
  begin

  totalgamma := totalgamma + zgamma*NumContracts;
  totaldelta := totaldelta + zdelta*NumContracts*1000;
  totaldecay := totaldecay + zdecay*NumContracts*1000;
  totalvalue := totalvalue + zvalue*NumContracts*1000;
  totalExpyValue := totalExpyValue + zExpyValue*NumContracts*1000;
  end;
  end;
  end;
  temppointer := temppointer^.nextrec;
  until temppointer = nil;
  end;

  With position do
  with sharesheet do
  Begin                              {now share values}
   If (Amount <> 0) then
   begin
   posndelta := posndelta + Amount;
   posnvalue := posnvalue + price*Amount;
   posnexpyvalue := posnexpyvalue + price*Amount;
   end;
   NumContracts := Amount + Pending;
   If (NumContracts <> 0) then
   begin
   totaldelta := totaldelta + NumContracts;
   totalvalue := totalvalue + price*NumContracts;
   totalexpyvalue := totalexpyvalue + price*NumContracts;
  end;
 end;

 With PosArray[count] do
 begin
 valuepend := totalvalue;
 deltapend := totaldelta;
 gammapend := totalgamma;
 decaypend := totaldecay;
 expyvaluepend := totalexpyvalue;
 value := posnvalue;
 delta := posndelta;
 gamma := posngamma;
 decay := posndecay;
 expyvalue := posnexpyvalue;
 end;
 end;
RecalcGraphs := false;
end;
end;




End.
