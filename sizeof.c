#include <stdio.h>
#include <sys/types.h>

main()
{
        printf("char  : %d\n", sizeof(char));
        printf("short : %d\n", sizeof(short));
        printf("int   : %d\n", sizeof(int));
        printf("long   : %d\n", sizeof(long));
        printf("long long  : %d\n", sizeof(long long));
        printf("int * : %d\n", sizeof(int *));
        printf("time_t : %d\n", sizeof(time_t));
        printf("off_t : %d\n", sizeof(off_t));
        printf("size_t : %d\n", sizeof(size_t));
        printf("uid_t : %d\n", sizeof(uid_t));
        printf("dev_t : %d\n", sizeof(dev_t));
        printf("ino_t : %d\n", sizeof(ino_t));
        printf("float : %d\n", sizeof(float));
        printf("double : %d\n", sizeof(double));
        printf("pid_t : %d\n", sizeof(pid_t));
}
