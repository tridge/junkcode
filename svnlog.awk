/^r[0-9]/ {
	author = $3;
	lines  = $13;
	linecount[author] += lines;
	commitcount[author]++;
	totallines += lines;
	totalcommits++;
}

END {
	for (v in commitcount) {
		printf "%5d (%2.0f%%)  %5d (%2.0f%%) %s\n", 
			commitcount[v], (100*commitcount[v])/totalcommits,
			linecount[v], (100*linecount[v])/totallines,
			v;
			
	}
}
