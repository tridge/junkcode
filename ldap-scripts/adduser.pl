#!/usr/bin/perl -w

use English;
use strict;
use Net::LDAP;
use Data::Dumper;
use Getopt::Long;

my $basedn = "dc=blu,dc=home,dc=samba,dc=org";

my $firstid = 10000;

my $opt_help = 0;
my $opt_uid = -1;
my $opt_userid;
my $opt_gid = 5000;
my $opt_surname = "Surname";
my $opt_given = "Firstname ";
my $opt_email;
my $opt_homedir = "/dev/null";
my $opt_loginshell = "/bin/false";
my $opt_domain = "BLUDOM";

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

########################
# show some help
sub usage() {
	printf "
Usage: adduser.pl [options]

   --userid      USERNAME
   --uid         UID
   --gid         GROUPID
   --surname     SURNAME
   --given       GIVEN_NAME
   --email       EMAIL_ADDRESS
   --homedir     HOME_DIRECTORY
   --loginshell  LOGIN_SHELL
   --domain      DOMAIN
";
}

GetOptions (
	    'help|h|?' => \$opt_help, 
	    'userid=s' => \$opt_userid, 
	    'gid=i' => \$opt_gid, 
	    'uid=i' => \$opt_uid, 
	    'surname=s' => \$opt_surname,
	    'given=s' => \$opt_given,
	    'email=s' => \$opt_email,
	    'homedir=s' => \$opt_homedir,
	    'loginshell=s' => \$opt_loginshell,
	    'domain=s' => \$opt_domain
	    );

if ($opt_help) {
	usage();
	exit(1);
}

if (! defined $opt_userid) {
	printf "You must specify a userid\n\n";
	usage();
	exit(1);
}

if (! defined $opt_email) {
	$opt_email = "$opt_userid\@home.samba.org";
}

my $dn= "uid=$opt_userid,ou=User,$basedn";

my $ldap=Net::LDAP->new('/var/lib/ldapi', 'unix_socket' => 1 );
$ldap->bind() || die "bind failed!";

if ($opt_uid == -1) {
	$opt_uid = find_next_uid($ldap);
}

my $result = $ldap->add ( $dn,
			  attr => [ 'uid' => $opt_userid,
				    'cn'   => "$opt_given $opt_surname",
				    'sn'    => $opt_surname,
				    'givenname'    => $opt_given,
				    'displayName' => "$opt_given $opt_surname",
				    'gecos' => "$opt_given $opt_surname",
				    'mail' => $opt_email,
				    'uidNumber' => $opt_uid,
				    'gidNumber' => $opt_gid,
				    'homeDirectory' => $opt_homedir,
				    'loginshell' => $opt_loginshell,
				    'rid' => (($opt_uid*2)+1000),
				    'domain' => $opt_domain,
				    'acctFlags' => '[U          ]',
				    'objectclass' => ['top', 'person',
						      'organizationalPerson',
						      'inetOrgPerson',
						      'posixAccount', 
						      'sambaAccount'],
				    ]
			  );

if ($result->code != 0) {
	die "could not add user '$opt_userid' : " . $result->error . "\n";
}

printf "Added user '%s' with uid %u\n", $opt_userid, $opt_uid;
exit(0);
