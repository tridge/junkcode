{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit MISC2;

Interface

Uses
  Crt,
  Dos,
  INITIAL,
  MISC,
  TRADING,
  DATA,
  MS,
  BLACK,
  DRAW2,
  IO;


Procedure newday;

Procedure newhour;

Procedure NewWeek;


{================================================================ ==========}

Implementation


Procedure newday;

begin
with status do
begin
if currentday > 199 then DoErrorMessage('END OF GAME')
else
begin
if currenttime < 8 then
begin
currenttime := currenttime + 1;
recalc_BS_values;
fillpendingcontracts;
Check4Limit;
CalculateMargin;
if dayofweek = 4 then
currentday := currentday + 3
else currentday := currentday + 1;
DayOfWeek := (DayOfWeek + 1) mod 5;
currenttime := 1;
makeintraarray(currentday);
end
else
begin
if dayofweek = 4 then
currentday := currentday + 3
else currentday := currentday + 1;
DayOfWeek := (DayOfWeek + 1) mod 5;
currenttime := 1;
makeintraarray(currentday);
recalc_BS_values;
fillpendingcontracts;
CalculateMargin;
check4limit;
end;
if (days_to_expy(1) < 0) then
begin
DoExpy;
recalc_BS_values;
recalc_user_values;
end;
end;
end;
end;


Procedure newhour;

begin
with status do
begin
if currenttime >= 8 then newday
else
begin
currenttime := currenttime + 1;
recalc_BS_values;
fillpendingcontracts;
check4limit;
end;
end;
end;


Procedure NewWeek;

var
  count : integer;
begin
newday;
with position do
with status do
begin
for count := 2 to 5 do
begin
if dayofweek = 4 then
currentday := currentday + 3
else currentday := currentday + 1;
DayOfWeek := (DayOfWeek + 1) mod 5;
intraarray[currenttime] := closingarray[currentday-1];
recalc_BS_values;
check4limit;
if (days_to_expy(1) < 0) then
DoExpy;
end;
currenttime := 1;
recalc_BS_values;
CalculateMargin;
                check4limit;
end;
end;



End.
