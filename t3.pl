#!/usr/bin/perl -w


my($base)="/usr/cvs/nas/linux";

my($s)="/usr/cvs/nas/linux/foo/bar";

if ($s =~ /$base/) {
	print "matches\n";
}
