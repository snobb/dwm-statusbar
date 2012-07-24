/*  statusbar.c  */
/*  Copyright (C) 2012 Alex Kozadaev [akozadaev at yahoo com]  */

#include "build_host.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <X11/Xlib.h>

/* version 0.61 */

/* Available statuses 
 *  
 *  Charging
 *  Discharging
 *  Full
 *  Unknown
 */
#define CHARGE      "Charging"
#define DISCHARGE   "Discharging"
#define FULL        "Full"

#define THRESHOLD 10
#define TIMEOUT   40
#define SUSPEND   { BOX_SUSPEND, NULL }     /* BOX_SUSPEND gets configured in Makefile */

#define LABUF     15
#define DTBUF     20
#define STR       60

/* Charging, Discharging, Unknown, Full (order matters) */
typedef enum { 
  C, D, U, F
} status_t;


void spawn(const char **params);
void set_status(char *str);
void open_display(void);
void close_display();
void get_datetime(char *buf);
status_t get_status();
int read_int(const char *path);
void read_str(const char *path, char *buf, size_t sz);

static Display *dpy;

int
main(void)
{
  int   timer = 0;
  float bat;                /* battery status */
  int   lnk;                /* wifi link      */
  char  la[LABUF] = { 0 };  /* load average   */
  char  dt[DTBUF] = { 0 };  /* date/time      */
  char  stat[STR] = { 0 };  /* full string    */
  status_t st;              /* battery status */
  char  status[] = { '+', '-', '?', '=' };  /* should be the same order as the enum above (C, D, U, F) */

#ifndef DEBUG
  open_display();
#endif

  while (!sleep(1)) {
    read_str(LA_PATH, la, LABUF);           /* load average */
    lnk = read_int(LNK_PATH);               /* link status */
    get_datetime(dt);                       /* date/time */
    bat = ((float)read_int(BAT_NOW) / 
           read_int(BAT_FULL)) * 100.0f;    /* battery */
    st = get_status();                      /* battery status (charging/discharging/full/etc) */

    if (st == D && bat < THRESHOLD) {
      snprintf(stat, STR, "LOW BATTERY: suspending after %d ", TIMEOUT - timer);
      set_status(stat);
      if (timer > TIMEOUT) {
#ifndef DEBUG
          spawn((const char*[])SUSPEND);
#else
          puts("sleeping");
#endif
        timer = 0;
      } else
        timer++;
    } else {
      snprintf(stat, STR, "%s | %d | %c%0.1f%% | %s", la, lnk, status[st], (bat > 100) ? 100 : bat, dt);
      set_status(stat);
      timer = 0;  /* reseting the standby timer */
    }
  }

#ifndef DEBUG
  close_display();
#endif
  return 0; 
}

void
spawn(const char **params) {
  if (fork() == 0) {
    setsid();
    execv(params[0], (char**)params);
    exit(0);
  }
}

void
set_status(char *str)
{
#ifndef DEBUG
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
#else
  puts(str);
#endif
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

status_t
get_status()
{
  char stat[50] = {0};
  read_str(BAT_STAT, stat, 50);

  if (strncmp(stat, CHARGE, strlen(CHARGE)) == 0)
    return C;
  else if (strncmp(stat, DISCHARGE, strlen(DISCHARGE)) == 0)
    return D;
  else if (strncmp(stat, FULL, strlen(FULL)) == 0)
    return F;
  else
    return U;
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

