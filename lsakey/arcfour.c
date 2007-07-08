#include "includes.h"

void arcfour(uint8_t *data, int len, const uchar *key, int key_len)
{
	uint8_t s_box[256];
	uint8_t index_i = 0;
	uint8_t index_j = 0;
	uint8_t j = 0;
	int ind;
	
	for (ind = 0; ind < 256; ind++) {
		s_box[ind] = (uint8_t)ind;
	}

	for (ind = 0; ind < 256; ind++) {
		uint8_t tc;

		j += (s_box[ind] + key[ind % key_len]);

		tc = s_box[ind];
		s_box[ind] = s_box[j];
		s_box[j] = tc;
	}
	for (ind = 0; ind < len; ind++) {
		uint8_t tc;
		uint8_t t;

		index_i++;
		index_j += s_box[index_i];

		tc = s_box[index_i];
		s_box[index_i] = s_box[index_j];
		s_box[index_j] = tc;
		
		t = s_box[index_i] + s_box[index_j];
		data[ind] = data[ind] ^ s_box[t];
	}
}
