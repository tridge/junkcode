#!/usr/bin/perl -w
# This CGI script presents the results of the build_farm build
#
# Copyright (C) Andrew Tridgell <tridge@samba.org>     2003
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

use strict qw{vars};
use lib qw ( /home/tridge/cvs_history );
use util;
use history;
use POSIX;
use Data::Dumper;
use CGI::Base;
use CGI::Form;
use File::stat;

my (%trees) = ('samba_ntvfs' => "");

my $req = new CGI::Form;

################################################
# start CGI headers
sub cgi_headers() {
    print "Content-type: text/html\r\n";

    util::cgi_gzip();

    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd"> 
<html>
<head>
<style type="text/css">  
.entry {
        background-color: #f0f0ff;
        width: 80%;
        text-align: left;
        margin: 15px 0px 15px 0px;
        border: 1px solid gray;
}
.entry TABLE {
        width: 100%;
}
.entry TABLE TD {
        vertical-align: top;
	text-align: left;
}
.entry TABLE TH {
        vertical-align: top;
        width: 5em;
        text-align: left;
}

TABLE.header {
        border: 1px solid black;
}
TABLE.header TH {
        font-weight: normal;
        width: 12em;
        text-align: left;
}
TABLE.header TD {
}

</style>
<title>NAS cvs history</title></head>
<body bgcolor="white" text="#000000" link="#0000EE" vlink="#551A8B" alink="#FF0000">
<table border=0>
</table>
';

}

################################################
# start CGI headers for diffs
sub cgi_headers_diff() {
    print "Content-type: application/x-diff\r\n";
    print "\n";
}

################################################
# end CGI
sub cgi_footers() {
    print "</body>";
    print "</html>\n";
}

##############################################
# main page
sub main_menu() {
    print $req->startform("GET");
    print $req->popup_menu("tree", [sort keys %trees]);
    
    print $req->submit('function', 'Recent Checkins');

    print $req->endform();
}

###############################################
# display top of page
sub page_top() {
    cgi_headers();
    main_menu();
}


###############################################
# main program

my $tree = $req->param('tree');

if (! $tree) {
    $tree = "samba_ntvfs";
}


if (defined $req->param("function")) {
    my $fn_name = $req->param("function");
    if ($fn_name eq "Recent Checkins") {
	page_top();
	history::cvs_history($req->param('tree'));
	cgi_footers();
    } elsif ($fn_name eq "diff") {
	page_top();
	history::cvs_diff($req->param('author'), $req->param('date'), $req->param('tree'), "html");
	cgi_footers();
    } elsif ($fn_name eq "text_diff") {
	page_top_diff();
	history::cvs_diff($req->param('author'), $req->param('date'), $req->param('tree'), "text");
    } else {
	page_top();
	history::cvs_history($tree);
	cgi_footers();
    }
} else {
    page_top();
    history::cvs_history($tree);
    cgi_footers();
}

