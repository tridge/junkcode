#!/home/tridge/bin/template -c
Content-Type: text/html

{{#
	Junkcode home page
}}

{{ JUNKCODE=/ftp/unpacked/junkcode }}

<html>
<title>Tridge's junk code</title>
<body>

<h1>Tridge's junk code</h1>

This is a collection of bits of code that I have written over the
years but that hasn't ever been released as a full project. There are
lots of bits and pieces in here that I'm sure will be useful to
someone.<p>

The reason I call this 'junkcode' is that I have no plans to properly
document, package or support this code. If you find it useful then
that's great, but I already have enough free software projects to keep
me busy so I won't be spending a lot of time on this stuff.<p>

If you want to send me patches to this stuff then you can email
junkcode@samba.org but please don't expect me to be very
responsive.<p>

<h2>Downloading</h2>

Most of this code can be downloaded using anonymous cvs, rsync, ftp or
http. I leave it as a challenge to the reader to work out how.

<h2>License</h2>

Unless specified otherwise (either in the source code or in some
documentation) you can assume that this code is released under the GNU
General Public License version 2 or later. If you want to use the code
in a piece of free software and the GPL doesn't meet your needs for
some reason then feel free to contact me and I may offer that bit of
code under a different license.<p>

<h2>Author</h2>

Most of this code was written by <a
href="http://samba.org/~tridge/">Andrew Tridgell<a>. 

<h2>Index</h2>
<table>
<tr>
{{!
	count=`/bin/ls /home/httpd/html/junkcode/*.txt | wc -l`
	ncols=5
	per_col=`expr \( $count + $ncols - 1 \) / $ncols`
	i=0
	for f in /home/httpd/html/junkcode/*.txt; do
	    name=`basename $f .txt` 

	    if [ $i -eq 0 ]; then
	       echo "<td valign=top><ul>"
	    fi

	    echo "<li> <a href="#$name">$name</a>"
	    i=`expr $i + 1`

	    if [ $i -eq $per_col ]; then
	       echo "</ul></td>"
	       i=0
	    fi
	done
}}
</tr>
</table>

{{ C_SOURCE=<p>Download and compile <a href="{{$JUNKCODE}}/{{$name}}.c">{{$name}}.c</a><p> }}
{{ DIR_SOURCE=<p>Download <a href="{{$JUNKCODE}}/{{$name}}/">here</a><p> }}
{{ TOP_SOURCE=<p>Download <a href="/ftp/unpacked/{{$name}}/">here</a><p> }}

{{!
	for f in /home/httpd/html/junkcode/*.txt; do
	name=`basename $f .txt` 
	echo "{{name=$name}}"
cat << EOF
	<hr>
	<a name="$name">
	<h3>$name</h3>
	`cat $f`
	<p>
EOF
	done
}}

</body>
</html>