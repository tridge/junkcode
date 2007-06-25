#!/usr/bin/perl -w

while (my $line = <>) {
	while ($line =~ /^.*[\W]([\w]*?\@samba.org)(.*)$/) {
		print "$1\n";
		$line = $2 . "\n";
	}
}
