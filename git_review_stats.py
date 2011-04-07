#!/usr/bin/env python

import git

from optparse import OptionParser
parser = OptionParser("git_review_stats.py [options]")
parser.add_option("-n", "--num-months",  dest="n", default=1, type='int', help="number of months")
(opts, args) = parser.parse_args()

repo = git.Repo()

commits = repo.commits_since(since='%u months ago' % opts.n)

stats = {}

class counts(object):
    def __init__(self, name):
        self.name = name
        self.commit_count = 0
        self.reviewed_count = 0

    def review_pct(self):
        if self.commit_count == 0:
            return 0.0
        return (100.0*self.reviewed_count)/self.commit_count

for c in commits:
    author = str(c.author)
    committer = str(c.committer)
    
    if not author in stats:
        stats[author] = counts(author)

    reviewed = False
    for review_tag in ['Review', 'Signed-Off', 'Pair-Program']:
        if c.message.find(review_tag) != -1:
            reviewed = True
    if author != committer:
            reviewed = True

    stats[author].commit_count += 1
    if reviewed:
        stats[author].reviewed_count += 1


keys = stats.keys()
keys = sorted(keys, key=lambda author: -stats[author].review_pct())
for author in keys:
    s = stats[author]
    print("%4.1f %4u %4u %s" % (s.review_pct(), s.reviewed_count, s.commit_count, s.name))
    
