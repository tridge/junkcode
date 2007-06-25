#!/usr/bin/perl -w

require POSIX;
require IO::Seekable;
use IO::Handle;
use IO::File;

my $fname = $ARGV[0];

sysopen(FILE,$fname,O_RDONLY) || die "Couldn't open $fname\n";

$min = 0;
$max = 2**31;

for ( ; $min != $max ; ) {
    print "min=$min max=$max\n";
    $try = int(($min + $max) / 2) + 1;
    sysseek(FILE, $try, SEEK_SET);
    $x = sysread(FILE, $b, 1);
    if ($x == 1) {
	$min = $try;
    } else {
	$max = $try - 1;
    }
}

$try = $min + 1;

print "file is $try long\n";
