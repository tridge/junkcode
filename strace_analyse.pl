#!/usr/bin/perl -w

# analyse a strace in format given by strace -ttT
# Andrew Tridgell <tridge@samba.org> March 2006
# released under GNU GPL version 2 or later


my %totals;
my %cpu;
my $total;
my $total_cpu;

my $start = 0;

while (my $line = <>) {
	my $call;
	my $time;
	my $clock;
	if ($line =~ /^(\d+):(\d+):(\d+).(\d+) ([a-z0-9]+)\(.* <(\d[^>]+)/ ||
	    $line =~ /^\d+\s+(\d+):(\d+):(\d+).(\d+) ([a-z0-9]+)\(.* <(\d[^>]+)/) {
		$clock = ($1 * 3600) + ($2 * 60) + $3 + ($4 * 1.0e-6);
		$call = $5;
		$time = $6;
		$totals{$call} += $time;
		$total += $time;
		if ($start != 0) {
			$cpu{$call} += $clock - $start;
			$total_cpu += $clock - $start;
		}
		$start = $clock + $time;
	}
}


print"
  CALL       time(sec) percent
  -----------------------------
";

foreach my $c (sort { $totals{$b} <=> $totals{$a} } keys %totals) {
	printf "  %-13s %6.2f  %5.2f%%\n", $c, $totals{$c}, 100.0*$totals{$c}/$total, 
}
printf("  %-13s %6.2f\n", "TOTAL", $total);

print"
  CALL        cpu(sec) percent
  -----------------------------
";

foreach my $c (sort { $cpu{$b} <=> $cpu{$a} } keys %cpu) {
	printf "  %-13s %6.2f  %5.2f%%\n", $c, $cpu{$c}, 100.0*$cpu{$c}/$total_cpu, 
}
printf("  %-13s %6.2f\n", "TOTAL", $total_cpu);
