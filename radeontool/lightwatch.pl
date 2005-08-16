#!/usr/bin/perl -w
use strict;

#
#  This handy script watches when the screensaver activates and
#  toggles the LCD backlight.  You won't see more than a 
#  second of the screensaver, so you might as well chose one
#  which consumes few MIPS/battery.
#
#  You will probably want to make this script owned by root
#  and SUID.  Also you will need the perl-suidperl package installed
#  for RedHat systems.
#     chown root lightwatch
#     chmod u+x lightwatch
#

delete @ENV{'IFS', 'CDPATH', 'ENV', 'BASH_ENV'};
$ENV{'PATH'} = '/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin:/usr/local/sbin';

$<=0;  # become root not just effective root

open(XS,"/usr/X11R6/bin/xscreensaver-command -watch|") or die;
while(<XS>) {
   if(/^BLANK/i) {
      system("radeontool light off");
   } elsif(/^UNBLANK/i) {
      system("radeontool light on");
   }
}


