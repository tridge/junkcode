#!/usr/bin/perl -Tw 

use strict;
use Net::LDAP;

my $basedn = "dc=blu,dc=home,dc=samba,dc=org";

if ($#ARGV != 1) {
	printf "Usage: addgroupmem.pl <groupname> <member>\n";
	exit(1);
}

my $groupid = $ARGV[0]; 
my $member = $ARGV[1]; 

my $ldap=Net::LDAP->new('/var/lib/ldapi', 'unix_socket' => 1 );
$ldap->bind() || die "bind failed!";

my $filter = "(&(objectClass=posixGroup)(cn=$groupid))";

my $mesg = $ldap->search(
                       base => $basedn,
                       filter => $filter,
                       scope => "sub",
                       attrs => [ 'dn', 'cn' ]
                       );
if ($mesg->count != 1) { 
	die "Expected exactly 1 group match - got " . $mesg->count . "\n";
}

my $entry = $mesg->entry(0);

$mesg = $ldap->modify($entry->dn,
		    add => {'memberUid' => $member}
		    );

$mesg->code && die $mesg->error;

printf "Added '%s' to group '%s'\n", $member, $groupid;
