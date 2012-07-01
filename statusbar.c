/*  statusbar.c  */
/*  Copyright (C) 2012 Alex Kozadaev [akozadaev at yahoo com]  */

#include "build_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <X11/Xlib.h>

#define LABUF     15
#define DTBUF     20
#define STR       60

void set_status(char *str);
void open_display(void);
void close_display();
void get_datetime(char *buf);
int read_int(const char *path);
void read_str(const char *path, char *buf, size_t sz);

static Display *dpy;

int
main(void)
{
  float bat;                /* battery status */
  int   lnk;                /* wifi link      */
  char  la[LABUF] = "\0";   /* load average   */
  char  dt[DTBUF] = "\0";   /* date/time      */
  char  stat[STR] = "\0";   /* full string    */

#ifndef DEBUG
  open_display();
#endif

  while (!sleep(1)) {
    read_str(LA_PATH, la, LABUF);           /* Load average */
    lnk = read_int(LNK_PATH);               /* link status */
    get_datetime(dt);                       /* date/time */
    bat = ((float)read_int(BAT_NOW) / 
           read_int(BAT_FULL)) * 100.0f;    /* battery */

    snprintf(stat, STR, "%s | %d | %0.1f%% | %s", la, lnk, (bat > 100) ? 100 : bat, dt);
#ifndef DEBUG
    set_status(stat);
#else
    puts(stat);
#endif
  }

#ifndef DEBUG
  close_display();
#endif
  return 0; 
}

void
set_status(char *str)
{
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
}

void
open_display(void)
{
  if (!(dpy = XOpenDisplay(NULL))) 
    exit(1);
  signal(SIGINT, close_display);
  signal(SIGTERM, close_display);
}

void
close_display()
{
  XCloseDisplay(dpy);
  exit(0);
}

void
get_datetime(char *buf)
{
  time_t rawtime;
  time(&rawtime);
  snprintf(buf, DTBUF, "%s", ctime(&rawtime));
}

int
read_int(const char *path)
{
  int i = 0;
  FILE *fh;

  if (!(fh = fopen(path, "r")))
    return -1;

  fscanf(fh, "%d", &i);
  fclose(fh);
  return i;
}

void
read_str(const char *path, char *buf, size_t sz)
{
  FILE *fh;

  if (!(fh = fopen(path, "r")))
    return;

  fgets(buf, sz, fh);
  fclose(fh);
}

/*  EOF  */

