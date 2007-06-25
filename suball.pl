#!/usr/bin/perl -w

my $sub_src = shift;
my $sub_dst = shift;

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
	my $src = `cat $fname`;
	my $replace = 0;
	
	while ((my $idx = index($src, $sub_src)) != -1) {
		substr $src, $idx, length($sub_src), $sub_dst;
		$replace = 1;
	}
	if ($replace) {
		print "Replacing $sub_src with $sub_dst in $fname\n";
		FileSave($fname, $src);
	}
}
