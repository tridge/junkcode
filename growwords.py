#! /usr/bin/python

# Copyright (C) 2003 by Martin Pool <mbp@sourcefrog.net>

# Given a dictionary of words, find the longest word that can be grown
# from a three-letter seed by successively adding one letter, then
# rearranging.  All the intermediate stages must occur in the
# dictionary.

# The solution is determined by the dictionary and the seed.

# There may be more than one equally long solution.

# Since we rearrange the word after each addition we just need to
# remember the population count of letters, not the particular word.

# Perhaps we could use a Python 26-tuple giving the count of each
# letter as a key...

# It seems like a greedy algorithm might work here...

# Algorithm: We hold a list of known 'reachable' words.  Initialize
# the list of good words with the seed word.  Read the dictionary and
# squash case and discard non-letter symbols.  Iterate through the
# dictionary, considering the words in order of size.

# For each word in the dictionary, it is reachable if it can be made
# by adding a letter to any shorter reachable word, and then
# rearranging.  Therefore it does not matter what order we process the
# words, as long as we do them in strictly ascending order by length.

# How do we efficiently find out if it is reachable?  A
# straightforward way is to try removing each unique letter from the
# word, and see if that population is reachable.  In other words, we
# turn the word into its population, and then generate the set of
# populations that are decreased by one in every non-zero count.

# So "aabd" has the population (2, 1, 0, 1).  This is reachable if we
# have (1, 1, 0, 1), or (2, 0, 0, 1) or (2, 1, 0, 0).

# So for each word, we need to check a number of possible predecessors
# limited to the number of unique letters in the word; at most 26.  If
# we store the reachable words in a hashtable indexed by population,
# we can do the probes in O(1) time.

# Therefore we can do the whole thing in time proportional to the
# number of words, O(n).  I think this is optimal, since we have to at
# the very least read in each word and therefore cannot be quicker
# than O(n).

# In the first version I read all the words into a single list and
# then sorted it by length.  Doing the sort by length is quite slow in
# Python and in addition this is at best O(n log n).  Doing a complete
# sort is a lot of work when we require just a very rough sort by
# size.  So instead we read the words into buckets by length, then
# join them up.

# The bucket thing may be a bit more than O(n), because of the
# possible cost of copying things as we join the buckets.  But it is
# not very expensive compared to the actual search.  There are a
# fairly small number of buckets anyhow -- just proportional to the
# longest word.

# Solution:

# Seed is given as the command line argument; words are read from
# stdin.  This solution doesn't require the seed to be three letters.

# A good way to feed it is from

#   grep '^[a-z]*$' /usr/share/dict/words




import sys, string
from pprint import pprint
from xreadlines import xreadlines
import re
import profile

letters_re = re.compile('[a-z]*')

def read_words():
    """Read from stdin onto all_words"""
    # indexed by length; contents is a list of words of that length
    by_len = {}

    # XXX: You'll get a deprecation warning here for Python 2.3.  I just use
    # xreadlines for the benefit of old machines.
    
    for w in xreadlines(sys.stdin):
        if w[-1] == '\n':
            w = w[:-1]                  # chomp

        # check chars are reasonable
        if not letters_re.match(w):
            raise ValueError()

        w = w.lower()
        l = len(w)

        # Put it into the right bucket for its length.  Make a new
        # one if needed.
        wl = by_len.get(l)
        if wl is None:
            wl = []
            by_len[l] = wl
        wl.append(w)

    # Now join up all the buckets so that we have one big list, sorted by
    # word length
    all_words = []
    lens = by_len.keys()
    lens.sort()
    for l in lens:
        all_words.extend(by_len[l])

    return all_words


def make_pop(w):
    # Make a 26-tuple giving the count of each letter in a word
    pop = [0]
    for l in string.ascii_lowercase:
        pop.append(w.count(l))
    return tuple(pop)

def make_reduced(p):
    """Given a population p, return a sequence of populations that are
    reduced by one character from p."""
    r = []
    for i in range(26):
        if p[i] > 0:
            # Copy it and reduce the i'th element.  The extra comma is
            # to form a tuple.
            reduced = (p[i] - 1),
            p2 = p[:i] + reduced + p[i+1:]
            r.append(p2)
    return r

def note_reachable(reachable, w):
    reachable[make_pop(w)] = w

def main():
    seed = sys.argv[1]
    all_words = read_words()
    
    ## all_words.sort(lambda x, y: cmp(len(x), len(y)))
    ## pprint(all_words)

    # reachable is a map from population tuples to a valid word with
    # that population
    reachable = {}

    # start off with just the seed
    note_reachable(reachable, seed)

    for w in all_words:
        w_pop = make_pop(w)
        if reachable.has_key(w_pop):
            # already have an anagram of w
            continue

        # for each possible predecessor of w, see if it is already reachable
        for v_pop in make_reduced(w_pop):
            if reachable.has_key(v_pop):
                print "%s reachable from %s" % (w, reachable[v_pop])
                note_reachable(reachable, w)
                break

        
if __name__ == '__main__':
    try:
        # Cool!  Psyco roughly halves the runtime.
        import psyco
        psyco.log('/tmp/psyco.log')
        psyco.full()
    except:
        print 'Psyco not found, ignoring it'
    main()
