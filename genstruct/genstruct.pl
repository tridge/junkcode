#!/usr/bin/perl -w
# a simple system for generating C parse info
# this can be used to write generic C structer load/save routines
# Copyright 2002 Andrew Tridgell <tridge@samba.org>
# released under the GNU General Public License v2 or later

use strict;
use Getopt::Long;

my($opt_parsers) = 0;

###################################################
# general handler
sub handle_general($$$$$$$)
{
	my($name) = shift;
	my($type) = shift;
	my($ptr_count) = shift;
	my($size) = shift;
	my($element) = shift;
	my($handler) = shift;
	my($flags) = shift;
	my($array_len) = 0;
	my($dynamic_len) = "NULL";

	# handle arrays, currently treat multidimensional arrays as 1 dimensional
	while ($element =~ /(.*)\[(.*?)\]$/) {
		$element = $1;
		if ($array_len == 0) {
			$array_len = $2;
		} else {
			$array_len = "$2 * $array_len";
		}
	}

	if ($flags =~ /_LEN\((\w*?)\)/) {
		$dynamic_len = "\"$1\"";
	}

	print CFILE "{\"$element\", $type, $ptr_count, $size, offsetof(struct $name, $element), $array_len, $handler, $dynamic_len},\n";
}


####################################################
# parse one element
sub parse_one($$$$)
{
	my($name) = shift;
	my($type) = shift;
	my($element) = shift;
	my($flags) = shift;
	my($tstr) = "";
	my($ptr_count) = 0;
	my($handler) = "NULL";
	my(%typemap) = (
		       "double" => "T_DOUBLE",
		       "float" => "T_FLOAT",
		       "int" => "T_INT",
		       "unsigned int" => "T_UNSIGNED",
		       "unsigned" => "T_UNSIGNED",
		       "long" => "T_LONG",
		       "unsigned long" => "T_ULONG",
		       "char" => "T_CHAR",
		       "unsigned char" => "T_CHAR",
		       "time_t" => "T_TIME_T"
		       );

	my($size) = "sizeof($type)";
	
	# make the pointer part of the base type 
	while ($element =~ /^\*(.*)/) {
		$type = "$type*";
		$element = $1;
	}

	# look for pointers 
	while ($type =~ /(.*)\*/) {
		$ptr_count++;
		$type = $1;
	}

	if ($type eq "char" && $ptr_count > 0) {
		handle_general($name, "T_STRING", $ptr_count-1, "sizeof(char *)", 
			       $element, $handler, $flags);
		return;
	}

	if ($typemap{$type}) {
		$tstr = $typemap{$type};
	} elsif ($type =~ /^enum/) {
		$tstr = "T_ENUM";
	} elsif ($type =~ /struct (.*)/) {
		$tstr = "T_STRUCT";
		$handler = "pinfo_$1";
	}

	# might be an unknown type
	if ($tstr eq "") {
		print "skipping unknown type '$type'\n";
		return;
	}
	handle_general($name, $tstr, $ptr_count, $size, $element, $handler, $flags);
}

####################################################
# parse one element
sub parse_element($$$)
{
	my($name) = shift;
	my($element) = shift;
	my($flags) = shift;
	my($type);
	my($data);

	# pull the base type
	if ($element =~ /^struct (\S*) (.*)/) {
		$type = "struct $1";
		$data = $2;
	} elsif ($element =~ /^enum (\S*) (.*)/) {
		$type = "enum $1";
		$data = $2;
	} elsif ($element =~ /^unsigned (\S*) (.*)/) {
		$type = "unsigned $1";
		$data = $2;
	} elsif ($element =~ /^(\S*) (.*)/) {
		$type = $1;
		$data = $2;
	} else {
		die "Can't parse element '$element'";
	}

	# handle comma separated lists 
	while ($data =~ /(\S*),[\s]?(.*)/) {
		parse_one($name, $type, $1, $flags);
		$data = $2;
	}
	parse_one($name, $type, $data, $flags);
}


####################################################
# parse the elements of one structure
sub parse_elements($$)
{
	my($name) = shift;
	my($elements) = shift;

	print "Parsing struct $name\n";

	print CFILE "static struct parse_struct pinfo_" . $name . "[] = {\n";

	while ($elements =~ /.*^([a-z].*?);\s*?(\S*?)\s*?\n(.*)/si) {
		my($element) = $1;
		my($flags) = $2;
		$elements = $3;
		parse_element($name, $element, $flags);
	}

	print CFILE "{NULL, 0, 0, 0, 0, 0, NULL}};\n\n";

	if ($opt_parsers) {
		# now the dump function
		print CFILE "
/* dump a $name structure */
char *dump_$name(const struct $name *$name) 
{
	return gen_dump(pinfo_$name, (const char *)$name, 0);
}

";
}
}


#####################################################################
# read a file into a string
sub FileLoad($)
{
	my($filename) = shift;
	local(*INPUTFILE);
	open(INPUTFILE, $filename) || die "can't open $filename";    
	my($saved_delim) = $/;
	undef $/;
	my($data) = <INPUTFILE>;
	close(INPUTFILE);
	$/ = $saved_delim;
	return $data;
}


####################################################
# parse a header file, generating a dumper structure
sub parse_header($)
{
	my($header) = shift;

	my($data) = FileLoad($header);

	# handle includes
	while ($data =~ /(.*?)\n\#include \"(.*?)\"(.*)/s) {
		my($inc) = FileLoad($2);

		if (!defined($inc)) {$inc = "";}

		$data = $1 . $inc . $3;
	}

	# strip comments
	while ($data =~ /(.*?)\/\*(.*?)\*\/(.*)/s) {
		$data = $1 . $3;
	}

	# collapse spaces 
	while ($data =~ s/[\t ][\t ]/ /s) {}
	while ($data =~ s/\n[\n\t ]/\n/s) {}

	print CFILE "/* This is an automatically generated file - DO NOT EDIT! */\n\n";

	# parse into structures 
	while ($data =~ /\nGENSTRUCT struct (\w*?) {\n(.*?)};(.*)/s) {
		my($name) = $1;
		my($elements) = $2;
		$data = $3;
		parse_elements($name, $elements);
	}
}


my($opt_header) = "";
my($opt_cfile) = "";
my($opt_help) = 0;

#########################################
# display help text
sub ShowHelp()
{
    print "
generator for C structure dumpers
Copyright tridge\@samba.org

Options:
    --help                this help page
    --header=SOURCE       the header to parse
    --cfile=DEST          the C file to produce
    --parsers             enable producing parser fns
";
    exit(0);
}


#########################################
# main program
#########################################
GetOptions (
	    'help|h|?' => \$opt_help, 
	    'header=s' => \$opt_header,
	    'cfile=s' => \$opt_cfile,
	    'parsers' => \$opt_parsers
	    );

if ($opt_help) {
    ShowHelp();
}

if (!$opt_header) {
	die "You must specify a header file to parse\n";
}

if (!$opt_cfile) {
	die "You must specify a C file to output the parser info\n";
}

open(CFILE, ">$opt_cfile") || die "can't open $opt_cfile";    

parse_header($opt_header);
