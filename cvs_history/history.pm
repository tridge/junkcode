# Copyright (C) Andrew Tridgell <tridge@samba.org>     2001
# Copyright (C) Martin Pool <mbp@samba.org>            2003
# script to show recent checkins in cvs

package history;

my $BASEDIR = "/home/build/master";
my $TIMEZONE = "PST";
my $TIMEOFFSET = 0;

use strict qw{vars};
use util;
use POSIX;
use Data::Dumper;
use CGI::Base;
use CGI::Form;
use File::stat;

my $req = new CGI::Form;

my $HEADCOLOR = "#a0a0e0";
my $CVSWEB_BASE = "https://home.tridgell.net/cgi-bin/cvsweb";
my (%tree_base) = ('samba' => "samba");
my $unpacked_dir = "/home/tridge/cvs_unpacked";
my $myself = "https://home.tridgell.net/cgi-bin/history";
my $basedir = "/home/tridge/cvs_history";

################################################
# start CGI headers
sub cgi_headers() {
    print "Content-type: text/html\r\n";

    util::cgi_gzip();

    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd"> 
<html>
<head><title>recent checkins</title></head>
<body bgcolor="white" text="#000000" link="#0000EE" vlink="#551A8B" alink="#FF0000">
';

}

################################################
# end CGI
sub cgi_footers() {
    print "</body>";
    print "</html>\n";
}

################################################
# print an error on fatal errors
sub fatal($) {
    my $msg=shift;
    print "ERROR: $msg<br>\n";
    cgi_footers();
    exit(0);
}


###############################################
# pretty up a cvs diff -u
sub cvs_pretty($)
{
    my $diff = shift;
    my $ret = "";
    my @lines = split(/$/m, $diff);

    my (%colors) = (
		    '^diff.*' => 'red',
		    '^=.*' => 'blue',
		    '^Index:.*' => 'blue',
		    '^\-.*' => '#a00000',
		    '^\+.*' => '#00a000'
		    );

    for (my $i=0; $i <= $#lines; $i++) {
	my $line = $lines[$i];

	for my $r (keys %colors) {
	    if ($line =~ /$r/m) {
		$line = "<font color=\"$colors{$r}\">$line</font>";
		last;
	    }
	}
	$ret .= $line;
    }
    return $ret;
}

sub cvsweb_paths($$)
{
    my $tree = shift;
    my $paths = shift;
    my $ret = "";
    while ($paths =~ /\s*([^\s]+)(.*)/) {
	$ret .= "<a href=\"$CVSWEB_BASE/$tree_base{$tree}/$1\">$1</a> ";
	$paths = $2;
    }
    
    return $ret;
}

#############################################
# show one row of history table
sub history_row($$)
{
    my $entry = shift;
    my $tree = shift;
    my $msg = util::cgi_escape($entry->{MESSAGE});
    my $t = POSIX::asctime(POSIX::gmtime($entry->{DATE}));
    my $age = util::dhm_time(time()-$entry->{DATE});

    $t =~ s/\ /&nbsp;/g;

    print "
<p><table bgcolor=\"\#f0f0ff\" width=\"80%\" class=\"entry\">
  <tr>
    <td>
     <b>$t</b><br>$age ago<br>
    </td>
  </tr>
  <tr>
    <td valign=top align=left>
<b><a href=$myself?function=diff&tree=$tree&date=$entry->{DATE}&author=$entry->{AUTHOR}>show diffs</a></b><br>
<a href=$myself?function=text_diff&tree=$tree&date=$entry->{DATE}&author=$entry->{AUTHOR}>download diffs</a><br>
    <pre>$msg</pre>
    </td>
  </tr>

  <tr><td><em>Author: </em>$entry->{AUTHOR}</td></tr>";

    if ($entry->{FILES}) {
	print "<tr><td><em>Modified: </em>";
	print cvsweb_paths($tree, $entry->{FILES});
	print "</td></tr>\n";
    }

    if ($entry->{ADDED}) {
	print "<tr><td><em>Added:</em> ";
	print cvsweb_paths($tree, $entry->{ADDED});
	print "</td></tr>\n";
    }

    if ($entry->{REMOVED}) {
	print "<tr><td><em>Removed: </em>";
	print cvsweb_paths($tree, $entry->{REMOVED});
	print "</td></tr>\n";
    }

    print "</table>\n";
}


#############################################
# show one row of history table
sub history_row_text($$)
{
    my $entry = shift;
    my $tree = shift;
    my $msg = util::cgi_escape($entry->{MESSAGE});
    my $t = POSIX::asctime(POSIX::gmtime($entry->{DATE}));
    my $age = util::dhm_time(time()-$entry->{DATE});

    print "Author: $entry->{AUTHOR}\n";
    print "Mofified: $entry->{FILES}\n";
    print "Added: $entry->{ADDED}\n";
    print "Removed: $entry->{REMOVED}\n";
    print "\n\n$msg\n\n\n";
}


###############################################
# show recent cvs entries
sub cvs_diff($$$$)
{
    my $author = shift;
    my $date = shift;
    my $tree = shift;
    my $text_html = shift;
    my $cmd;
    my $module;

    util::InArray($tree, [keys %tree_base]) || fatal("unknown tree");

    my $log = util::LoadStructure("$basedir/history.$tree");

    chdir("$unpacked_dir/$tree") || fatal("no tree $unpacked_dir/$tree available");

    $module = $tree_base{$tree};

    for (my $i=0; $i <= $#{$log}; $i++) {
	my $entry = $log->[$i];
	if ($author eq $entry->{AUTHOR} &&
	    $date == $entry->{DATE}) {
	    my $t1;
	    my $t2;

	    chomp($t1 = POSIX::ctime($date-60+($TIMEOFFSET*60*60)));
	    chomp($t2 = POSIX::ctime($date+60+($TIMEOFFSET*60*60)));

	    if ($text_html eq "html") {
		print "<h2>CVS Diff in $tree for $t1</h2>\n";
		
		print "<table border=0><tr>\n";
		history_row($entry, $tree);
		print "</tr></TABLE>\n";
	    } else {
		history_row_text($entry, $tree);
	    }

	    if (! ($entry->{TAG} eq "") && !$entry->{REVISIONS}) {
		print '
<br>
<b>sorry, cvs diff on branches not currently possible due to a limitation 
in cvs</b>
<br>';
	    }

	    $ENV{'CVS_PASSFILE'} = "$BASEDIR/.cvspass";

	    if ($entry->{REVISIONS}) {
		    for my $f (keys %{$entry->{REVISIONS}}) {
			my $cmd;
			my $diff;
			my $fix_diff = 0;
			if ($entry->{REVISIONS}->{$f}->{REV1} eq "NONE") {
			    $cmd = "cvs rdiff -u -r 0 -r $entry->{REVISIONS}->{$f}->{REV2} $module/$f";
			    $fix_diff = 1;
			} elsif ($entry->{REVISIONS}->{$f}->{REV2} eq "NONE") {
			    $cmd = "cvs rdiff -u -r $entry->{REVISIONS}->{$f}->{REV1} -r 0 $module/$f";
			    $fix_diff = 1;
			} elsif ($text_html eq "html") {
			    $cmd = "cvs diff -b -u -r $entry->{REVISIONS}->{$f}->{REV1} -r $entry->{REVISIONS}->{$f}->{REV2} $f";
			} else {
			    $cmd = "cvs diff -u -r $entry->{REVISIONS}->{$f}->{REV1} -r $entry->{REVISIONS}->{$f}->{REV2} $f";
			}

			$diff = `$cmd 2> /dev/null`;
			if ($fix_diff) {
			    $diff =~ s/^--- $module\//--- /mg;
			    $diff =~ s/^\+\+\+ $module\//\+\+\+ /mg;
			}
			
			if ($text_html eq "html") { 
			    print "<!-- $cmd --!>\n";
			    $diff = util::cgi_escape($diff);
			    $diff = cvs_pretty($diff);
			    print "<pre>$diff</pre>\n";
			} else {
			    print "$diff\n";
			}
		    }
	    } else {
		    if ($text_html eq "html") { 
			my $cmd = "cvs diff -b -u -D \"$t1 $TIMEZONE\" -D \"$t2 $TIMEZONE\" $entry->{FILES}";
		    } else {
			my $cmd = "cvs diff -u -D \"$t1 $TIMEZONE\" -D \"$t2 $TIMEZONE\" $entry->{FILES}";
		    }

		    my $diff = `$cmd 2> /dev/null`;

		    if ($text_html eq "html") { 
			print "<!-- $cmd --!>\n";
			$diff = util::cgi_escape($diff);
			$diff = cvs_pretty($diff);
		    }
		    
		    print "<pre>$diff</pre>\n";
	    }

	    return;
	}
    }    
}


###############################################
# show recent cvs entries
sub cvs_history($)
{
    my $tree = shift;
    my (%authors) = ('ALL' => 1);
    my $author;

    util::InArray($tree, [keys %tree_base]) || fatal("unknown tree");

    my $log = util::LoadStructure("$basedir/history.$tree");

    for (my $i=$#{$log}; $i >= 0; $i--) {
	$authors{$log->[$i]->{AUTHOR}} = 1;
    }

    print $req->startform("GET");
    print "Select Author: ";
    print $req->popup_menu("author", [sort keys %authors]);
    print $req->submit('sub_function', 'Refresh');
    print $req->hidden('tree', $tree);
    print $req->hidden('function', 'Recent Checkins');
    print $req->endform();

    print "<h2>Recent checkins for $tree</h2>\n";
    
    print "
";

    $author = $req->param("author");

    for (my $i=$#{$log}; $i >= 0; $i--) {
	my $entry = $log->[$i];
	if (! $author ||
	    ($author eq "ALL") || 
	    ($author eq $entry->{AUTHOR})) {
	    history_row($entry, $tree);
	}
    }
    print '
';
}


#cvs_diff($req->param('author'), $req->param('date'), $req->param('tree'));
#cvs_history("trinity");


1;
