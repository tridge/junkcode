{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit MS;

Interface

Uses
  Crt,
  Dos,
  Graph3,
  INITIAL,
  DATA,
  MISC;




Procedure rotate(num : integer);

Procedure createmonths;               { initialises the months array }

Procedure initsheets;

Procedure addstrike(strikeprice : real;totop : boolean);

Function Interval(price : real) : real;

Function nextup(price : real) : real;

Function nextdown(price : real) : real;

Procedure createstrikes;

Procedure createmarket;

Procedure Check4Limit;

Procedure NewMonth;

Procedure DoExpy;

Procedure Check4Expy;

{===========================================================================}

Implementation


Procedure rotate(num : integer);

var
  temp : monthtype;
  i,j : integer;
begin
  for i := 1 to num do
  with position do
  begin
  temp := months[1];
  for j:= 1 to 3 do
  months[j] := months[j+1];
  months[4] := temp;
  end;
end;


Procedure createmonths;               { initialises the months array }

var
  seed : real;
  month1,month2,month3 : strikemonthtype;
  count1,count2 : integer;

begin
with position do
with status do
begin                           {....createmonths....}
count2 := 1;
for count1 := 1 to 4 do
begin
month1[count1] := monthsarray[count2];
month2[count1] := monthsarray[count2 + 1];
month3[count1] := monthsarray[count2 + 2];
count2 := count2 + 3;
end;
seed := random;
if seed < 0.333 then
begin
months := month1;
while not between(days_to_expy(1),1,93) do
 rotate(1);
end
else if seed < 0.666 then
begin
months := month2;
while not between(days_to_expy(1),1,93) do
 rotate(1);
end
else
begin
months := month2;
while not between(days_to_expy(1),1,93) do
 rotate(1);
end;
end;
end;                              {....createmonths....}


Procedure initsheets;

var
  monthcount : integer;
begin
for monthcount := 1 to 4 do
with position do
begin
callsheet[monthcount] := nil;
putsheet[monthcount] := nil;
end;
end;



Procedure addstrike(strikeprice : real;totop : boolean);

var
  monthcount : integer;
  temppointer1,temppointer2 : transpointer;
begin
  Position.NumStrikes := Position.NumStrikes + 1;
  for monthcount := 1 to 4 do
  with position do
  begin
  temppointer1 := callsheet[monthcount];
  temppointer2 := putsheet[monthcount];

  if not totop then
  begin
  if temppointer1 = nil then
  begin
  new(callsheet[monthcount]);
  temppointer1 := callsheet[monthcount];
  new(putsheet[monthcount]);
  temppointer2 := putsheet[monthcount];
  end else
    begin
     while temppointer1^.nextrec <> nil do
     begin
     temppointer2 := temppointer2^.nextrec;
     temppointer1 := temppointer1^.nextrec;
     end;
     new(temppointer1^.nextrec);
     temppointer1 := temppointer1^.nextrec;
     new(temppointer2^.nextrec);
     temppointer2 := temppointer2^.nextrec;
    end;

  with temppointer1^ do
  begin
   month := months[monthcount];
   strike := strikeprice;
   amount := 0;
   Pending := 0;
   tagged := false;
   value := 0;
   nextrec := nil;
  end;
  temppointer2^ := temppointer1^;
  end
  else
  begin                  {...if to top}
  new(callsheet[monthcount]);
  new(putsheet[monthcount]);
  with callsheet[monthcount]^ do
  begin
   month := months[monthcount];
   strike := strikeprice;
   amount := 0;
   pending := 0;
   tagged := false;
   value := 0;
   nextrec := temppointer1;
  end;
  putsheet[monthcount]^ := callsheet[monthcount]^;
  putsheet[monthcount]^.nextrec := temppointer2;
  end;                   {...if to top}

  end;
end;


Function Interval(price : real) : real;

begin
if between(price,2000,1000) then interval := 50 else
if between(price,1000,500) then interval := 20 else
if between(price,500,100) then interval := 10 else
if between(price,100,50) then interval := 5 else
if between(price,50,20) then interval := 2 else
if between(price,20,10) then interval := 1 else
if between(price,10,2) then interval := 0.5 else
if between(price,2,1.8) then interval := 0.2 else
if between(price,1.8,0) then interval := 0.1
else interval := 100;
end;


Function nextup(price : real) : real;

var
  step : real;
begin
step := interval(price);
nextup := step*int(price/step) + step;
end;


Function nextdown(price : real) : real;

var
  step : real;
begin
step := interval(price);
nextdown := step*int(price/step);
end;


Procedure createstrikes;

var
  day : integer;

begin                               {....CREATESTRIKES....}
initsheets;
with position do
with status do
begin
 uplimit := nextup(closingarray[-200]);
 downlimit := nextdown(closingarray[-200]);
 addstrike(downlimit,false);
 addstrike(uplimit,false);
 for day := -199 to currentday do
 begin
  if closingarray[day] > uplimit then
  begin
  uplimit := nextup(closingarray[day]);
  addstrike(uplimit,false);
  end else
  if closingarray[day] < downlimit then
  begin
  downlimit := nextdown(closingarray[day]);
  addstrike(downlimit,true)
  end;
 end;
end;
end;                                 {....CREATESTRIKES....}


Procedure createmarket;

begin                                {....CREATEMARKET....}
createmonths;
createstrikes;
position.sharesheet.amount := 0;
position.sharesheet.pending := 0;
end;                                 {....CREATEMARKET....}


Procedure Check4Limit;

begin
with position do
with status do
begin
  if intraarray[currenttime] > uplimit then
  begin
  uplimit := nextup(intraarray[currenttime]);
  addstrike(uplimit,false);
  end else
  if intraarray[currenttime] < downlimit then
  begin
  downlimit := nextdown(intraarray[currenttime]);
  addstrike(downlimit,true)
  end;
end;
end;


Procedure NewMonth;

var
  temppointer : transpointer;
  monthcount : integer;
begin
rotate(1);
with position do
begin
temppointer := callsheet[1];
for monthcount := 1 to 3 do
 callsheet[monthcount] := callsheet[monthcount + 1];
callsheet[4] := temppointer;
temppointer := putsheet[1];
for monthcount := 1 to 3 do
 putsheet[monthcount] := putsheet[monthcount + 1];
putsheet[4] := temppointer;
end;
end;


Procedure DoExpy;

var
  commod : commodtype;
  temppointer : transpointer;
  cost, transcost : real;
begin
with position do
with status do
for commod := Call to Put do
begin
if commod = Call then
temppointer := callsheet[1]
else
temppointer := putsheet[1];
repeat
begin
with temppointer^ do
if amount <> 0 then
begin
cost := amount * value * 1000;
transcost := transaccost(cost,commod);
capital := capital + cost - transcost;
amount := 0;
end;
temppointer := temppointer^.nextrec;
end until temppointer = nil;
end;
newmonth;
end;


Procedure Check4Expy;

begin
if (Days_To_Expy(1) <= 0) then
DoExpy;
end;



End.
