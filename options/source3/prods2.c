#define MAIN

#define BUFLEN 30000
#include "pinst.h"

#define NULL 0L
#define NORM 0x8184
#define SEEK_SET 0
#define SEEK_CUR 1



char serial[10];
char startname[70] =  "CCCCCC";
char mediumfudge[50];
char codename[70] =  "AAAAA";
char codesname[65];
int fhandle;
char buffer[BUFLEN];
char *string1;
long lastseek;
int i;


extern unsigned char _osmajor;
extern unsigned char _osminor;

void encode(char *name,char *coded)
{
coded[0] = name[4] *2 - name[8]+2 ;
coded[1] = name[6] * 3 - 2 * coded[0]+1;
coded[2] = name[8] * 2 - coded[1] +3;
coded[3] = name[2] *3 - coded[0] - coded[2];
coded[4] = name[3] * 2 - coded[2];
coded[5] = name[5] * coded[2] / (coded[3] + 7);
coded[6] = name[0] * 2 - coded[4]+ 15;
coded[7] = name[1] * 4 - 3* coded[5] + 7;
coded[8] = name[7] * coded[5] + coded[7];
}



int pirated()
{
int count;
encode(&(startname[1]),&(codesname[1]));
encode(&(startname[7]),&(codesname[6]));
encode(&(startname[15]),&(codesname[12]));
encode(&(startname[35]),&(codesname[17]));
encode(&(startname[40]),&(codesname[23]));
encode(&(startname[35]),&(codesname[29]));
encode(&(startname[17]),&(codesname[34]));
encode(&(startname[19]),&(codesname[39]));
encode(&(startname[22]),&(codesname[45]));
encode(&(startname[27]),&(codesname[51]));
for (count = 0; count < 60; count++)
{
if (codesname[count]== 64) codesname[count] = 65;
if (codesname[count] == 0) codesname[count] = sqrt(count + 10);
}
codesname[50] = 0;
}


void readstring(char *buf)
{
while (*(buf-1) != 13)
{
*(buf++) = toupper(getch());
putch(*(buf-1));
if (*(buf-1) == 8) buf -= 2;
}
*(buf-1) = 0;
}


char *find(char ch,char *buff,char *limit,int num)
{
int count = 0;
while (buff < limit)
{
if (*buff++ == ch) count++;
              else count = 0;
if (count == num) return(buff-num);
}
return(NULL);
}


int checkdisktype(void)
{

struct fatinfo
	{
	char fi_fatid;
	char fi_sclus;
	int fi_bysec;
	int fi_nclus;
	} fat;
getfatd(&fat);
return(fat.fi_fatid*fat.fi_sclus*fat.fi_bysec/fat.fi_nclus);
}

main()
{
printf("\n   ENTER NAME : ");
startname[0] = 20;
startname[1] = 8;
readstring(&startname[2]);
printf("\n   ENTER SERIAL NO. : ");
readstring(serial);

for (i = strlen(startname);i < 30;i++)
startname[i] = ' ';
sprintf(&startname[30],"SERIAL No.    %s%c",serial,0);

pirated();
printf("\nSearching.....");
fhandle = open("options.exe",NORM);
lseek(fhandle,1,SEEK_SET);

do
{
lastseek = read(fhandle,&buffer,BUFLEN);
string1 = find(_osmajor * _osminor - ALPHA + 'C',buffer,&buffer[BUFLEN],checkdisktype()- GAMMA + 5);
if (string1 != NULL)
	{
	printf("\nsuccessful 1\n");
	sprintf(string1,"%s",startname);
	string1 = find(_osminor / _osmajor - BETA + 'A',buffer,&buffer[BUFLEN],checkdisktype()- GAMMA + 5);
	if (string1 != NULL)
		{
		printf("successful 2\n");
		sprintf(string1,"%s",codesname);
		lseek(fhandle,-lastseek,SEEK_CUR);
		write(fhandle,&buffer,BUFLEN);
		printf("\nSuccessfully Installed for :\n%s",startname);
		exit(0);
		}
	}
} while (string1 == NULL && !eof(fhandle));
printf("Un-Successful\n");
close(fhandle);
}

