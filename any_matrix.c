#include <stdio.h>
#include <malloc.h>
#include <stdarg.h>

/*******************************************************************
create a matrix of any dimension. The return must be cast correctly.
********************************************************************/
void *any_matrix(int dimension, int el_size, ...)
{
	int *dims=NULL;
	void **mat;
	int i,j,size,ptr_size,ppos,prod;
	int padding;
	void *next_ptr;
	va_list ap;
	
	if (dimension <= 0) return(NULL);
	if (el_size <= 0) return(NULL);
	
	dims = (int *)malloc(dimension * sizeof(int));
	if (dims == NULL) return(NULL);

	/* gather the arguments */
	va_start(ap, el_size);
	for (i=0;i<dimension;i++) {
		dims[i] = va_arg(ap, int);
	}
	va_end(ap);
	
	/* now we've disected the arguments we can go about the real
	   business of creating the matrix */
	
	/* calculate how much space all the pointers will take up */
	ptr_size = 0;
	for (i=0;i<(dimension-1);i++) {
		prod=sizeof(void *);
		for (j=0;j<=i;j++) prod *= dims[j];
		ptr_size += prod;
	}

	/* padding overcomes potential alignment errors */
	padding = (el_size - (ptr_size % el_size)) % el_size;
	
	/* now calculate the total memory taken by the array */
	prod=el_size;
	for (i=0;i<dimension;i++) prod *= dims[i];
	size = prod + ptr_size + padding;

	/* allocate the matrix memory */
	mat = (void **)malloc(size);

	if (mat == NULL) {
		fprintf(stdout,"Error allocating %d dim matrix of size %d\n",dimension,size);
		free(dims);
		return(NULL);
	}

	/* now fill in the pointer values */
	next_ptr = (void *)&mat[dims[0]];
	ppos = 0;
	prod = 1;
	for (i=0;i<(dimension-1);i++) {
		int skip;
		if (i == dimension-2) {
			skip = el_size*dims[i+1];
			next_ptr = (void *)(((char *)next_ptr) + padding); /* add in the padding */
		} else {
			skip = sizeof(void *)*dims[i+1];
		}

		for (j=0;j<(dims[i]*prod);j++) {
			mat[ppos++] = next_ptr;
			next_ptr = (void *)(((char *)next_ptr) + skip);
		}
		prod *= dims[i];
	}

	free(dims);
	return((void *)mat);
}




/*
  double ****x;
  double ***y;

  x = (double ****)any_matrix(4, sizeof(double), 3, 6, 8, 2);

  x[0][3][4][1] = 5.0;

  y = x[1];

  y[2][3][4] = 7.0;

  free(x);
*/
