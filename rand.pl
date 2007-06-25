#!/usr/bin/perl -ws

use Data::Dumper;

my(@list) = split(/\n/, `cat rr-list`);

my($count)=$#list + 1;

print Dumper @list;
print "count=$count\n";


open(LOGFILE,">>rr.log");

print LOGFILE "hello\n";
