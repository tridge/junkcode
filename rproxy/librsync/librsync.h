#ifndef _LIBRSYNC_H
#define _LIBRSYNC_H

/* returns total bytes written to write_fn */
ssize_t
librsync_signature(void *readprivate,
		   void *sigwriteprivate,
		   int (*read_fn)(void *readprivate, char *buf, size_t len),
		   int (*sigwrite_fn)(void *sigwriteprivate, char *buf, size_t len),
		   unsigned int block_len
		   );


/* returns total bytes written to write_fn */
ssize_t
librsync_encode(void *readprivate,
		void *writeprivate,
		void *sigreadprivate,
		int (*read_fn)(void *readprivate, char *buf, size_t len),
		int (*write_fn)(void *writeprivate, char *buf, size_t len),
		int (*sigread_fn)(void *sigreadprivate, char *buf, size_t len)
		);
		
		
/* returns total bytes written to write_fn */
ssize_t
librsync_decode(void *readprivate,
		void *writeprivate,
		void *littokreadprivate,
		int (*pread_fn)(void *readprivate, char *buf,
				size_t len, off_t offset),
		int (*write_fn)(void *writeprivate, char *buf,
				size_t len),
		int (*littokread_fn)(void *littokreadprivate,
				     char *buf, size_t len)
		);


void librsync_dump_logs();

#endif
