#!/usr/bin/perl -w

use strict;
use Net::LDAP;

my $basedn = "dc=blu,dc=home,dc=samba,dc=org";
my $firstid = 10000;

if ($#ARGV != 0) {
	printf "Usage: addmachine.pl <machineacct>\n";
	exit(1);
}

#########################
# find the next reasonable uid value 
sub find_next_uid($) {
	my $ldap = shift;

	my $mesg = $ldap->search(
				 base => $basedn,
				 filter => "(objectClass=posixAccount)",
				 scope => "sub",
				 attrs => [ 'uidNumber' ]
				 );
	my $maxid = 0;
	for (my $i=0; $i < $mesg->count; $i++) {
		my $thisid = $mesg->entry($i)->get_value('uidNumber');
		if ($thisid > $maxid) {
			$maxid = $thisid;
		}
	}
	if ($maxid == 0) {
		return $firstid;
	}
	return $maxid + 1;
}

my $userid = $ARGV[0];
my $gid = 5002;
my $surname = "NONE";
my $given = "NONE";
my $email = "NONE";
my $enumber = $userid;
my $homedir = "/dev/null";
my $loginshell = "/bin/false";
my $domain = "BLUDOM";
my $dn= "uid=$userid,ou=Machine,$basedn";

my $ldap=Net::LDAP->new('/var/lib/ldapi', 'unix_socket' => 1 );
$ldap->bind() || die "bind failed!";

my $uid = find_next_uid($ldap);

my $result = $ldap->add ( $dn,
			  attr => [ 'uid' => $userid,
				    'cn'   => "$given $surname",
				    'sn'    => $surname,
				    'givenname'    => $given,
				    'displayName' => "$given $surname",
				    'gecos' => "$given $surname",
				    'mail' => $email,
				    'employeeNumber' => $enumber,
				    'uidNumber' => $uid,
				    'gidNumber' => $gid,
				    'homeDirectory' => $homedir,
				    'loginshell' => $loginshell,
				    'objectclass' => ['top', 'person',
						      'organizationalPerson',
						      'inetOrgPerson',
						      'posixAccount'],
				    ]
			  );

$result->code == 0 or die "could not add machine! ".$result->error;

exit(0);
