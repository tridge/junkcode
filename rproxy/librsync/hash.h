#ifndef _LIBRSYNC_HASH_H
#define _LIBRSYNC_HASH_H

#include "librsync.h"

int build_hash_table(struct sum_struct *s);
int find_in_hash(int sum,char *(sum_fun)(void));

#endif
