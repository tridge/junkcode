#!/usr/bin/perl -w

my $ip_file = "$ENV{HOME}/.spam_ips";

#####################################################################
# load a data structure from a file (as saved with SaveStructure)
sub LoadStructure($)
{
	my $f = shift;
	my $contents = FileLoad($f);
	return eval "$contents";
}


use strict;
use Data::Dumper;

#####################################################################
# read a file into a string
sub FileLoad($)
{
    my($filename) = shift;
    local(*INPUTFILE);
    open(INPUTFILE, $filename) || return undef;
    my($saved_delim) = $/;
    undef $/;
    my($data) = <INPUTFILE>;
    close(INPUTFILE);
    $/ = $saved_delim;
    return $data;
}

#####################################################################
# write a string into a file
sub FileSave($$)
{
    my($filename) = shift;
    my($v) = shift;
    local(*FILE);
    open(FILE, ">$filename") || die "can't open $filename";    
    print FILE $v;
    close(FILE);
}

#####################################################################
# save a data structure into a file
sub SaveStructure($$)
{
    my($filename) = shift;
    my($v) = shift;
    FileSave($filename, Dumper($v));
}


##############
# main program

my $ip = 0;

while (my $line = <>) {
	if ($line =~ /^Received: from .*?\[([\d\.]+)\]/) {
		$ip = $1;
		last;
	}
}

if ($ip eq "127.0.0.1") {
	exit 0;
}

if ($ip) {
	my $ips = LoadStructure($ip_file);
	$ips->{$ip}++;
	SaveStructure($ip_file, $ips);
}
