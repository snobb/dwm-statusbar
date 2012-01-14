/*  statusbar.c  */
/*  Copyright (C) 2012 Alex Kozadaev [akozadaev at yahoo com]  */

#include "build_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <X11/Xlib.h>

#ifdef DEBUG
#include <stdarg.h>
#define ERR_PREFIX  "!!ERROR: "
#endif

#define LABUF     15
#define DTBUF     20
#define STR       60

#ifdef DEBUG
void xerror(const char *msg, ...);
#endif

void set_status(char *str);
void open_display(void);
void close_display();
void get_load_avg(char *buf);
float get_battery(void);
void get_datetime(char *buf);
int get_wifi(void);
int read_int(const char *path);
void read_str(const char *path, char *buf, size_t sz);

static Display *dpy;

int
main(void)
{
  float bat;                 /* battery status */
  int lnk;                   /* wifi link      */
  char la[LABUF] = "\0";     /* load average   */
  char dt[DTBUF] = "\0";     /* date/time      */
  char stat[STR] = "\0";     /* full string    */

  open_display();

  while (!sleep(1)) {
    get_load_avg(la);
    lnk = get_wifi();
    bat = get_battery();
    get_datetime(dt);

    snprintf(stat, STR, "%s | %d | %0.1f%% | %s", la, lnk, bat, dt);
    set_status(stat);
  }

  close_display();
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
  if (!(dpy = XOpenDisplay(NULL))) {
#ifdef DEBUG
    xerror("Cannot open display.\n");
#endif
    exit(1);
  }
  signal(SIGINT, close_display);
  signal(SIGTERM, close_display);
}

void
close_display()
{
  XCloseDisplay(dpy);
#ifdef DEBUG
  fputs("statusbar: exiting...\n", stderr);
#endif
  exit(0);
}

#ifdef DEBUG
void
xerror(const char *msg, ...)
{
  va_list ap;

  fprintf(stderr, "%s", ERR_PREFIX); 

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
}
#endif

void
get_load_avg(char *buf)
{
  read_str(LA_PATH, buf, LABUF);
}

float
get_battery(void)
{
  int now, full;

  now = read_int(BAT_NOW);
  full = read_int(BAT_FULL);
  
  return (now * 100) / full;
}

void
get_datetime(char *buf)
{
  time_t rawtime;

  time(&rawtime);
  snprintf(buf, DTBUF, "%s", ctime(&rawtime));
}

int
get_wifi(void)
{
  int wifi = read_int(LNK_PATH);
  return wifi;
}

int
read_int(const char *path)
{
  FILE *fh;
  int i = 0;

  fh = fopen(path, "r");
  if (fh == NULL) {
#ifdef DEBUG
    xerror("Cannot open %s for reading.\n", path);
#endif
    return -1;
  }
  
  if (fscanf(fh, "%d", &i) < 0) {
#ifdef DEBUG
    xerror("Cannot read from %s\n", path);
#endif
  }
  
  fclose(fh);
  return i;
}

void
read_str(const char *path, char *buf, size_t sz)
{
  FILE *fh;

  fh = fopen(path, "r");
  if (fh == NULL) {
#ifdef DEBUG
    xerror("Cannot open %s for reading.\n", path);
#endif
    return;
  }
  
  if (fgets(buf, sz, fh) == NULL) {
#ifdef DEBUG
    xerror("Cannot read from %s.\n", path);
#endif
  }
  
  fclose(fh);
}

/*  EOF  */

