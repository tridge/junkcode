#!/usr/bin/perl -Tw 

use strict;
use Net::LDAP;
use Crypt::SmbHash;

my $basedn = "dc=blu,dc=home,dc=samba,dc=org";

if ($#ARGV != 1) {
	printf "Usage: setpass.pl <userid> <password>\n";
	exit(1);
}

my $userid = $ARGV[0]; 
my $password = $ARGV[1]; 

my $ldap=Net::LDAP->new('/var/lib/ldapi', 'unix_socket' => 1 );
$ldap->bind() || die "bind failed!";

my $filter = "(uid=$userid)";

my $mesg = $ldap->search(
                       base => $basedn,
                       filter => $filter,
                       scope => "sub",
                       attrs => [ 'dn', 'cn' ]
                       );
if ($mesg->count != 1) { die "Expected exactly 1 match\n";}

my $entry = $mesg->entry(0);
my ($dn) = $entry->dn;

my $name = $entry->get_value('cn');

my $lmhash;
my $nthash;
ntlmgen $password,$lmhash,$nthash;

$mesg = $ldap->modify( $dn,
                        replace => {
                            'pwdMustChange' => 0, #next logon
#                            'pwdMustChange' => time() + 60*60*24*100,
                            'pwdCanChange' => 0,
                            'lmPassword' => $lmhash,
                            'ntPassword' => $nthash,
                            'pwdLastSet'  => time(),
                            }
                        );

$mesg->code && die $mesg->error;

printf "Set password for user '%s'\n", $userid;
