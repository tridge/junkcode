#!/usr/bin/perl -w
# convert a solid dump to a postgres dump
# tridge@samba.org

use strict;
use Getopt::Long;
use Data::Dumper;

my ($opt_output) = "";
my ($opt_help) = 0;


# Maps into postgres types
my(%TypeMap) = 
    (
     "varchar(254)" => "character varying",
     "integer" => "integer",
     "date" => "date",
     "decimal(16,2)" => "real",
     "timestamp" => "timestamp"
     );

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

########################################################################
# remove double quotes from a string
sub NoQuotes($)
{
	my $n = shift;
	if ($n =~ /^\"(\S+)\"$/) {
		return $1;
	}
	return $n;
}

########################################################################
# change a table name of form "DAVID"."FOO" into foo
sub CvtTable($)
{
	my $n = shift;
	if ($n =~ /^\"(\w+)\"\.\"(\w+)\"$/) {
		return lc $2;
	}
	return lc NoQuotes($n);
}


########################################################################
# fix up a data field on load
sub FixData($)
{
	my $v = shift;
	$v =~ s/[\r]/\\r/sg;
	$v =~ s/[\n]//sg;
	$v =~ s/[\t]/\\t/sg;
	# fix some bad date fields
	if ($v =~ /1(19\d\d-\d\d-0)/) {
		print "Fixed bad date $v as 1900-01-01\n";
		$v = "1900-01-01";
	}
	if ($v eq "NULL") {
		$v = "\\N";
	}
	if ($v eq "") {
		$v = "\\N";
	}
	return $v;
}



########################################################################
# process a 'LOAD DATA' via COPY FROM
sub LoadData()
{
	my $term = ",";
	my $table = "";
	my $fname = "";
	my $cwd = `pwd`;
	my $fields = "";

	chomp($cwd);

	while (<INPUT>) {
		my $line = $_;
		if ($line =~ /^\)$/) {
			last;
		}
		if ($line =~ /^INFILE '(.*)'/) {
			$fname = $1;
			next;
		}
		if ($line =~ /^INTO TABLE (\S+)/) {
			$table = CvtTable($1);
			next;
		}
		if ($line =~ /^FIELDS TERMINATED BY '(.*)'$/) {
			$term = $1;
			next;
		}
		if ($line =~ /^\t(\S+) /) {
			if ($fields) {
				$fields .= ", " . lc NoQuotes($1);
			} else {
				$fields .= lc NoQuotes($1);
			}
		}
	}
	print "Load data into $table from $fname\n";

	my $data = FileLoad($fname);

	my @lines = split /[^\r]$/ms, $data;

	my $count = $#lines + 1;

	$| = 1;

	print OUTPUT "COPY $table FROM stdin;\n";

	for (my $i=0; $i <= $#lines; $i++) {
		my $v;
		my $l = $lines[$i];
		if (! ($l =~ /\'/)) { next; }
		while ($l) {
			if ($l =~ /^'(.*?)',(.*)$/ms) {
				$l = $2;
				$v = FixData($1);
				print OUTPUT "$v\t";
			} elsif ($l =~ /^(.*?),(.*)$/ms) {
				$l = $2;
				$v = FixData($1);
				print OUTPUT "$v\t";
			} elsif ($l =~ /\'(.*)/) {
				$v = FixData($1);
				print OUTPUT "$v\n";
				last;
			} elsif ($l =~ /^NUL$/) {
				$v = FixData("NULL");
				print OUTPUT "$v\n";
				last;
			}
		}
		if ($i % 10 eq 0) {
			print "$i/$count\r";
		}
	}
	print OUTPUT "\\.\n\n";

	print "$count/$count\n";
}


########################################################################
# process a 'CREATE INDEX'
sub CreateIndex($)
{
	my ($line) = shift;

	if ($line =~ /(\S+) ON (\S+) \((\S+)\)/) {
		my $index = CvtTable($1);
		my $table = CvtTable($2);
		my @fields = split(',', $3);

		print "Create index $index\n";
		print OUTPUT "CREATE INDEX $index ON $table (";
		for (my $i=0; $i <= $#fields; $i++) {
			my $f = lc NoQuotes($fields[$i]);
			if ($i != 0) {
				print OUTPUT ",";
			}
			print OUTPUT "$f";
		}
		print OUTPUT ");\n\n";
	}
	
}

########################################################################
# process a 'CREATE VIEW'
sub CreateView($)
{
	my ($line) = shift;

	if ($line =~ /(\w+) AS SELECT (.*);$/) {
		my $view = lc $1;
		my $query = lc $2;

		print "Create view $view\n";
		print OUTPUT "CREATE VIEW $view AS SELECT $query;\n\n";
	}
	
}

########################################################################
# process a 'CREATE TABLE'
sub CreateTable($)
{
	my $table_name = $1;
	my(@fields);
	my $unique = "";

	$table_name = CvtTable($table_name);

	print "Creating table $table_name\n";

	my $nfields = 0;

	while (<INPUT>) {
		my($not_null) = 0;
		my($name) = "";
		my($type) = "";

		chomp $_;

		if ($_ =~ /^\);/) { last; }
		if ($_ =~ /^$/) { last; }

		if ($_ =~ /UNIQUE (.*)$/) { 
			$unique = lc $1;
			$unique =~ s/\"//sg;
			next;
		}

		my($line) = $_;
		
		if ($line =~ /(.*?),$/) {
			$line = $1;
		}

		if ($line =~ / NOT NULL/) {
			$not_null = 1;
		}

		if ($line =~ /(\S+) (\S+)/) {
			$name = lc NoQuotes($1);
			$type = lc $2;
			if ($type =~ /(.*)\);$/) {
				$type = $1;
			}
		}

		if (! $TypeMap{$type}) {
			die "Unknown type $type for field $name in table $table_name\n";
		}
		
		$type = $TypeMap{$type};

		$fields[$nfields]->{'type'} = $type;
		$fields[$nfields]->{'name'} = $name;
		$nfields++;
	}

	print OUTPUT "CREATE TABLE $table_name (\n";
	for (my $i=0; $i < $nfields; $i++) {
		if ($i gt 0) {
			print OUTPUT ",\n";
		}
		print OUTPUT "\t$fields[$i]->{'name'} $fields[$i]->{'type'}";
	}

	if ($unique) {
		print OUTPUT ",\n\tUNIQUE $unique";
	}

	print OUTPUT "\n);\n\n";
}


########################################################################
# process one file
sub process_file($)
{
	my($filename) = shift;
	
	print "Processing $filename\n";

	open(INPUT,"<$filename") || die "Can't open $filename";

	while (<INPUT>) {
		if ($_ =~ /^CREATE TABLE (\S+) \(/) {
			CreateTable($1);
			next;
		}
		if ($_ =~ /^CREATE INDEX (.*)/) {
			CreateIndex($1);
			next;
		}
		if ($_ =~ /^CREATE VIEW (.*)/) {
			CreateView($1);
			next;
		}
		if ($_ =~ /^LOAD DATA/) {
			LoadData();
			next;
		}
	};

	close(INPUT);
}

#########################################
# display help text
sub ShowHelp()
{
    print "
           solid to postgres converter
           Copyright tridge\@samba.org 2002

           Usage: solid_convert.pl [options] <files>

           Options:
             --help                this help page
             --output FILE         file to output to
           \n";
    exit(0);
}


##############
# main program

GetOptions (
	    'help|h|?' => \$opt_help, 
	    'output=s' => \$opt_output,
	    );

if ($opt_help) {
    ShowHelp();
    exit(0);
}

die "ERROR: You must specify an output file with --output" unless ($opt_output);

open(OUTPUT,">$opt_output") || die "ERROR: Failed to open $opt_output";

for (my $i=0; $i <= $#ARGV; $i++) {
	process_file($ARGV[$i]);
}
