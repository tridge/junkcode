#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ndd_var.h>
#include <sys/kinfo.h>

/*
  get ethernet MAC address on AIX
 */
static int aix_get_mac_addr(const char *device_name, uint8_t mac[6])
{
        size_t ksize;
        struct kinfo_ndd *ndd;
	int count, i;

        ksize = getkerninfo(KINFO_NDD, 0, 0, 0);
        if (ksize == 0) {
		errno = ENOSYS;
		return -1;
        }

        ndd = (struct kinfo_ndd *)malloc(ksize);
        if (ndd == NULL) {
		errno = ENOMEM;
		return -1;
        }

        if (getkerninfo(KINFO_NDD, ndd, &ksize, 0) == -1) {
		errno = ENOSYS;
		return -1;
        }

	count= ksize/sizeof(struct kinfo_ndd);
	for (i=0;i<count;i++) {
		if ((ndd[i].ndd_type == NDD_ETHER || 
		     ndd[i].ndd_type == NDD_ISO88023) &&
		    ndd[i].ndd_addrlen == 6 &&
		    (strcmp(ndd[i].ndd_alias, device_name) == 0 ||
		     strcmp(ndd[i].ndd_name, device_name == 0))) {
                        memcpy(mac, ndd[i].ndd_addr, 6);
			free(ndd);
			return 0;
		}
        }
	free(ndd);
	errno = ENOENT;
	return -1;
}


int main(int argc, char *argv[])
{
	unsigned char mac[6];
	int i, ret;

	if (argc != 2) {
		printf("Usage: aix_getmac <interface>\n");
		exit(1);
	}
	ret = aix_get_mac_addr(argv[1], mac);
	if (ret == -1) {
		perror("aix_getmac");
		exit(1);
	}
	printf("%02x:%02x:%02x:%02x:%02x:%02x\n", 
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
