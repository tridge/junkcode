
/* DEFINITIONS FOR OPTIONS ANALYST COLOR CHANGER 27/12/88 */


#define FALSE 0
#define TRUE 1
#define ARROWR  77
#define ARROWL  75
#define ARROWU  72
#define ARROWD  80
#define PGUP	73
#define PGDN	81
#define HOME    71
#define END     79
#define TAB  9
#define LTAB 11
#define F01 59
#define F02 60
#define F03 61
#define F04 62
#define F05 63
#define F06 64
#define F07 65
#define F08 66
#define F09 67
#define F10 68
#define VLINE 179
#define HLINE 196
#define MAXPOINTS 30


typedef struct
{	int border;
	int heading;
	int heading2;
	int commands;
	int status;
	int data;
	int databack;
	int highlight;
	int highback;
	int windowfore;
	int windowback;
} colortype;

typedef struct
{	int hidden;
	int shown;
} cursortype;

typedef struct
{
char name[4];
int colors[11];
int cursor[2];
int graphcolors[8];
int linestyles[5];
int fontsizes[3];
} graphicstype;

typedef enum
{CCGA,MCGA,HERC,CEGA,MEGA} coltype;

