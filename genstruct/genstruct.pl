#!/usr/bin/perl -w
# a simple system for generating C parse info
# this can be used to write generic C structer load/save routines
# Copyright 2002 Andrew Tridgell <genstruct@tridgell.net>
# released under the GNU General Public License v2 or later

use strict;
use Getopt::Long;
use Data::Dumper;

# search path for include files
my($opt_include) = ".";
my(%already_included) = ();


###################################################
# general handler
sub handle_general($$$$$$$$)
{
	my($name) = shift;
	my($ptr_count) = shift;
	my($size) = shift;
	my($element) = shift;
	my($flags) = shift;
	my($dump_fn) = shift;
	my($parse_fn) = shift;
	my($tflags) = shift;
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

	if ($flags =~ /_NULLTERM/) {
		$tflags = "FLAG_NULLTERM";
	}

	print CFILE "{\"$element\", $ptr_count, $size, offsetof(struct $name, $element), $array_len, $dynamic_len, $tflags, $dump_fn, $parse_fn},\n";
}


####################################################
# parse one element
sub parse_one($$$$)
{
	my($name) = shift;
	my($type) = shift;
	my($element) = shift;
	my($flags) = shift;
	my($ptr_count) = 0;
	my($size) = "sizeof($type)";
	my($tflags) = "0";
	
	# enums get the FLAG_ALWAYS flag
	if ($type =~ /^enum /) {
		$tflags = "FLAG_ALWAYS";
	}


	# make the pointer part of the base type 
	while ($element =~ /^\*(.*)/) {
		$ptr_count++;
		$element = $1;
	}

	# convert spaces to _
	$type =~ s/ /_/g;

	my($dump_fn) = "gen_dump_$type";
	my($parse_fn) = "gen_parse_$type";

	handle_general($name, $ptr_count, $size, $element, $flags, $dump_fn, $parse_fn, $tflags);
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

	print CFILE "int gen_dump_struct_$name(struct parse_string *, const char *, unsigned);\n";
	print CFILE "int gen_parse_struct_$name(char *, const char *);\n";

	print CFILE "static const struct parse_struct pinfo_" . $name . "[] = {\n";

	while ($elements =~ /.*^([a-z].*?);\s*?(\S*?)\s*?\n(.*)/si) {
		my($element) = $1;
		my($flags) = $2;
		$elements = $3;
		parse_element($name, $element, $flags);
	}

	print CFILE "{NULL, 0, 0, 0, 0, NULL, 0, NULL, NULL}};\n";

	print CFILE "
int gen_dump_struct_$name(struct parse_string *p, const char *ptr, unsigned indent) {
	return gen_dump_struct(pinfo_$name, p, ptr, indent);
}
int gen_parse_struct_$name(char *ptr, const char *str) {
	return gen_parse_struct(pinfo_$name, ptr, str);
}

";
}


#####################################################################
# read a file into a string, handling the include path
sub IncludeLoad($)
{
	my($filename) = shift;
	my(@dirs) = split(/:/, $opt_include);

	for (my($i)=0; $i <= $#{@dirs}; $i++) {
		local(*INPUTFILE);
		if (open(INPUTFILE, $dirs[$i] . "/" . $filename)) {
			my($saved_delim) = $/;
			undef $/;
			my($data) = <INPUTFILE>;
			close(INPUTFILE);
			$/ = $saved_delim;
			return $data;
		}
	}

	die "Couldn't find include file $filename\n";
}

####################################################
# parse out the enum declarations
sub parse_enum_elements($$)
{
	my($name) = shift;
	my($elements) = shift;
	
	print "Parsing enum $name\n";

	print CFILE "static const struct enum_struct einfo_" . $name . "[] = {\n";

	my(@enums) = split(/,/s, $elements);
	for (my($i)=0; $i <= $#{@enums}; $i++) {
		my($enum) = $enums[$i];
		if ($enum =~ /\s*(\w*)/) {
			my($e) = $1;
			print CFILE "{\"$e\", $e},\n";
		}
	}

	print CFILE "{NULL, 0}};\n";

	print CFILE "
int gen_dump_enum_$name(struct parse_string *p, const char *ptr, unsigned indent) {
	return gen_dump_enum(einfo_$name, p, ptr, indent);
}

int gen_parse_enum_$name(char *ptr, const char *str) {
	return gen_parse_enum(einfo_$name, ptr, str);
}

";
}

####################################################
# parse out the enum declarations
sub parse_enums($)
{
	my($data) = shift;

	while ($data =~ /\nGENSTRUCT enum (\w*?) {(.*?)};(.*)/s) {
		my($name) = $1;
		my($elements) = $2;
		$data = $3;
		parse_enum_elements($name, $elements);
	}
}


####################################################
# parse a header file, generating a dumper structure
sub parse_data($)
{
	my($data) = shift;

	# strip comments
	$data =~ s/\/\*.*?\*\// /sg;
	# collapse spaces 
	$data =~ s/[\t ]+/ /sg;
	$data =~ s/\s*\n\s+/\n/sg;

	parse_enums($data);

	# parse into structures 
	while ($data =~ /\nGENSTRUCT struct (\w*?) {\n(.*?)};(.*)/s) {
		my($name) = $1;
		my($elements) = $2;
		$data = $3;
		parse_elements($name, $elements);
	}
}

####################################################
# parse a header file, generating a dumper structure
sub parse_includes($)
{
	my($data) = shift;

	# handle includes
	while (defined($data) && $data =~ /(.*?)^\#\s*include\s*\"([\w.]*?)\"(.*)/ms) {
		my($p1) = $1;
		my($p2) = $2;
		my($p3) = $3;
		parse_data($p1);

		if (!defined($already_included{$p2})) {
			$already_included{$p2} = 1;
			parse_header($p2);
		}

		$data = $p3;
	}
	if (defined($data)) {
		parse_data($data);
	}
}

####################################################
# parse a header file, generating a dumper structure
sub parse_header($)
{
	my($header) = shift;
	my($data) = IncludeLoad($header);
	parse_includes($data);
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
	    'include=s' => \$opt_include
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

print CFILE "/* This is an automatically generated file - DO NOT EDIT! */\n\n";

parse_header($opt_header);
