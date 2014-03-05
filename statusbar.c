/*  statusbar.c  */
/*  Copyright (C) 2012 Alex Kozadaev [akozadaev at yahoo com]  */

#include "build_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <X11/Xlib.h>

/* version 0.64 */

#define THRESHOLD 8
#define TIMEOUT   40
#define SUSPEND   { "/bin/sh", "/usr/local/bin/suspend.sh", NULL }

#define LABUF     14
#define DTBUF     20
#define LNKBUF    8
#define STR       64

/* Available statuses 
 *
 *  Charging
 *  Discharging
 *  Unknown
 *  Full
 */
typedef enum {
  C, D, U, F
} status_t;


static void spawn(const char **params)  __attribute__ ((unused));
static void set_status(char *str);
static void open_display(void)          __attribute__ ((unused));
static void close_display()             __attribute__ ((unused));
static void get_datetime(char *buf);
static status_t get_status();
static int read_int(const char *path);
static void read_str(const char *path, char *buf, size_t sz);

static Display *dpy;

int
main(void)
{
  int   timer = 0;
  float bat;                /* battery status */
  char  lnk[STR] = { 0 };   /* wifi link      */
  char  la[STR] = { 0 };    /* load average   */
  char  dt[STR] = { 0 };    /* date/time      */
  char  stat[STR] = { 0 };  /* full string    */
  status_t st;              /* battery status */
  char  status[] = { '+', '-', '?', '=' };  /* should be the same order as the enum above (C, D, U, F) */

#ifndef DEBUG
  open_display();
#endif

  while (!sleep(1)) {
    read_str(LA_PATH, la, LABUF);           /* load average */
    read_str(LNK_PATH, lnk, LNKBUF);        /* link status */
    get_datetime(dt);                       /* date/time */
    bat = ((float)read_int(BAT_NOW) /
           read_int(BAT_FULL)) * 100.0f;    /* battery */
    st = get_status();                      /* battery status (charging/discharging/full/etc) */

    if (st == D && bat < THRESHOLD) {
      snprintf(stat, STR, "LOW BATTERY: suspending after %d ", TIMEOUT - timer);
      set_status(stat);
      if (timer >= TIMEOUT) {
#ifndef DEBUG
        spawn((const char*[])SUSPEND);
#else
        puts("sleeping");
#endif
        timer = 0;
      } else
        timer++;
    } else {
      snprintf(stat, STR, "%s | %s | %c%0.1f%% | %s", la, lnk, status[st], (bat > 100) ? 100 : bat, dt);
      set_status(stat);
      timer = 0;  /* reseting the standby timer */
    }
  }

#ifndef DEBUG
  close_display();
#endif
  return 0; 
}

static void
spawn(const char **params) {
  if (fork() == 0) {
    setsid();
    execv(params[0], (char**)params);
    exit(0);
  }
}

static void
set_status(char *str)
{
#ifndef DEBUG
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
#else
  puts(str);
#endif
}

static void
open_display(void)
{
  if (!(dpy = XOpenDisplay(NULL))) 
    exit(1);
  signal(SIGINT, close_display);
  signal(SIGTERM, close_display);
}

static void
close_display()
{
  XCloseDisplay(dpy);
  exit(0);
}

static void
get_datetime(char *buf)
{
  time_t rawtime;
  time(&rawtime);
  snprintf(buf, DTBUF, "%s", ctime(&rawtime));
}

static status_t
get_status()
{
  FILE *bs;
  char st;

  if ((bs = fopen(BAT_STAT, "r")) == NULL)
    return U;

  st = fgetc(bs);
  fclose(bs);

<<<<<<< HEAD
  switch(st) {
    case 'c': return C;     /* Charging */
    case 'i': return F;     /* Idle */
    case 'd': return D;     /* Discharging */
=======
  switch(tolower(st)) {
    case 'c': return C;     /* Charging */
    case 'd': return D;     /* Discharging */
    case 'i':               /* Idle - fall through */
>>>>>>> upstream/master
    case 'f': return F;     /* Full */
    default : return U;     /* Unknown */
  }
}

static int
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

static void
read_str(const char *path, char *buf, size_t sz)
{
  FILE *fh;
  char ch = 0;
  int idx = 0;

  if (!(fh = fopen(path, "r"))) return;

  while ((ch = fgetc(fh)) != EOF && ch != '\0' && ch != '\n' && idx < sz) {
    buf[idx++] = ch;
  }

  buf[idx] = '\0';
  fclose(fh);
}

/* vim: ts=4 sts=8 sw=4 smarttab et si tw=80 ci cino+=t0(0:0 fo=crtocl list */

