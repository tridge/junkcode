#include <stdio.h>

typedef struct {unsigned x;} FOOBAR;

#define X_FOOBAR(x) ((FOOBAR) { x })
#define FOO_ONE X_FOOBAR(1)

FOOBAR f = FOO_ONE;   

static const struct {
	FOOBAR y; 
} f2[] = {
	{FOO_ONE}
};   
