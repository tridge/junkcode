
/* PROTOTYPES? */


/* DEFINITIONS FOR OPTIONS ANALYST 19/5/88 */


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


typedef enum
{MONTH,STRIKE,VALUEC,VALUEP,HELDC,HELDP,DELTAC,DELTAP,
SHAREPRICE,STOCKHELD,VOLATILITY,INTEREST,DATE,YEARMONTH,SHARESPER,
EXPIRYDAY,DAYSLEFT,DIVIDENDDAY,DIVIDENDCENTS,
VOLC,VOLP,MARKETC,MARKETP,COVERVAL,POVERVAL
  } coltype;

typedef enum
{SCREEN1, SCREEN2, SCREEN3, SCREEN4, SCREEN5
} screentype;

typedef enum
{PAGEUP, PAGEDOWN
} pagetype;

typedef enum
{DELTAS,HELDS,INVVOL,OVERVALUED} displaytype;

typedef struct
{       int month;
	float strike;
	double valuec;
	double valuep;
	double deltac;
	double deltap;
	int heldc;
	int heldp;
	double marketc;
	double marketp;
	double volc;
	double volp;
	double coverval;
	double poverval;
} datarow;

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
colortype colors;
cursortype cursor;
int graphcolors[8];
int linestyles[5];
int fontsizes[3];
} graphicstype;



typedef struct
{	int row;
	coltype col;
} celltype;

typedef struct
{
	int dyear;
	int dmonth;
	int sharesper;
	int daysleft;
	int dday;
	int payout;
} dividendsize;

typedef struct
{
        int eyear;
	int emonth;
	int eday;
	int daysleft;
} expirytype;

typedef struct
{
char username[65];
} nametype;

typedef struct
{
int gposn;
double gval;
} gvaltype;

typedef struct
{
char product[20];
nametype names;
float stockprice;
float interest;
float volatility;
dividendsize  sizepay[24];
datarow data[31];
int prevtimespan;
double previnterval;
char legend1[3][10];
char legend2[6][10];
double pricedata[3][MAXPOINTS];   /* first being x axis, subsequent ones y axis values*/
double timedata[6][MAXPOINTS];
int numpoints1;
int numpoints2;
long stockheld;
} statustype;

typedef struct
{
char product[20];
char date[7];
int dateget;
char lastfile[20];
celltype cell;
screentype screen;
int decimal;
displaytype display;
graphicstype graphics;
expirytype expiry[24];
char usernames[65];
int prt; /* 0 Epson   1 IBM */
} systype;


typedef struct
{
int days;
double s;
} divtype;



#if !defined(MAIN)


extern  char codename[65];

extern double totalvalue,totaldelta,weightedvol;
extern long totalcalls,totalputs;
extern double weightedvolc,weightedvolp;

extern  char months[12][3];

extern char startname[65];



extern char filename[20];

extern int recalcvolvalues;
extern int numpts;
extern pagetype page;

extern int syear;
extern int smonth;
extern int sday;

extern  char utilchoice[5][40];

extern  char resolutionchoice[4][40];

extern  char graphchoice[5][40];


/* COLOR CGA CARD */
extern  graphicstype ccgagraphics;

/* MONO CGA CARD */
extern  graphicstype mcgagraphics;

/* HERCULES CARD */
extern  graphicstype hercgraphics;
/* COLOR EGA CARD */
extern  graphicstype cegagraphics;

/* MONO EGA CARD */
extern  graphicstype megagraphics;

extern statustype status;

extern systype sys;


extern divtype divmatrix[13];

#endif