#include <stdio.h>
#include <string.h>
#include <zlib.h>

static unsigned infunc(void *in_desc, unsigned char **bptr)
{
	static char buf[1024];
	int fd = *(int *)in_desc;
	int count;

	count = read(fd, buf, sizeof(buf));
	if (count <= 0) return 0;

	*bptr = buf;
	return count;	
}

static int outfunc(void *out_desc, unsigned char *buf, unsigned count)
{
	int fd = *(int *)out_desc;
	int ret = write(fd, buf, count);
	if (ret == count) return 0;
	return -1;	
}

int main(void)
{
	z_stream stream;
	char buf[32768];
	int ret;
	int in_desc=0, out_desc=1;

	memset(&stream, 0, sizeof(stream));

	ret = inflateBackInit(&stream, 15, buf);
	if (ret != Z_OK) {
		fprintf(stderr, "inflateBackInit failed - %d\n", ret);
		return -1;
	}
	
	ret = inflateBack(&stream, infunc, &in_desc, outfunc, &out_desc);
	if (ret < 0) {
		fprintf(stderr, "inflateBack failed - %d\n", ret);
		return -1;
	}

	return 0;
}
