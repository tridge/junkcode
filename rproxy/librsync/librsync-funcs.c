#include "includes.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>


#define DEFLITBUFSIZE 1024

static char glob_checksum2[SUM_LENGTH];
static char * old_glob_inbuf = NULL;
static char * glob_inbuf = NULL;
static int glob_block_count;

static struct librsync_stats_s _librsync_stats;

static char librsync_log_buf[LIBRSYNC_MAXLOGLINES][LIBRSYNC_MAXLOGLINELEN+1];
static int librsync_log_start = 0;
static int librsync_log_end = 0;

/*  #define librsync_log printf */
#define librsync_log librsync_log2

static int
librsync_log2 (const char *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vsnprintf(librsync_log_buf[librsync_log_end%LIBRSYNC_MAXLOGLINES],
			LIBRSYNC_MAXLOGLINELEN, fmt, ap);
	va_end(ap);

	if (ret == -1) ret = LIBRSYNC_MAXLOGLINELEN;

	librsync_log_buf[librsync_log_end%LIBRSYNC_MAXLOGLINES][ret] = 0;
	librsync_log_end++;
	if (librsync_log_start < librsync_log_end-LIBRSYNC_MAXLOGLINES) {
		librsync_log_start =  librsync_log_end-LIBRSYNC_MAXLOGLINES;
	}
	return 0;
}

int
librsync_log_get_line(char * buf)
{
	/* buf is of size MAXLOGLINELEN */

	if (librsync_log_start==librsync_log_end) return 0;

	memcpy(buf, librsync_log_buf[librsync_log_start++],
	       LIBRSYNC_MAXLOGLINELEN);

	return 1;
}

static char *
my_sum_fun(void)
{
	if (old_glob_inbuf!=glob_inbuf) {
		get_checksum2(glob_inbuf,glob_block_count,glob_checksum2);
		old_glob_inbuf = glob_inbuf;
	}
	return glob_checksum2;
}

/*  void */
/*  printmdsum(char * sum) { */
/*    librsync_log("Got sum 2: %u%u%u%u\n", */
/*  	       (int)(sum[0]), */
/*  	       (int)(sum[4]), */
/*  	       (int)(sum[8]), */
/*  	       (int)(sum[12])); */
/*  } */


static int
do_read(int (*read_fn)(void *readprivate, char *buf,size_t len),
	 void *readprivate, char *buf,size_t len)
{
	int count = 0;
	int ret;

	if (!len) return len;

	while (count < len) {
		ret = read_fn(readprivate,buf+count,len-count);  
		if (ret==0) return count;
		if (ret == -1) return ret;
		count+=ret;
	}
  
	return count;
}

static int
do_write(int (*write_fn)(void *writeprivate, char *buf,size_t len),
	 void *writeprivate, char *buf,int len)
{
	int count = 0;
	int ret;
	int iter = 0;

	if (!len) return len;

	while (count < len) {
		ret = write_fn(writeprivate,buf+count,len-count);  
		count+=ret;
		if (ret == -1) return ret;
		if (ret==0 && ++iter>100) {
			errno = EIO;
			return -1;
		}
	}
  
	return count;
}

static int32 read_prev;
static int32 write_prev;

static int
read_int32(ssize_t (*read_fn)(void *readprivate, char * buf, size_t len),
	   void * readprivate, int * retval) {
	char mybuf[4];
	int ret;
	
	ret = do_read(read_fn, readprivate,mybuf,4);
	if (ret == -1) return ret;
	
	if (ret==0) { return 0; }
	
	*retval = IVAL(mybuf,0) + read_prev;
	read_prev = *retval;

/*  	librsync_log("Got value %d\n", *retval); */

	return 4;
}

static ssize_t
write_int32(ssize_t (*write_fn)(void *writeprivate, char *buf,size_t len),
	    void *writeprivate, int32 anint) {
	char buf[4];

	SIVAL(buf,0,anint - write_prev);
	write_prev = anint;
	
	return do_write(write_fn, writeprivate, buf, 4);
}

static ssize_t
write_buf(ssize_t (*write_fn)(void *writeprivate,char *buf,size_t len),
	  void *writeprivate,
	  char * buffer,
	  size_t buflen) {
	size_t totalwritten = 0;
	ssize_t writtenbytes;
	
	if (!buflen) return 0;

	librsync_log("Writing %d bytes of literal data \n", buflen);

	/* Write the size of the literal data */
	writtenbytes = write_int32(write_fn, 
				   writeprivate,
				   buflen);
	if (writtenbytes<0) { return writtenbytes; }
	totalwritten += writtenbytes;
	
	/* Write the literal data */
	writtenbytes = do_write(write_fn, writeprivate,buffer,buflen);
	if (writtenbytes<0) { return writtenbytes; }

	totalwritten+=writtenbytes;

	return totalwritten;
}



ssize_t
librsync_signature(void *readprivate,
		   void *sigwriteprivate,			
		   int (*read_fn)(void *readprivate, char *buf, size_t len),
		   int (*write_fn)(void *sigwriteprivate, char *buf,size_t len),
		   unsigned int block_len)
{
	ssize_t count;
	char * inbuf;
	int totalsigsize = 0;
	int ret;

	librsync_log("**** librsync_signature *******\n","");

	inbuf = malloc(block_len);
	if (!inbuf) {
		librsync_log("Failed to allocate %d bytes of memory\n", block_len);
		errno = ENOMEM;
		return -1;
	}
	
	/* Current format:
	 * 4 bytes for version number
	 * 4 bytes for block length
	 * (int checksum1 SUM_LENGTH checksum2)*
	 */
	
	ret = write_int32(write_fn, sigwriteprivate,LIBRSYNC_VERSION);
	if (ret == -1) return ret;
	totalsigsize+=ret;
  
	ret = write_int32(write_fn, sigwriteprivate,block_len);
	if (ret == -1) return ret;
	totalsigsize+=ret;


	while (((count = do_read(read_fn, readprivate,inbuf,block_len)))>0) {
		int checksum1;
		char checksum2[SUM_LENGTH];

		checksum1 = get_checksum1(inbuf,count);

		if ((ret = get_checksum2(inbuf,count,checksum2)) == -1) {
			return ret;
		}

		ret = write_int32(write_fn,sigwriteprivate,checksum1);
		if (ret == -1) return ret;
		totalsigsize+=ret;

		ret = do_write(write_fn,sigwriteprivate,checksum2,SUM_LENGTH);
		if (ret == -1) return ret;
		totalsigsize+=ret;
	}
	free(inbuf);
	
	if (count<0) {
		return count;
	}

	return totalsigsize;
}



static int
make_sum_struct(struct sum_struct ** signatures,
		int (*sigread_fn)(void *sigreadprivate, char *buf, size_t len),
		void * sigreadprivate,
		int block_len)
{
  
	struct sum_buf * asignature;
	int count = 0;
	int index = 0;
	int ret = 0;
	int checksum1;

	if ((*signatures = malloc(sizeof(struct sum_struct)))==NULL) {
		errno = ENOMEM;
		return -1;
	}
	memset(*signatures,0,sizeof(struct sum_struct));
	(*signatures)->n = block_len;

	(*signatures)->sums = NULL;

	while ((ret=read_int32(sigread_fn, sigreadprivate,&checksum1))==4) {

		(*signatures)->sums = realloc((*signatures)->sums,
					      (count+1)*sizeof(struct sum_buf));
		if ((*signatures)->sums == NULL) {
			errno = ENOMEM;
			ret = -1;
			break;
		}
		asignature = &((*signatures)->sums[count]);

		asignature->sum1 = checksum1;
		asignature->i = ++index;

		/* read in the long sum */
		ret = do_read(sigread_fn,sigreadprivate,asignature->sum2,
			      SUM_LENGTH);
		if (ret == -1) { break; };

		count++;
	}
	if (ret == -1) { return ret; };

	(*signatures)->count = count;
	librsync_log("Read %d sigs\n",count);

	ret = build_hash_table(*signatures)<0;

	return ret;
}


ssize_t
librsync_encode(void *readprivate,
		void *writeprivate,
		void *sigreadprivate,
		int (*read_fn)(void *readprivate,char *buf,size_t len),
		int (*write_fn)(void *writeprivate,char *buf,size_t len),
		int (*sigread_fn)(void *sigreadprivate, char *buf, size_t len)
		)
{
	char * inbuf;
	char * outbuf;
	int outbufsize = 0;
	int inbufsize;
	int bytesread = 0;
	int totalwritten = 0;
	int block_len;
	int writtenbytes = 0;
	struct sum_struct * signatures;
	int ret;
	int i = 0;
	int sum;
	int token;
	int librsync_remote_version;
	int havesum;
	int s1,s2;

	librsync_log("**** librsync_encode *******\n","");

	ret = read_int32(sigread_fn, sigreadprivate,&librsync_remote_version);
	if (ret!=4) { return ret; }

	librsync_log("remote version: %d\n",librsync_remote_version);

	if (librsync_remote_version != LIBRSYNC_VERSION) {
		librsync_log("This is rsync %d. We don't take %d.\n",
			     LIBRSYNC_VERSION, librsync_remote_version);
		errno = EBADMSG;
		return -1;
	}

	ret = read_int32(sigread_fn, sigreadprivate, &block_len);
	if (ret!=4) { return ret; }

	librsync_log("The block length is %d\n", block_len);


#define SIG_BUFFER_INCREMENTS 4096

	/* Fix all the memory leaks here. sig_buffer_p must be freed upon return */

	/* Put the char * sigbuffer into our structures */
	if ((ret=make_sum_struct(&signatures,
				 sigread_fn,
				 sigreadprivate,
				 block_len))<0) {
		return ret;
	};



	/* Write the protocol version the token stream follows to the token stream*/
	writtenbytes = write_int32(write_fn,writeprivate, LIBRSYNC_VERSION);
	if (writtenbytes<0) return writtenbytes;
	totalwritten+=writtenbytes;

	/* Write the block length to the token stream. Not necessary, but we do
	 *  it to simplify the interface ("I'd buy that for four bytes")
	 */
	writtenbytes = write_int32(write_fn, writeprivate,block_len);
	if (writtenbytes<0) return writtenbytes;
	totalwritten+=sizeof(int);





	/* Now do our funky checksum checking */

#define INBUFLEN ((block_len + 512) & ~511)
#define OUTBUFLEN 256
	inbuf = malloc(INBUFLEN);
	outbuf = malloc(OUTBUFLEN);
	if (!inbuf || !outbuf) {
		errno = ENOMEM;
		return -1;
	}
	outbufsize = 0;
	inbufsize = 0;

	havesum = 0;
	glob_block_count = block_len;
	while((bytesread = read_fn(readprivate,inbuf+inbufsize,INBUFLEN-inbufsize))>0) {
		inbufsize+=bytesread;
		librsync_log("Read %d bytes from read_fn\n", bytesread);

		i=0;
		while(i+block_len<=inbufsize) {
			if (!havesum) {
				sum = get_checksum1(inbuf+i,block_len);
				librsync_log("new checksum after block match: %u\n", sum);
				havesum = 1;
				s1 = sum & 0xFFFF;
				s2 = sum >> 16;
			} else {
				/* Add the value for this character */
				/* The previous byte is already subtracted (below) */
				s1 += (inbuf[i+block_len-1]+CHAR_OFFSET);
				s2 += s1;
				sum = s1 + (s2<<16);
			}

			glob_inbuf = inbuf+i;
			if ((token = find_in_hash(sum,my_sum_fun))>0) {
				librsync_log("Found token %d in stream at offset %d\n", token,i);
	
				/* Write all the literal data out */
				writtenbytes= write_buf(write_fn, writeprivate,
							outbuf, outbufsize);
				if (writtenbytes<0) { return writtenbytes; }
				totalwritten += writtenbytes;
				outbufsize = 0;

				/* Write the token */
				librsync_log("Writing token %d to encoded string \n", token);
				writtenbytes = write_int32(write_fn,
							   writeprivate,
							   -token);
				if (writtenbytes<0) { return writtenbytes; }

				totalwritten += writtenbytes;

				i+=block_len;
				havesum = 0;
				old_glob_inbuf = NULL;
			} else {
				/* Append this character to the outbuf */
				outbuf[outbufsize++] = inbuf[i];
				
				if (outbufsize == OUTBUFLEN) {
					librsync_log("Flushing literal data\n");
				/* Write all the literal data out */
					writtenbytes= write_buf(write_fn,
								writeprivate,
								outbuf,
								outbufsize);
					if (writtenbytes<0) { return writtenbytes; }
					totalwritten += writtenbytes;
					outbufsize = 0;
				}
				s1 -= inbuf[i] + CHAR_OFFSET;
				s2 -= block_len * (inbuf[i]+CHAR_OFFSET);
				i++;
			}
		}
		/* Copy the remaining data into the front of the buffer */
		memcpy(inbuf,
		       inbuf+i,
		       inbufsize-i);
		inbufsize = inbufsize-i;
	}
	if (bytesread<0) {
		return bytesread;
	}


	librsync_log("the remainder of the buffer is %d bytes\n",inbufsize);

	/* Now the remainder of the buffer */
	i = 0;
	while (i<inbufsize) {
		if (!havesum) {
			sum = get_checksum1(inbuf+i,inbufsize-i);
			havesum = 1;
			s1 = sum & 0xFFFF;
			s2 = sum >> 16;
		} else {
			s1 += (inbuf[i+block_len-1]+CHAR_OFFSET);
			s2 += s1;
			sum = s1 + (s2<<16);
		}

		glob_inbuf = inbuf+i;
		glob_block_count = inbufsize-i;

		if ((token = find_in_hash(sum,my_sum_fun))>0) {
      
			librsync_log("Found token %d in stream at offset %d\n", token,i);
      
			/* Write all the literal data out */
			writtenbytes= write_buf(write_fn,
						writeprivate,
						outbuf,
						outbufsize);
			if (writtenbytes<0) { return writtenbytes; }
			totalwritten += writtenbytes;
			outbufsize = 0;
			
			/* Write the token */
			librsync_log("Writing token %d to encoded string \n",
				     token);
			writtenbytes = write_int32(write_fn,
						   writeprivate,
						   -token);
			if (writtenbytes<0) {
				return writtenbytes;
			}
			totalwritten += writtenbytes;

			i+=block_len;
			havesum = 0;
			old_glob_inbuf = NULL;
		} else {
			/* Append this character to the outbuf */

			outbuf[outbufsize++] = inbuf[i];
      
			if (outbufsize == OUTBUFLEN) {
				librsync_log("Flushing literal data\n");
				/* Write all the literal data out */
				writtenbytes= write_buf(write_fn,
							writeprivate,
							outbuf,
							outbufsize);
				if (writtenbytes<0) { return writtenbytes; }
				totalwritten += writtenbytes;
				outbufsize = 0;				
			}
			s1 -= inbuf[i] + CHAR_OFFSET;
			s2 -= block_len * (inbuf[i]+CHAR_OFFSET);
			i++;
		}
	}

	/* Flush any literal data remaining */
	librsync_log("Flushing %d bytes of remaining data\n",outbufsize);
	writtenbytes= write_buf(write_fn,
				writeprivate,
				outbuf,
				outbufsize);
	if (writtenbytes<0) { return writtenbytes; }
	totalwritten += writtenbytes;
	outbufsize = 0;				

	/* Terminate the stream with a null */
	writtenbytes = write_int32(write_fn,writeprivate, 0);
	if (writtenbytes<0) return writtenbytes;

	totalwritten+=writtenbytes;

	/* We'll write a real-signature of the transfered file here */
  
	return totalwritten;
}


ssize_t
librsync_decode(void *readprivate,
		void *writeprivate,
		void *littokreadprivate,
		int (*pread_fn)(void *readprivate, char *buf,
				size_t len, off_t offset),
		int (*write_fn)(void *writeprivate, char *buf, size_t len),
		int (*littokread_fn)(void *littokreadprivate,
				     char *buf, size_t len)
		)
{
	char * litbuf;
	int totalwritten = 0;
	int litbufsize;
	int type;
	int readbytes = 0;
	int writtenbytes = 0;
	char * blockbuf;
	int librsync_remote_version;
	int block_len;
	int ret;
  
	librsync_log("******  librsync_decode *********\n");

	ret = read_int32(littokread_fn, littokreadprivate,
			 &librsync_remote_version);
	if (ret!=4) { return ret; };

	librsync_log("remote version: %d\n", librsync_remote_version);
	if (librsync_remote_version != LIBRSYNC_VERSION) {
		librsync_log("Version mismatch: %d != %d\n",
			     librsync_remote_version, 
			     LIBRSYNC_VERSION);
		errno = EBADMSG;
		return -1;
	}


	ret = read_int32(littokread_fn, littokreadprivate, &block_len);
	if (ret!=4) { return ret; };
	librsync_log("Block_len: %d\n", block_len);

	litbuf = malloc(DEFLITBUFSIZE);
	blockbuf = malloc(block_len);
	if (!litbuf || !blockbuf) {
		errno = ENOMEM;
		return -1;    
	}
	litbufsize = DEFLITBUFSIZE;

	type = 1;
	while ((ret=read_int32(littokread_fn, littokreadprivate,&type))==4) {
		if (type==0) break; /* We're done! */

		if (type>0) { /* Literal data */
			int count = type;
			librsync_log("Receiving %d bytes of literal data\n", count);
			if (count > MAXLITDATA) {
				librsync_log("Literal count of %d exceeds %d!\n", count, MAXLITDATA);
				errno = ERANGE;
				return -1;
			}
			while (count>0) {

				readbytes = do_read(littokread_fn, littokreadprivate, litbuf, count);
				if (readbytes<0) return readbytes;

				if (readbytes==0) {
					librsync_log("Expecting %d bytes of literal dat, receiving %d bytes\n", type, count);
					errno = ENODATA;
					return -1;
				}
				writtenbytes = do_write(write_fn, writeprivate,
							litbuf,readbytes);
				if(writtenbytes<0) return writtenbytes;

				totalwritten+=writtenbytes;
				count-=readbytes;
			}
		}

		if (type<0) {
			int ret = 0;
			ret = pread_fn(readprivate, blockbuf, block_len,
				       (-type-1)*block_len);
			if (ret == -1) { return ret; }
			librsync_log("Writing block %d of input file (%d bytes) to output file\n",
				     -type-1, ret);
			ret = do_write(write_fn,writeprivate, blockbuf, ret);
			if (ret == -1) { return ret; }
      
			totalwritten += ret;
		}
	}
	if (ret!=4) {
		return ret;
	}
	librsync_log("Termination: readbytes = %d, type = %d\n", readbytes,type);
	free (litbuf);
	free (blockbuf);

	return totalwritten;
}

int
librsync_get_stats(struct librsync_stats_s dest) {
	memcpy(&dest, &_librsync_stats, sizeof(struct librsync_stats_s));

	return 0;
}

void
librsync_dump_logs(void) {
	char myline[LIBRSYNC_MAXLOGLINELEN];

	while (librsync_log_get_line(myline)) {
		fprintf(stderr, "%s", myline);
	}
}
