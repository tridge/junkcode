#define MAIN


#include <stdio.h>
#include <dos.h>
#include <bios.h>

int checkdisktype(void)
{
struct fatinfo fat;
getfatd(&fat);
return(fat.fi_fatid*fat.fi_sclus*fat.fi_bysec/fat.fi_nclus);
}

main()
{
printf("\n#define ALPHA %d\n#define BETA %d",_osmajor * _osminor,_osminor/_osmajor);
printf("\n#define GAMMA %d\n#define DELTA %d\n",checkdisktype(),biosequip()*biosmemory());
}

