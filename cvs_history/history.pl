#!/usr/bin/perl -w
# Copyright (C) Andrew Tridgell <tridge@samba.org>     2001
# script to show recent checkins in cvs

my $BASEDIR = "/home/tridge/history";
my $TIMEZONE = "PST";
my $TIMEOFFSET = 0;

use strict qw{vars};
use lib "/home/tridge/history";
use util;
use POSIX;
use Data::Dumper;
use CGI::Base;
use CGI::Form;
use File::stat;

my $req = new CGI::Form;

my $HEADCOLOR = "#a0a0e0";
my $CVSWEB_BASE = "/cgi-bin/cvsweb.cgi/nas/linux/trinity";

my (%trees) = ('trinity' => "");

my $unpacked_dir = "/home/tridge/clipper";

###############################################
# work out a URL so I can refer to myself in links
my $myself = $req->self_url;
if ($myself =~ /(.*)[?].*/) {
    $myself = $1;
}

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

sub plural_write ($$)
{
    my ($num,$text) = @_;
    if ($num != 1) {
        $text = $text . "s";
    }
    if ($num > 0) {
        return $num . " " . $text;
    }
    else {
        return "";
    }
}

sub readableTime ($$)
{
    my ($i, $break, $retval);
    my ($secs,$long) = @_;

    # this function works correct for time >= 2 seconds
    if ($secs < 2) {
        return "very little time";
    }

    my %desc = (1 , 'second',
                   60, 'minute',
                   3600, 'hour',
                   86400, 'day',
                   604800, 'week',
                   2628000, 'month',
                   31536000, 'year');
    my @breaks = sort {$a <=> $b} keys %desc;
    $i = 0;
    while ($i <= $#breaks && $secs >= 2 * $breaks[$i]) { 
        $i++;
    }
    $i--;
    $break = $breaks[$i];
    $retval = plural_write(int ($secs / $break), $desc{"$break"});

    if ($long == 1 && $i > 0) {
        my $rest = $secs % $break;
        $i--;
        $break = $breaks[$i];
        my $resttime = plural_write(int ($rest / $break), 
                                $desc{"$break"});
        if ($resttime) {
            $retval = $retval . ", " . $resttime;
        }
    }

    return $retval;
}

#End raided code

sub cvsweb_paths($)
{
    my $paths = shift;
    my $ret = "";
    while ($paths =~ /\s*([^\s]+)(.*)/) {
	$ret .= "<a href=\"$CVSWEB_BASE/$1\">$1</a> ";
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
    my $readabletime = readableTime(time()-$entry->{DATE},1);
    print "
<table border=3>
<tr><td>
<b>$t UTC</b><br>
$readabletime ago<br>
<b><a href=$myself?function=diff&tree=$tree&date=$entry->{DATE}&author=$entry->{AUTHOR}>show diffs</a></b><br>
</td>
<td>$msg</td>
</tr>
</table>
<table>
<tr><td><b>Author:</b></td><td>$entry->{AUTHOR}</td></tr>
<tr valign=top><td><b>Modified:</b></td><td>";
print cvsweb_paths($entry->{FILES});
print "</td></tr>
<tr valign=top><td><b>Added:</b></td><td>";
print cvsweb_paths($entry->{ADDED});
print "</td></tr>
<tr valign=top><td><b>Removed:</b></td><td>";
print cvsweb_paths($entry->{REMOVED});
print "</td></tr>
</table>
\n";
}


###############################################
# show recent cvs entries
sub cvs_diff($$$)
{
    my $author = shift;
    my $date = shift;
    my $tree = shift;
    my $cmd;

    util::InArray($tree, [keys %trees]) || fatal("unknown tree");

    my $log = util::LoadStructure("history.$tree");

    chdir("$unpacked_dir/$tree") || fatal("no tree $unpacked_dir/$tree available");

    for (my $i=0; $i <= $#{$log}; $i++) {
	my $entry = $log->[$i];
	if ($author eq $entry->{AUTHOR} &&
	    $date == $entry->{DATE}) {
	    my $t1;
	    my $t2;

	    chomp($t1 = POSIX::ctime($date-60+($TIMEOFFSET*60*60)));
	    chomp($t2 = POSIX::ctime($date+60+($TIMEOFFSET*60*60)));

	    print "<h2>CVS Diff in $tree for $t1</h2>\n";

	    print "<table border=0><tr>\n";
	    history_row($entry, $tree);
	    print "</tr></TABLE>\n";

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
			    my $cmd = "cvs diff -u -r $entry->{REVISIONS}->{$f}->{REV1} -r $entry->{REVISIONS}->{$f}->{REV2} $f";
			    print "<!-- $cmd --!>\n";
			    my $diff = `$cmd 2> /dev/null`;

			    $diff = util::cgi_escape($diff);
			    $diff = cvs_pretty($diff);
			    
			    print "<pre>$diff</pre>\n";
		    }
	    } else {
		    my $cmd = "cvs diff -u -D \"$t1 $TIMEZONE\" -D \"$t2 $TIMEZONE\" $entry->{FILES}";
		    print "<!-- $cmd --!>\n";
		    my $diff = `$cmd 2> /dev/null`;

		    $diff = util::cgi_escape($diff);
		    $diff = cvs_pretty($diff);
		    
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
    my (%authors) = ('ALL');
    my $author;

    util::InArray($tree, [keys %trees]) || fatal("unknown tree");

    my $log = util::LoadStructure("history.$tree");

    for (my $i=$#{$log}; $i >= 0; $i--) {
	$authors{$log->[$i]->{AUTHOR}} = 1;
    }

    print $req->startform("GET", $myself);
    print "Select Author: ";
    print $req->popup_menu("author", [sort keys %authors]);
    print $req->submit('function', 'Go');
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


###############################################
# main program
cgi_headers();

chdir("$BASEDIR") || fatal("can't change to data directory");

if (defined $req->param("function")) {
    my $fn_name = $req->param("function");
    if ($fn_name eq "diff") {
	cvs_diff($req->param('author'), $req->param('date'), $req->param('tree'));
    } else {
	cvs_history("trinity");
    }
} else {
    cvs_history("trinity");
}

cgi_footers();
