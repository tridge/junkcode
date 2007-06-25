#!/usr/bin/perl -w

sub x_push() {
	my @deadhosts = ();

	push(@deadhosts, "foo");
	push(@deadhosts, "bar");

	for my $host (@deadhosts) {
		print "$host\n";
	}
}

sub dump_args() {
	printf("%d arguments\n", $#ARGV+1);
	for (my $i=0; $i <= $#ARGV; $i++) {
		printf("%s\n", $ARGV[$i]);
	}
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


#dump_args();
printf("%s\n", dhm_time($ARGV[0]));
