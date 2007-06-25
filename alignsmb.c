#include <stdio.h>

typedef unsigned __u32;
typedef unsigned char __u8;
typedef unsigned short __u16;

#pragma pack(1)

# define offsetof(T,F) ((unsigned int)((char *)&((T *)0L)->F - (char *)0L))

struct smb_hdr {
	__u32 smb_buf_length;	/* big endian on wire *//* BB length is only two or three bytes - with one or two byte type preceding it but that is always zero - we could mask the type byte off just in case BB */
	__u8 Protocol[4];
	__u8 Command;
	union {
		struct {
			__u8 ErrorClass;
			__u8 Reserved;
			__u16 Error;	/* note: treated as little endian (le) on wire */
		} DosError;
		__u32 CifsError;	/* note: le */
	} Status;
	__u8 Flags;
	__u16 Flags2;		/* note: le */
	__u16 PidHigh;		/* note: le */
	union {
		struct {
			__u32 SequenceNumber;  /* le */
			__u32 Reserved; /* zero */
		} Sequence;
		__u8 SecuritySignature[8];	/* le */
	} Signature;
	__u8 pad[2];
	__u16 Tid;
	__u16 Pid;		/* note: le */
	__u16 Uid;
	__u16 Mid;
	__u8 WordCount;
};

struct smb_hdr h;

int main(void)
{
	printf("size is %d\n", sizeof(h));

	h.Status.CifsError = 3;

	printf("error is at %d\n", offsetof(struct smb_hdr, Status.CifsError));

	
	h.Status.CifsError++;

	printf("error is %d\n", h.Status.CifsError++);

	return 0;
}
