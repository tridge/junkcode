#include <stdio.h>
#include <stdlib.h>
#include "tiffio.h"
	
int main(int argc, char* argv[])
{
	TIFF *tiff;
	double res = 17.74;
	int i;
	
	if (argc < 2)
		return 1;

	res = atof(argv[1]);

	res = 1.0 / (res * 1.0e-4);

	for (i=2;i<argc;i++) {
		tiff = TIFFOpen(argv[i], "r+");
		if (tiff == NULL) {
			perror(argv[i]);
			return 2;
		}
	
		/* For list of available tags and tag values see tiff.h */
		if (TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER) != 1)
			fprintf( stderr, "Failed to set ResolutionUnit.\n");

		/* Don't forget to calculate and set new resolution values */
		if (TIFFSetField(tiff, TIFFTAG_XRESOLUTION, res) != 1)
			fprintf( stderr, "Failed to set XResolution.\n");
		if (TIFFSetField(tiff, TIFFTAG_YRESOLUTION, res) != 1)
			fprintf( stderr, "Failed to set YResolution.\n");
		
		TIFFRewriteDirectory(tiff);
		TIFFClose(tiff);
	}

	return 0;
}

