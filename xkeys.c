#ifdef XWINDOWS

#ifdef __TURBOC__
#include "tcinc.h"
#else
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#endif

#define EXTERN extern
#include "bug.h"


Display *Dpy=NULL;
Window ww=0;


/*******************************************************************
send one keysym to a window
********************************************************************/
int send_key(Display *dpy,Window w,int state,int keysym)
{
  static XKeyEvent event;
  static count=0;
  event.serial = count++;
  event.type = 2;
  event.send_event = 1;
  event.x = event.y = 0;
  event.x_root = event.y_root = 0;
  event.same_screen = 1;
  event.time = 0;
  event.window = w;
  event.subwindow = w;
  event.root = 0;
  event.state = state;
  event.keycode = XKeysymToKeycode(dpy,keysym);
  XSendEvent(dpy,w, True, KeyPressMask, &event);
  return 0;
}


/*******************************************************************
find a window that matches a name
********************************************************************/
int find_window(Display *dpy, Window w,char *match)
{
  Window root, parent;
  unsigned int nchildren;
  Window *children = NULL;
  int n;
  char *name = NULL;
  sstring aname;
  
  XFetchName (dpy, w, &name);

  if (!name)
    {
      XWindowAttributes wattr;
      XGetWindowAttributes(dpy,w,&wattr); /* do check return - Linus */
      sprintf(aname,"%dx%d",wattr.width,wattr.height);
      name = aname;
    }

  if (name) Debug(3,"Found window '%s'\n",name);
  
  if (name && strncmp(name,match,strlen(match)) == 0)
    {
      Debug(1,"matched %s to %s\n",name,match);
      if (name && name != aname) XFree ((char *) name);
      if (children) XFree ((char *) children);
      return(w);
    }

  if (name && name != aname) XFree ((char *) name);   
  
  if (XQueryTree (dpy, w, &root, &parent, &children, &nchildren))
    for (n = 0; n < nchildren; n++)
      {
	Window Ww = find_window (dpy, children[n],match);
	if (Ww != 0) return(Ww);
      }
  if (children) XFree ((char *) children);
  return 0;
}



/*******************************************************************
open a window for writing keystrokes to
********************************************************************/
FN BOOL X_open_window(char *match)
{
  Dpy = XOpenDisplay (NULL);
  if (!Dpy) {
    Error("unable to open display \"%s\"\n",XDisplayName (NULL));
    return False;
  }

  ww = find_window (Dpy, RootWindow(Dpy, DefaultScreen(Dpy)),match);

  if (!ww)
    {
      Error("can't find window matching %s\n",match);
      return False;
    }

  return True;
}


/*******************************************************************
close the keystroke window
********************************************************************/
FN void X_close_window(void)
{
  if (Dpy)
    XCloseDisplay(Dpy);
  Dpy = NULL;
  ww = 0;
}


/*******************************************************************
send a string to a window
********************************************************************/
FN BOOL X_send_string(char *str)
{
  char s[1024];
  static int state = 0;
  char *tok;

  if (!Dpy || !ww) 
    return(False);

  strcpy(s,str);
  
  tok = strtok(s," \t\n\r");
  do
    {
      int keysym = XStringToKeysym(tok);
      if (strlen(tok) == 1 && *tok>='A' && *tok <='Z')
	{
	  state ^= ShiftMask;	  
	  send_key(Dpy,ww,state,XK_Shift_L);
	  send_key(Dpy,ww,state,keysym);
	  state ^= ShiftMask;	  
	  send_key(Dpy,ww,state,XK_Shift_L);
	}      
      else
	{
	  if (IsModifierKey(keysym))       
	    {
	      Debug(3,"modifier %d\n",keysym);
	      switch (keysym)
		{
		case XK_Shift_L:
		case XK_Shift_R:
		  state ^= ShiftMask;
		  break;
		case XK_Control_L:
		case XK_Control_R:
		  state ^= ControlMask;
		  break;
		case XK_Alt_L:
		case XK_Alt_R:
		  state ^= Mod1Mask;
		  break;
		}
	    }
	  send_key(Dpy,ww,state,keysym);
	}
    }
  while ((tok = strtok(NULL," \t\n\r")));

  XFlush(Dpy);
  return True;
}

#endif
