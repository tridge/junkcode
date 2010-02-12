#!/usr/bin/perl -w

my %data;
my $total = 0;

while (<>) {
	my $v = $_;
	chomp($v);
	$data{$v}++;
	$total++;
}

my $accum = 0;
my $line = 0;

foreach my $v (sort { $data{$a} <=> $data{$b} } keys %data) {
    $accum += $data{$v};
    printf "%d (%.0f%% / %.0f%%) %s  (%u)\n", 
    $data{$v}, 
    (100*$data{$v})/$total, 
    (100*$accum)/$total, 
    $v,
    $line;
    $line++;
}
