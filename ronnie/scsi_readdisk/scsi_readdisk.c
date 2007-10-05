#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>

#define SCSI_TIMEOUT 5000 /* ms */


static struct timeval tp1,tp2;
static size_t num_blocks = 1;
static int do_sync = 0;
static int do_random = 0;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

const char *sensetable[16]={
	"no sense",
	"recovered error",
	"not ready",
	"medium error",
	"hardware error",
	"illegal request",
	"unit attention",
	"data protect",
	"blank check",
	"vendor specific",
	"copy aborted",
	"aboreted command",
	"unknown",
	"unknown",
	"unknown",
	"unknown"
};

void print_sense_data(unsigned char *sense, int sense_len)
{
	int i;
	unsigned char asc, ascq;

	printf("Device returned sense information\n");
	if(sense[0]==0x70){
		printf("filemark:%d eom:%d ili:%d  sense-key:0x%02x (%s)\n",
			!!(sense[2]&0x80),
			!!(sense[2]&0x40),
			!!(sense[2]&0x20),
			sense[2]&0x0f,
			sensetable[sense[2]&0x0f]);
		printf("command specific info: 0x%02x 0x%02x 0x%02x 0x%02x\n",
			sense[8],sense[9],sense[10],sense[11]);

		asc=sense[12];
		printf("additional sense code:0x%02x\n", asc);

		ascq=sense[13];
		printf("additional sense code qualifier:0x%02x\n", ascq);

		printf("field replacable unit code:0x%02x\n", sense[14]);

		if((asc==0x20)&&(ascq==0x00))
			printf("INVALID COMMAND OPERATION CODE\n");
	}

	printf("Sense data:\n");
	for(i=0;i<sense_len;i++){
		printf("0x%02x ", sense[i]);
		if((i%8)==7)printf("\n");
	}
	printf("\n");
}

int open_scsi_device(const char *dev)
{
	int fd, vers;

	if((fd=open(dev, O_RDWR))<0){
		printf("ERROR could not open device %s\n", dev);
		return -1;
	}
	if ((ioctl(fd, SG_GET_VERSION_NUM, &vers) < 0) || (vers < 30000)) {
		printf("/dev is not an sg device, or old sg driver\n");
		close(fd);
		return -1;
	}

	return fd;
}

int scsi_io(int fd, unsigned char *cdb, unsigned char cdb_size, int xfer_dir, unsigned char *data, unsigned int *data_size, unsigned char *sense, unsigned int *sense_len)
{
	sg_io_hdr_t io_hdr;

	memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';

	/* CDB */
	io_hdr.cmdp = cdb;
	io_hdr.cmd_len = cdb_size;

	/* Where to store the sense_data, if there was an error */
	io_hdr.sbp = sense;
	io_hdr.mx_sb_len = *sense_len;
	*sense_len=0;

	/* Transfer direction, either in or out. Linux does not yet
	   support bidirectional SCSI transfers ?
	 */
	io_hdr.dxfer_direction = xfer_dir;

	/* Where to store the DATA IN/OUT from the device and how big the
	   buffer is
	 */
	io_hdr.dxferp = data;
	io_hdr.dxfer_len = *data_size;

	/* SCSI timeout in ms */
	io_hdr.timeout = SCSI_TIMEOUT;


	if(ioctl(fd, SG_IO, &io_hdr) < 0){
		perror("SG_IO ioctl failed");
		return -1;
	}

	/* now for the error processing */
	if((io_hdr.info & SG_INFO_OK_MASK) != SG_INFO_OK){
		if(io_hdr.sb_len_wr > 0){
			*sense_len=io_hdr.sb_len_wr;
			return 0;
		}
	}
	if(io_hdr.masked_status){
		printf("status=0x%x\n", io_hdr.status);
		printf("masked_status=0x%x\n", io_hdr.masked_status);
		return -2;
	}
	if(io_hdr.host_status){
		printf("host_status=0x%x\n", io_hdr.host_status);
		return -3;
	}
	if(io_hdr.driver_status){
		printf("driver_status=0x%x\n", io_hdr.driver_status);
		return -4;
	}

	return 0;
}

unsigned long read_uint32(unsigned char *data)
{
	unsigned long val;

	val = data[0];
	val <<= 8;
	val |= data[1];
	val <<= 8;
	val |= data[2];
	val <<= 8;
	val |= data[3];

	return val;
}

struct scsi_inquiry_data {
	int peripheral_qualifier;
#define SCSI_DEV_TYPE_SBC	0x00
	int peripheral_device_type;
	int removable;
};

struct scsi_inquiry_data *scsi_inquiry(int fd)
{
	unsigned char cdb[]={0x12,0,0,0,0,0};

	unsigned int data_size=96;
	unsigned char data[data_size];

	unsigned int sense_len=32;
	unsigned char sense[sense_len];

	int res, alen, i;

	static struct scsi_inquiry_data inq;

	cdb[3]=(data_size>>8)&0xff;
	cdb[4]=data_size&0xff;


	res=scsi_io(fd, cdb, sizeof(cdb), SG_DXFER_FROM_DEV, data, &data_size, sense, &sense_len);
	if(res){
		printf("SCSI_IO failed\n");
		return NULL;
	}
	if(sense_len){
		print_sense_data(sense, sense_len);
		return NULL;
	}

	/* Peripheral Qualifier */
	inq.peripheral_qualifier = data[0]&0xe0;

	/* Peripheral Device Type */
	inq.peripheral_device_type = data[0]&0x1f;


	/* RMB */
	inq.removable = data[1]&0x80;

	return &inq;
}


struct scsi_capacity_data {
	unsigned long blocks;
	unsigned long blocksize;
};

struct scsi_capacity_data *scsi_read_capacity10(int fd)
{
	unsigned char cdb[]={0x25,0,0,0,0,0,0,0,0,0};

	unsigned int data_size=96;
	unsigned char data[data_size];

	unsigned int sense_len=32;
	unsigned char sense[sense_len];

	int res, alen, i;
	static struct scsi_capacity_data capacity;

	res=scsi_io(fd, cdb, sizeof(cdb), SG_DXFER_FROM_DEV, data, &data_size, sense, &sense_len);
	if(res){
		printf("SCSI_IO failed\n");
		return NULL;
	}
	if(sense_len){
		print_sense_data(sense, sense_len);
		return NULL;
	}

	capacity.blocks    = read_uint32(&data[0]);
	capacity.blocksize = read_uint32(&data[4]);

	return &capacity;
}

#define READ_FUA    0x04
#define REAF_FUA_NV 0x02
/* If READ_FUA is not set   the device may return data from cache
   if it is set  the device must read the data from medium
*/
unsigned char *scsi_read10(int fd, unsigned long lba, unsigned short len, unsigned char fua)
{
	unsigned char cdb[]={0x28,0,0,0,0,0,0,0,0,0};

	unsigned int data_size=len*512;

	unsigned int sense_len=32;
	unsigned char sense[sense_len];
	int res, alen, i;
	char *buf;

	buf = malloc(len * 512);
	if (!buf) {
		printf("Malloc of %d failed\n", len * 512);
		exit(1);
	}
	/* allow device to return data from cache or force a read from the medium ?*/
	cdb[1] = fua;


	/* lba */
	cdb[2] = (lba>>24)&0xff;
	cdb[3] = (lba>>16)&0xff;
	cdb[4] = (lba>> 8)&0xff;
	cdb[5] = (lba    )&0xff;

	/* number of blocks */
	cdb[7] = (len>>8)&0xff;
	cdb[8] = (len   )&0xff;

	res=scsi_io(fd, cdb, sizeof(cdb), SG_DXFER_FROM_DEV, buf, &data_size, sense, &sense_len);
	if(res){
		printf("SCSI_IO failed\n");
		return NULL;
	}
	if(sense_len){
		print_sense_data(sense, sense_len);
		return NULL;
	}

	return buf;
}

static void read_disk(int fd, struct scsi_capacity_data *capacity)
{
	static double total, thisrun;
	unsigned char *buf;
	unsigned long offset = 0;

	total = 0;
	while (1) {
		if (do_random) {
			offset = random() % (capacity->blocks - num_blocks);
		} else {
			offset += num_blocks;
		}
		buf = scsi_read10(fd, offset, num_blocks, do_sync);
		if (buf == NULL) {
			printf("read failure\n");
			exit(0);
		}
		free(buf);

		total += num_blocks*512;
		thisrun += num_blocks*512;
		if (end_timer() >= 1.0) {
			printf("%d MB    %g MB/sec\n", 
			       (int)(total/1.0e6),
			       (thisrun*1.0e-6)/end_timer());
			start_timer();
			thisrun = 0;
		}
	}
}


static void usage(void)
{
	printf("\n" \
"readfiles - reads from a list of files, showing read throughput\n" \
"\n" \
"Usage: readfiles [options] <files>\n" \
"\n" \
"Options:\n" \
"    -B size        number of 512-byte blocks per i/o\n" \
"    -S             force device to bypass any read cache\n" \
"    -R             random read\n" \
"");
}

int main(int argc, char *argv[])
{
	int i;
	extern char *optarg;
	extern int optind;
	int c;
	int fd;
	struct scsi_inquiry_data *inq_data;
	struct scsi_capacity_data *capacity;


	while ((c = getopt(argc, argv, "B:h:SR")) != -1) {
		switch (c) {
		case 'B':
			num_blocks = strtol(optarg, NULL, 0);
			break;
		case 'S':
			do_sync = READ_FUA;
			break;
		case 'R':
			do_random = 1;
			break;
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		usage();
		exit(1);
	}

	/* open the scsi device */
	fd = open_scsi_device(argv[0]);
	if (fd == -1) {
		perror(argv[0]);
		exit(10);
	}

	/* read inq data and make sure it is a DISK device   so that we will
	   use the correct scsi protocol later
	*/
	inq_data = scsi_inquiry(fd);
	if (inq_data == NULL) {
		printf("Failed to read INQ data from device\n");
		exit(10);
	}
	if (inq_data->peripheral_device_type != SCSI_DEV_TYPE_SBC) {
		printf("Not a SCSI disk\n");
		exit(10);
	}

	/* get size of the device in number of 512 byte blocks */
	capacity = scsi_read_capacity10(fd);
	if (capacity == NULL) {
		printf("Failed to read capacity from device\n");
		exit(10);
	}


	srandom(time(NULL));
	start_timer();

	read_disk(fd, capacity);

	return 0;
}

