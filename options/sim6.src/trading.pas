{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {No numeric coprocessor}

Unit TRADING;

Interface

Uses
  INITIAL,
  MISC;


Procedure recalc_Pos_Value(var PositionValue : real);

Procedure CalculateMargin;

Procedure
 add_to_pending(Month:monthtype;strike:real;commod:commodtype;number:integer);

Procedure FillPendingContracts;

{===========================================================================}

Implementation


Procedure recalc_Pos_Value(var PositionValue : real);

var
  temppointer : transpointer;
  monthcount,T : integer;
  currentprice : real;
begin                         {....recalc_Pos_Value....}
with position do
with status do
begin
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
  if amount < 0 then
    PositionValue := PositionValue + asker * amount * 1000
   else
    PositionValue := PositionValue + bider * amount * 1000;
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
  if amount < 0 then
    PositionValue := PositionValue + asker * amount * 1000
   else
    PositionValue := PositionValue + bider * amount * 1000;
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
  if amount < 0 then
    PositionValue := PositionValue + ask * amount
   else
    PositionValue := PositionValue + bid * amount;
  end;
end;                   {....recalc_Pos_Value....}





Procedure CalculateMargin;

var
  PositionValue : real;
begin
Recalc_Pos_Value(PositionValue);
with position do
begin
  Capital := Capital + CompanyMargin;
  if PosValue < 0 then
    CompanyMargin := -PosValue
   else
    CompanyMargin := 0;
  Capital := Capital - CompanyMargin;
end;
end;


Procedure
 add_to_pending(Month:monthtype;strike:real;commod:commodtype;number:integer);

var
  temppointer : transpointer;
  cost,transcost : real;  {cost of options and transaction costs}
  TransOK : boolean;
  monthindex : integer;

begin
transOK := true;
with position do
with status do
begin
case commod of
Put,Call :  begin
        monthindex := 1;
        while transOK and
        not (uppercase(Months[monthindex]) = uppercase(Month)) do
        begin
         monthindex := monthindex + 1;
         if monthindex > 3 then transOK := false;
        end;
        if transOK then
        if commod = Put then
        temppointer := putsheet[monthindex]
        else
        temppointer := callsheet[monthindex];
        while transOK and not ((strike - temppointer^.strike) < 0.005) do
        begin
         if temppointer = nil then transOK := false;
         temppointer := temppointer^.nextrec;
        end;
        if not transOK then
        burp
        else
        with temppointer^ do
             begin

             {first remove old pending costs etc.}
             if pending <> 0 then
             begin
              if pending > 0 then
             begin
               cost := asker * pending * 1000;
               Transcost := TransacCost(cost,commod);
               PendingCost := PendingCost - cost - TransCost;
             end
              else
             begin
               cost := -bider * pending * 1000;
               Transcost := TransacCost(cost,commod);
               PendingCost := PendingCost - TransCost;
             end;
             end;

             {now add the new costs}
             Pending := Pending + number;
              if pending > 0 then
             begin
               cost := asker * pending * 1000;
               Transcost := TransacCost(cost,commod);
               PendingCost := PendingCost + cost + TransCost;
             end
              else
             begin
               cost := -bider * pending * 1000;
               Transcost := TransacCost(cost,commod);
               PendingCost := PendingCost + TransCost;
             end;

        end;
        end;
Share : with sharesheet do
             begin

             {first remove old pending costs etc.}
             if pending <> 0 then
             begin
              if pending > 0 then
             begin
               cost := ask * pending;
               Transcost := TransacCost(cost,share);
               PendingCost := PendingCost - cost - TransCost;
             end
              else
             begin
               cost := -bid * pending;
               Transcost := TransacCost(cost,share);
               PendingCost := PendingCost - TransCost;
             end;
             end;

             {now add the new costs}
             Pending := Pending + number;
              if pending > 0 then
             begin
               cost := ask * pending;
               Transcost := TransacCost(cost,share);
               PendingCost := PendingCost + cost + TransCost;
             end
              else
             begin
               cost := -bid * pending;
               Transcost := TransacCost(cost,share);
               PendingCost := PendingCost + TransCost;
             end;
           end;
end;
end;
end;


Procedure FillPendingContracts;

var
  temppointer : transpointer;
  monthcount : integer;
  cost,transcost,Margin : real;
  commod : commodtype;
begin
with position do
with status do
begin
PendingCost := 0;
for commod := Call to Put do
for monthcount := 1 to 3 do
begin
if commod = Call then
  temppointer := callsheet[monthcount]
 else
  temppointer := putsheet[monthcount];
repeat
with temppointer^ do
 if  pending <> 0 then
 begin
  if pending > 0 then
    begin
      cost := pending * Bider * 1000;
      transcost := TransacCost(cost,commod);
      if (Capital - Cost - TransCost) >= 0 then
      begin
        Capital := Capital - Cost - TransCost;
        amount := amount + pending;
      end;
      pending := 0;
    end
   else
    begin
      cost := -pending * asker * 1000;
      transcost := TransacCost(cost,commod);
      if (Capital - TransCost) >= 0 then
      begin
        Capital := Capital - TransCost;
        CompanyMargin := CompanyMargin + cost;
        amount := amount + pending;
      end;
      pending := 0;
    end
 end;
temppointer := temppointer^.nextrec;
until temppointer = nil;
end;



with sharesheet do
 if  pending <> 0 then
 begin
  if pending > 0 then
    begin
      cost := pending * Bid;
      transcost := TransacCost(cost,share);
      if (Capital - Cost - TransCost) >= 0 then
      begin
        Capital := Capital - Cost - TransCost;
        amount := amount + pending;
      end;
      pending := 0;
    end
   else
    begin
      cost := -pending * ask;
      transcost := TransacCost(cost,share);
      if (Capital - TransCost) >= 0 then
      begin
        Capital := Capital - TransCost;
        CompanyMargin := CompanyMargin + cost;
        amount := amount + pending;
      end;
      pending := 0;
    end
 end;
end;
end;




End.
