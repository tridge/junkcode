#!/usr/bin/perl

my $fname = "foo\nbar";

open(FILE,">$fname") || die "Couldn't open $fname\n";

