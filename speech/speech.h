/* Useful definitions of types used in speech processing */
/* Andrew Tridgell 1991 */

/* #define USE_FORTRAN */

typedef unsigned char	byte;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef signed short	int16;
typedef signed int	int32;
typedef float		real4;
typedef double		real8;
typedef	struct {int16 i1,i2,i3;} real6; /* 6 byte DOS reals */
typedef signed char	BOOL;
#define True 1
#define False 0
#define Pi	3.1415927
#define Pi2	(2*Pi)

#define FLOAT double

#define ABS(x) ((x)<0?-(x):(x))
