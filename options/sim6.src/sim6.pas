{$R-}    {Range checking off}
{$B+}    {Boolean complete evaluation on}
{$S+}    {Stack checking on}
{$I+}    {I/O checking on}
{$N-}    {numeric coprocessor}
{$M 65500,16384,200000} {Turbo 3 default stack and heap}


Program simulator;



Uses
  IO,
  INITIAL,
  INSTALL,
  MISC,
  USER;



begin
     SaveCursor;
     MakeHelpArray;
     if not GetStatus then
     Installation;
     DoTitleScreen;
     test;
end.


