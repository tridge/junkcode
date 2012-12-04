#!/usr/bin/perl -w

my $sub_src = shift;
my $sub_dst = shift;

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
    if (!defined($data)) {
	    return "";
    }
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

while (my $fname = shift) {
	my $src = FileLoad($fname);
	my $replace = 0;
	if (!defined($src)) {
		print("Unable to open $fname\n");
		next;
	}
	
	while ((my $idx = index($src, $sub_src)) != -1) {
		substr $src, $idx, length($sub_src), $sub_dst;
		$replace = 1;
	}
	if ($replace) {
		print "Replacing $sub_src with $sub_dst in $fname\n";
		FileSave($fname, $src);
	}
}
