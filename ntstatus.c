#include <stdio.h>

typedef unsigned uint32;
typedef unsigned BOOL;


#ifdef __GNUC__
typedef struct {uint32 v;} NTSTATUS;
#define NT_STATUS(x) ((NTSTATUS) { x })
#define NT_STATUS_V(x) ((x).v)
#else
typedef uint32 NTSTATUS;
#define NT_STATUS(x) (x)
#define NT_STATUS_V(x) (x)
#endif

BOOL bar(NTSTATUS x)
{
	return x;
}

NTSTATUS foo(void)
{
	return NT_STATUS(3);
}

int main()
{
	NTSTATUS x;
	
	x = foo();
	bar(1);

	printf("%d\n", NT_STATUS_V(x));
	return 0;
}
