/* Here is an implementation of these routines for
   output to Postscript files */



FILE *outfile;

#define PTS_PER_INCH 72
#define P_SCALE_X 0.5
#define P_SCALE_Y 0.5
#define PAGE_WIDTH 8 /* inches */
#define PAGE_HEIGHT 8 /* inches */
#define P_MAXX ((int)(PAGE_WIDTH*PTS_PER_INCH/P_SCALE_X))
#define P_MAXY ((int)(PAGE_HEIGHT*PTS_PER_INCH/P_SCALE_Y))
#define P_ORIGIN_X 0
#define P_ORIGIN_Y 100
#define P_LINE_WIDTH 1.0
#define P_FONT "Courier-Bold"
#define P_FONT_SIZE 12
#define P_FONT_WIDTH (36.0*P_FONT_SIZE/60)
#define P_LINE_CAP 0
#define P_MAC1 "/x { moveto } def"
#define P_MAC2 "/y { lineto } def"
#define P_MAC3 "/z { stroke } def"



void start_graphics()
{
/* Start it going */
	outfile = stdout;
	fprintf(outfile,"%c!\ninitgraphics\n",'%');
/* Setup scaling etc. */
	fprintf(outfile,"%-4.2f %-4.2f scale\n",P_SCALE_X,P_SCALE_Y);
	fprintf(outfile,"%d %d translate\n",P_ORIGIN_X,P_ORIGIN_Y);
/* Define some macros */
	fprintf(outfile,"%s\n%s\n%s\n",P_MAC1,P_MAC2,P_MAC3);
/* Default text size etc. */
	fprintf(outfile,"/%s findfont\n",P_FONT);
	fprintf(outfile,"%d scalefont\n",(int)(P_FONT_SIZE/P_SCALE_X));
	fprintf(outfile,"setfont\n");
/* Default line width, style etc. */
	fprintf(outfile,"%-3.1f setlinewidth\n",P_LINE_WIDTH);
	fprintf(outfile,"%d setlinecap\n",P_LINE_CAP);
	fprintf(outfile,"[] 0 setdash\n");
}

void end_graphics()
{
/* display it */
	fprintf(outfile,"showpage\n");
/* close file */
/*	fclose(outfile);*/
}

int get_maxx()
{
	return(P_MAXX);
}

int get_maxy()
{
	return(P_MAXY);
}

int new_y(y)
int y;
/* Flip in Y direction to make origin in bottom left */
{
	return(get_maxy()-y);
}

void draw_line(x1,y1,x2,y2)
int x1,y1,x2,y2;
{
	fprintf(outfile,"%d %d x %d %d y z\n",x1,new_y(y1),x2,new_y(y2));
}

int descender()
/* Height of descender */
{
	return(char_height()/3);
}

void graph_text(x,y,str)
int x,y;
char *str;
{
	fprintf(outfile,"%d %d x (%s) show\n",x,new_y(y)+descender(),str);
}

void choose_line_type(type)
int type;
{
char *dashing;
double width;
int index;
index = type % 6;
switch (index)
{
	case 0 : dashing="[] 0"; width=1.2; break;
	case 1 : dashing="[6] 0"; width=1.2; break;
	case 2 : dashing="[4] 1"; width=1.2; break;
	case 3 : dashing="[] 0"; width=1.8; break;
	case 4 : dashing="[6] 0"; width=1.8; break;
	case 5 : dashing="[4] 1"; width=1.8; break;
}
	fprintf(outfile,"%-3.1f setlinewidth\n",
		width*P_LINE_WIDTH);
	fprintf(outfile,"%s setdash\n",dashing);

}

void wait_for_end()
{
}

int char_width()
{
	return((int)(P_FONT_WIDTH/P_SCALE_X));
}

int char_height()
{
	return((int)(P_FONT_SIZE/P_SCALE_Y));
}

/* End of PostScript specific routines */

