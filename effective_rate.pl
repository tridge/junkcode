#!/usr/bin/perl -w

use strict;

if ($#ARGV < 1) {
	print "
Usage: effective_rate.pl <advertised_rate> <days_between_interest_credit>
";
	exit(0);
}

sub compound_rate($$)
{
	my $rate = shift;
	my $interest_period = shift;
	my $value = 1.0;
	my $daily_rate = $rate / 365.0;

	my $limbo = 0;
	for (my $day=1; $day <= 365.0; $day++) {
		$limbo += ($daily_rate/100.0) * $value;
		if ($day % $interest_period == 0) {
			$value += $limbo;
			$limbo = 0;
		}
	}
	$value += $limbo;

	return 100 * ($value - 1.0);
}

my $rate = shift;
my $interest_period = shift;

my $real_rate = compound_rate($rate, $interest_period);

printf "Compound rate %.2f\n", $real_rate;
