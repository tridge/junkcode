{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit DATA;

Interface

Uses
  INITIAL;


Procedure makeintraarray(day : integer);

Procedure makedata(zeroprice,volpct,rpct : real);

Function HistVol : real; {...Produce volatility from history...}

{===========================================================================}

Implementation

Procedure makeintraarray(day : integer);

const
     nsub = 10;
var
 ph,rhat,uH,dH,probH,h,r,vol : real;
zeroVOL, VOLph,VOLrhat,VOLu,VOLd,VOLprob,VOLh,VOLr,VOLvol : real;
   i,j : integer;

begin
with position do
begin
VOLvol := (5 + random*5)/100;
zeroVOL := volatilityarray[day]/100;
  r := status.riskfreerate/100;
  h:= 1/(Nsub*365);
  VOLr := 0.02;
  VOLu := exp(VOLvol*sqrt(h));
  VOLd := 1/VOLu;
  VOLrhat := VOLr/(365*nsub) + 1;
  VOLprob := (VOLrhat-VOLd)/(VOLu-VOLd);
  VOLph := zeroVOL;
  rhat := r/(365*nsub) + 1;
  ph := closingarray[day];
with position do
begin
  for i := 0 to 7 do
  begin
  for j:= 1 to nsub do
  begin

  if random > (1-VOLprob) then
     VOLph := VOLph*VOLd
  else VOLph := VOLph*VOLu;

  uH := exp(volpH*sqrt(h));
  dH := 1/uH;
  probH := (rhat-dH)/(uH-dH);

  if random > probH then
     ph := ph*uH
  else ph := ph*dH;
  end;
  intravol[8-i] := VOLpH*100;
  intraarray[8-i] := int(ph*100 + 0.5)/100;
  end;
end;
end;
end;



Procedure makedata(zeroprice,volpct,rpct : real);

const
     nsub = 5;
var
 ph,p,rhat,u,uH,d,dH,prob,probH,h,r,vol : real;
zeroVOL, VOLph,VOLp,VOLrhat,VOLu,VOLd,VOLprob,VOLh,VOLr,VOLvol : real;
   i,j : integer;

begin
VOLvol := (5 + random*5)/100;
zeroVOL := volpct/100;
  r := rpct/100;
  h:= 1/(Nsub*365);
  VOLr := 0.02;
  VOLu := exp(VOLvol*sqrt(h));
  VOLd := 1/VOLu;
  VOLrhat := VOLr/(365*nsub) + 1;
  VOLprob := (VOLrhat-VOLd)/(VOLu-VOLd);
  VOLp := zeroVOL;
  VOLph := VOLp;
  rhat := r/(365*nsub) + 1;
  p := zeroprice;
  ph := p;
with position do
begin
  closingarray[0] := zeroprice;
  volatilityarray[0] := zeroVOL*100;
  for i := 1 to 200 do          { This loop generates both }
  begin                         { past and future data     }
  for j:= 1 to nsub do
  begin

  if random > VOLprob then
     VOLp := VOLp*VOLd
  else VOLp := VOLp*VOLu;
  if random > (1-VOLprob) then
     VOLph := VOLph*VOLd
  else VOLph := VOLph*VOLu;

  u := exp(volp*sqrt(h));
  d := 1/u;
  prob := (rhat-d)/(u-d);

  uH := exp(volpH*sqrt(h));
  dH := 1/uH;
  probH := (rhat-dH)/(uH-dH);

  if random > prob then
     p := p*d
  else p := p*u;
  if random > probH then
     ph := ph*uH
  else ph := ph*dH;
  end;
  volatilityarray[i] := VOLp*100;
  volatilityarray[-i] := VOLph*100;
  closingarray[i] := int(p*100 + 0.5)/100;        {to nearest cent}
  closingarray[-i] := int(ph*100 + 0.5)/100;
  end;
end;
makeintraarray(0);  {initialise the hourly prices}
end;                              {...MAKEDATA...}


Function HistVol : real; {...Produce volatility from history...}

var
 length,k,numdays,err : integer;
 sigma2,sigma2hat,ratio,sum,temp,f : real;

begin                   {...HistVol...}
   val(copy(position.VolMethod,1,2),numdays,err);
   temp := 0;
   sum := 0;
   with status do
   for k:= (currentday-NumDays) to CurrentDay do
   begin
   with position do
    ratio := ln(closingarray[k]/closingarray[k-1]);
    temp := temp + sqr(ratio);
    sum := sum + ratio;
   end;
   sigma2 := temp/NumDays - sqr(sum/NumDays);
   sigma2hat := (sigma2*NumDays)/(NumDays-1);
   f:=1/365;
   HistVol := 100*sqrt(sigma2hat/f);

end;       {...HistVol....}



End.
