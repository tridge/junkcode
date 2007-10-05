#!/usr/bin/perl -w

use Data::Dumper;

my %ltype;

sub process_file($) {
	my $fname = shift;

	open(HANDLE, $fname) || die "Can't open $fname";
	my $data = "";
	read(HANDLE, $data, 4096);
	$data =~ tr/A-Za-z0-9/ /cs;
	
	if ($data =~ /GNU General Public/ ||
	    $data =~ /GNU Public License/) {
		if ($data =~ /Everyone is permitted to copy and distribute verbatim copies of this license document but changing it is not allowed/) {
			$ltype{"COPYING"}++;
		} elsif ($data =~ /License version 3/) {
			$ltype{"GPLV3"}++;
		} elsif ($data =~ /any later version/ ||
		    $data =~ /v2 or later/ ||
		    $data =~ /version 2 or later/) {
			$ltype{"GPLV2+"}++;
		} elsif ($data =~ /version 2/i) {
			$ltype{"GPLV2"}++;
			printf("%s\n", $fname);
		} else {
			$ltype{"GPLvX"}++;			
		}
	} elsif ($data =~ /copyright/i ||
		 $data =~ /license/i) {
		$ltype{"OTHER"}++;
	} else {
		$ltype{"NONE"}++;
	}
	close(HANDLE);
}

sub traverse($) {
	my $dname = shift;
	my $d;
	if (-f $dname) {
		process_file($dname);
		return;
	}
	opendir(DIR, $dname) || die "Can't open directory $dname";
	my @names = readdir(DIR);
	foreach my $f (@names) {
		next if ($f eq ".") || ($f eq "..");
		my $fname = $dname . "/" . $f;
		if (-d $fname) {
			traverse($fname);
		} elsif (-f _) {
			process_file($fname);
		}
	}
	closedir(DIR);
}


for (my $i=0;$i<=$#ARGV;$i++) {
	traverse($ARGV[$i]);
}

foreach my $l (keys %ltype) {
	printf("%-6s %u\n", $l, $ltype{$l});
}

