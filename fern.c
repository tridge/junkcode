
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <tridgec/tridge.h>

#define NUM_RAN	200
#define MAX_COLOUR 255
#define XOFS 0
#define YOFS 0

typedef unsigned char PIXEL;



/*******************************************************************
write a pixel matrix
*******************************************************************/
BOOL write_pix_matrix(PIXEL **mat,int size,CONST char *fname)
{
FILE *file;
file = file_open(fname,"w");
fwrite((char *)&mat[0][0],sizeof(PIXEL),size*size,file);
file_close(file);
return(True);
}

/*******************************************************************
read a pixel matrix
*******************************************************************/
BOOL read_pix_matrix(PIXEL **mat,int size,CONST char *fname)
{
FILE *file;
file = file_open(fname,"r");
fread((char *)&mat[0][0],sizeof(PIXEL),size*size,file);
file_close(file);
return(True);
}


/*******************************************************************
make a 2 D  pixel matrix
********************************************************************/
PIXEL **pmatrix2D(int dim1,int dim2)
{
PIXEL **ret;
int i,j;

ret = (PIXEL **)any_matrix(2,sizeof(PIXEL),dim1,dim2);
if (ret == NULL) return(NULL);

for (i=0;i<dim1;i++)
for (j=0;j<dim2;j++)
	ret[i][j] = 0.0;
return(ret);
}




int N,NUM_ITTR;
BOOL read_in;

int main(int argc,char *argv[])
{
float a[4],b[4],c[4],d[4],e[4],f[4];
int randx , randy;
int loop,i,j;
PIXEL **oldimage;
PIXEL **newimage;
int COL1 = 180;

if (argc<5)
	{
	fprintf(debugfile,"Usage: %s N niter read_in base_colour\n",argv[0]);
	exit(0);
	}

N = atoi(argv[1]);
NUM_ITTR = atoi(argv[2]);
read_in = atoi(argv[3]);
COL1 = atoi(argv[4]);

xStartDisplay(N,N,argc,argv);

	oldimage = pmatrix2D(N,N);
	newimage = pmatrix2D(N,N);


	if ((oldimage == NULL) || (newimage == NULL))
		{
		fprintf(debugfile,"Help!!!");
		exit(0);
		}
	
	/* initialize the transformation values */

	a[0] = 0 ; a[1] = 0.85 ; a[2] = 0.2 ; a[3] = -0.15 ;
	b[0] = 0 ; b[1] = 0.04 ; b[2] = -0.26 ; b[3] = 0.28;
	c[0] = 0 ; c[1] = -0.04 ; c[2] = 0.23 ; c[3] = 0.26;
	d[0] = 0.16 ; d[1] = 0.85 ; d[2] = 0.22 ; d[3] = 0.24;
	e[0] = 0 ; e[1] = 0 ; e[2] = 0 ; e[3] = 0 ;
	f[0] = 0 ; f[1] = 1.6 ; f[2] = 1.6 ; f[3] = 0.44;


if (read_in)
	read_pix_matrix(oldimage,N,"fern.mat");
else
{
	/* set up initial configuration */

	start_random();
	for ( i = 0 ; i < NUM_RAN ; i++)
	{
		randx = rand_float(0.0,N-1);
		randy = rand_float(0.0,N-1);
		oldimage[randx][randy] = 1;
	}
}


	for(loop=0;loop<NUM_ITTR;loop++)
	{
	int changed=0;
	fprintf(stdout,"Loop %d\n",loop);
	for(i=0; i < N; i++)
		for (j=0; j < N; j++)
		{

#define OK(ii,jj) (!((ii<0) || (jj<0) || (ii>=N) || (jj>=N)))
#define ISHIFT 200
#define JSHIFT 30
#define SCALE	70 
			if (oldimage[i][j])
			{
			int k;
			int ii,jj;
			for (k=0;k<4;k++)
				{
				ii = (a[k]*(i-ISHIFT) + b[k]*(j-JSHIFT) + e[k]*SCALE);
				jj = (c[k]*(i-ISHIFT) + d[k]*(j-JSHIFT) + f[k]*SCALE);
				ii += ISHIFT;
				jj += JSHIFT;
				if (OK(ii,jj))
					newimage[ii][jj] = oldimage[ii][jj]+1;
				}
			}
		}



 	xClearDisplay(); 

	for(i=0; i<N ; i++)
	for(j=0;j<N;j++)
		{
		PIXEL old,new;
		old = oldimage[i][j];
		new = newimage[i][j];

			if ((old != new) && (old*new == 0))
				changed++;
		        
		        if (new)
			  {
			    xSetForeground(COL1 + new - 1);
			    xPutPoint(XOFS + i,YOFS + j);
			  }

			oldimage[i][j] = new;
			newimage[i][j] = 0;
		}
	xSync(False);

	if ((loop % 10 == 0) || (changed == 0))
		write_pix_matrix(oldimage,N,"fern.mat");

	fprintf(stdout,"\n Changed pixels = %d ",changed);
	if (changed == 0) break;
}

free(oldimage);
free(newimage);

xWaitEvent(eKeyPress);

xEndDisplay();
return(0);
}


