#!/usr/bin/perl -w

use strict;
use Net::LDAP;

my $basedn = "dc=blu,dc=home,dc=samba,dc=org";
my $firstid = 5000;

if ($#ARGV != 0) {
	printf "Usage: addgroup.pl <groupname>\n";
	exit(1);
}

my $groupid = $ARGV[0];

#########################
# find the next reasonable gid value 
sub find_next_gid($) {
	my $ldap = shift;

	my $mesg = $ldap->search(
				 base => $basedn,
				 filter => "(objectClass=posixGroup)",
				 scope => "sub",
				 attrs => [ 'gidNumber' ]
				 );
	my $maxid = 0;
	for (my $i=0; $i < $mesg->count; $i++) {
		my $thisid = $mesg->entry($i)->get_value('gidNumber');
		if ($thisid > $maxid) {
			$maxid = $thisid;
		}
	}
	if ($maxid == 0) {
		return $firstid;
	}
	return $maxid + 1;
}

my $ldap=Net::LDAP->new('/var/lib/ldapi', 'unix_socket' => 1 );
$ldap->bind() || die "bind failed!";

my $gid = find_next_gid($ldap);

my $dn= "uid=$groupid,ou=Group,$basedn";

my $result = $ldap->add ( $dn,
			  attr => [ 'cn' => $groupid,
				    'gidNumber' => $gid,
				    'objectclass' => ['top', 'posixGroup'],
				    ]
			  );

$result->code == 0 or die "could not add group '$groupid' : " . $result->error . "\n";

# add add it to the group mapping database, allocating a RID
`smbgroupedit -a "$groupid" -t d -u "$groupid"`;

printf "Added group '%s' with gid %u\n", $groupid, $gid;

