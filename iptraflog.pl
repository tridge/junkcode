#!/usr/bin/perl -w

use strict;

my $totals;
my $t_in;
my $t_out;

while (my $line = <>) {
	chop $line;
	if ($line =~ /^(\w+\/\d+):.*?(\d+) bytes incoming.*?(\d+) bytes outgoing/) {
		$totals->{$1}->{'in'} = $2;
		$totals->{$1}->{'out'} = $3;
	}
}

foreach my $protocol (sort keys %$totals) {
	my $p_in = $totals->{$protocol}->{in} / 1.0e6;
	my $p_out = $totals->{$protocol}->{out} / 1.0e6;
	$t_in += $totals->{$protocol}->{in};
	$t_out += $totals->{$protocol}->{out};
	print "$protocol  $p_in $p_out\n";
}

printf "Total in %.1f MB   Total out %.1f MB\n", $t_in/1.0e6, $t_out/1.0e6;

