
{
  counts[$0]++;
  total++;
}

END {
  for (v in counts) {
    printf "%d (%.0f%%) %s\n", counts[v], (100*counts[v])/total, v;
  }
}
