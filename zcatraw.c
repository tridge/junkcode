#include <stdio.h>
#include <string.h>
#include <zlib.h>


int main(void)
{
	char inbuf[1024], outbuf[1024];
	struct z_stream_s zs;
	int ret;

	memset(&zs, 0, sizeof(zs));

	zs.next_out = outbuf;
	zs.avail_out = sizeof(outbuf);

	ret = inflateInit2(&zs, -15);
	if (ret != Z_OK) {
		fprintf(stderr,"inflateInit2 error %d\n", ret);
		goto failed;
	}

	while (1) {
		if (zs.avail_in == 0) {
			zs.avail_in = read(0, inbuf, sizeof(inbuf));
			zs.next_in = inbuf;
			if (zs.avail_in == 0) break;
		}

		ret = inflate(&zs, Z_SYNC_FLUSH);

		if (zs.avail_out < sizeof(outbuf)) {
			write(1, outbuf, sizeof(outbuf) - zs.avail_out);
			zs.avail_out = sizeof(outbuf);
			zs.next_out = outbuf;
		}

		if (ret == Z_STREAM_END) break;
		if (ret != Z_OK) {
			fprintf(stderr,"inflate error %d\n", ret);
			goto failed;
		}
	}

	return 0;

failed:
	return -1;
}
