#include <stdio.h>
#include <string.h>


unsigned char Data[100001];
unsigned char keystream[1001];
int Rpoint[300];


main (int argc,char *argv[]) {
  FILE *fd;
  int 	i,j,k;
  int	size;
  char ch;
  char *name;
  int cracked;
  int sizemask;
  int maxr;
  int rsz;
  int pos;
  int Rall[300]; /* resource allocation table */

  if (argc<2) {
    printf("usage: glide filename (username)");
    exit(1);
  }

  /* read PWL file */

  fd=fopen(argv[1],"rb");
  if(fd==NULL) {
    printf("can't open file %s",argv[2]);
    exit(1);
  }
  size=0;
  while(!feof(fd)) {
    Data[size++]=fgetc(fd);
  }
  size--;
  fclose(fd);

  /* find username */
  name=argv[1];
  if(argc>2) {
    name=argv[2];  
  } else {
    char *p = strrchr(name,'/');
    if (p) name = p+1;
  }
  printf("Username: %s\n",name);

  /* copy encrypted text into keystream */
  cracked=size-0x0208;
  if(cracked<0) cracked=0;
  if(cracked>1000) cracked=1000;
  memcpy(keystream,Data+0x208,cracked );

	/* generate 20 bytes of keystream */
  for(i=0;i<20;i++) {
    ch=toupper(name[i]);
    if(ch==0) break;
    if(ch=='.') break;
    keystream[i]^=ch;
  };
  cracked=20;


	/* find allocated resources */

  sizemask=keystream[0]+(keystream[1]<<8);
  printf("Sizemask: %04X\n",sizemask);

  for(i=0;i<256;i++) Rall[i]=0;

  maxr=0;
  for(i=0x108;i<0x208;i++) {
    if(Data[i]!=0xff) {
      Rall[Data[i]]++;
      if (Data[i]>maxr) maxr=Data[i];
    }
  }
  maxr=(((maxr/16)+1)*16);	/* resource pointer table size appears
to be divisible by 16 */

	/* search after resources */

  Rpoint[0]=0x0208+2*maxr+20+2;	/* first resource */
  for(i=0;i<maxr;i++) {
    /* find size of current resource */
    pos=Rpoint[i];
    rsz=Data[pos]+(Data[pos+1]<<8);
    rsz^=sizemask;
    printf("Analyzing block with size:
%04x\t(%d:%d)\n",rsz,i,Rall[i]);
    if( (Rall[i]==0) && (rsz!=0) ) {
      printf("unused resource has nonzero size !!!\n");
      exit(0);
    }

    pos+=rsz;

		/* Resources have a tendency to have the wrong size for some
reason */
		/* check for correct size */

    if(i<maxr-1) {
      while(Data[pos+3]!=keystream[1]) {
	printf(":(%02x)",Data[pos+3]);
	pos+=2; /* very rude may fail */
      }
    }

    pos+=2;	/* include pointer in size */
    Rpoint[i+1]=pos;
  }
  Rpoint[maxr]=size;

	/* insert Table data into keystream */
  for(i=0;i <= maxr;i++) {
    keystream[20+2*i]^=Rpoint[i] & 0x00ff;
    keystream[21+2*i]^=(Rpoint[i] >> 8) & 0x00ff;
  }
  cracked+=maxr*2+2;

  printf("%d bytes of keystream recovered\n",cracked);

  /* decrypt resources */
  for(i=0;i < maxr;i++) {
    rsz=Rpoint[i+1]-Rpoint[i];
    if (rsz>cracked) rsz=cracked;
    printf("Resource[%d] (%d)\n",i,rsz);
    for(j=0;j<rsz;j++) printf("%c",Data[Rpoint[i]+j]^keystream[j]);
    printf("\n");
  }


  exit(0);
}
