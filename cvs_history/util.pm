###################################################
# utility functions to support pidl
# Copyright (C) tridge@samba.org, 2001
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

package util;

use CGI::Base;
use Data::Dumper;

#####################################################################
# check if a string is in an array
sub InArray($$)
{
    my $s = shift;
    my $a = shift;
    for my $v (@{$a}) {
	if ($v eq $s) { return 1; }
    }
    return 0;
}

#####################################################################
# flatten an array of arrays into a single array
sub FlattenArray($) 
{ 
    my $a = shift;
    my @b;
    for my $d (@{$a}) {
	for my $d1 (@{$d}) {
	    push(@b, $d1);
	}
    }
    return \@b;
}

#####################################################################
# flatten an array of hashes into a single hash
sub FlattenHash($) 
{ 
    my $a = shift;
    my %b;
    for my $d (@{$a}) {
	for my $k (keys %{$d}) {
	    $b{$k} = $d->{$k};
	}
    }
    return \%b;
}


#####################################################################
# return the modification time of a file
sub FileModtime($)
{
    my($filename) = shift;
    return (stat($filename))[9];
}


#####################################################################
# read a file into a string
sub FileLoad($)
{
    my($filename) = shift;
    local(*INPUTFILE);
    open(INPUTFILE, $filename) || return "";
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
# return a filename with a changed extension
sub ChangeExtension($$)
{
    my($fname) = shift;
    my($ext) = shift;
    if ($fname =~ /^(.*)\.(.*?)$/) {
	return "$1.$ext";
    }
    return "$fname.$ext";
}

#####################################################################
# save a data structure into a file
sub SaveStructure($$)
{
    my($filename) = shift;
    my($v) = shift;
    FileSave($filename, Dumper($v));
}

#####################################################################
# load a data structure from a file (as saved with SaveStructure)
sub LoadStructure($)
{
    return eval FileLoad(shift);
}


####################################################################
# setup for gzipped output of a web page if possible. 
# based on cvsweb.pl method
# as a side effect this function adds the final line ot the HTTP headers
sub cgi_gzip()
{
    my $paths = ['/usr/bin/gzip', '/bin/gzip'];
    my $GZIPBIN;
    my $Browser = $ENV{'HTTP_USER_AGENT'} || "";

#  newer browsers accept gzip content encoding
# and state this in a header
# (netscape did always but didn't state it)
# It has been reported that these
#  braindamaged MS-Internet Exploders claim that they
# accept gzip .. but don't in fact and
# display garbage then :-/
# Turn off gzip if running under mod_perl. piping does
# not work as expected inside the server. One can probably
# achieve the same result using Apache::GZIPFilter.
    my $maycompress = (($ENV{'HTTP_ACCEPT_ENCODING'} =~ m|gzip|
			|| $Browser =~ m%^Mozilla/3%)
		       && ($Browser !~ m/MSIE/)
		       && !defined($ENV{'MOD_PERL'}));
    
    if (!$maycompress) { print "\r\n"; return; }

    for my $p (@{$paths}) {
	if (stat($p)) { $GZIPBIN = $p; }
    }

    my $fh = do {local(*FH);};

    if (stat($GZIPBIN) && open($fh, "|$GZIPBIN -1 -c")) {
	print "Content-encoding: x-gzip\r\n";
	print "Vary: Accept-Encoding\r\n";  #RFC 2068, 14.43
	print "\r\n"; # Close headers
	$| = 1; $| = 0; # Flush header output
	select ($fh);
	print "<!-- gzip encoded --!>\n";
    } else {
	print "\r\n";
    }
}

##########################################
# escape a string for cgi
sub cgi_escape($)
{
    my $s = shift;

    return (html_escape($s))[0];
}

##########################################
# count the number of lines in a buffer
sub count_lines($)
{
    my $s = shift;
    my $count;
    $count = split(/$/m, $s);
    return $count;
}

################
# display a time as days, hours, minutes
sub dhm_time($)
{
	my $sec = shift;
	my $days = int($sec / (60*60*24));
	my $hour = int($sec / (60*60)) % 24;
	my $min = int($sec / 60) % 60;

	my $ret = "";

	if ($sec < 0) { 
		return "-";
	}

	if ($days != 0) { 
		return sprintf("%dd %dh %dm", $days, $hour, $min);
	}
	if ($hour != 0) {
		return sprintf("%dh %dm", $hour, $min);
	}
	if ($min != 0) {
		return sprintf("%dm", $min);
	}
	return sprintf("%ds", $sec);
}

1;

