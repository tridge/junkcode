#!/usr/bin/perl -w


#############################################
# process a file, translating anything that 
# might be an notes email address into internet 
# emails

my $maxgrp1 = 0;
my $maxgrp2 = 0;
my $culprit = 0;
my $culpritN = 0;

my $ll = 1;

while (my $line = <>) {
	chomp $line;
	my @ids = split(/\,/, $line);
	my @ids2;
	my %hash;

	$hash = {};

	if ($#ids > $maxgrp1) {
		$maxgrp1 = $#ids;
	}

	for (my $i=0;$i <= $#ids; $i++) {
		my $id = $ids[$i] + 1 - 1;
		$hash{$id} = 1;
	}

	foreach my $i (keys %hash) {
		print "$i,";
	}

	@ids2 = (keys %hash);

	if ($#ids2 > $maxgrp2) {
		$maxgrp2 = $#ids2;
		$culprit = $ll;
		$culpritN = $#ids;
	}

	print "\n";
	$ll++;
}

print "maxgrp1=$maxgrp1\n";
print "maxgrp2=$maxgrp2\n";
print "culprit=$culprit\n";
print "culpritN=$culpritN\n";
