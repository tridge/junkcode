

static uint32_t hash_fn(void *data);

struct myhash {
	int hash_size;
	struct hash_list *hash;
}

struct hash_list {
	struct hash_list *next, *prev;
	uint32_t full_hash;
	void *data;
};



h = hash_fn(data);

list = myhash->hash[h % myhash->hash_size];
for (l=list;l;l=l->next) {
	if (l->full_hash != h) continue;
	if (strcmp(l->data, data) != 0) continue;
	
}
