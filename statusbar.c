/*  statusbar.c  */
/*  Copyright (C) 2012 Alex Kozadaev [akozadaev at yahoo com]  */

#include "build_host.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>


#define ERR_PREFIX  "!!ERROR: "

#define LABUF     15
#define DTBUF     20
#define FULLSTR   52

void xerror(const char *msg, ...);
void set_status(char *str);
void get_load_avg(char *buf);
float get_battery(void);
void get_datetime(char *buf);
int get_wifi(void);
int read_int(const char *path);
void read_str(const char *path, char *buf, size_t sz); 

static Display *dpy;

int
main(void) {
  float bat;                 /* battery status */
  unsigned int lnk;          /* wifi link      */
  char la[LABUF] = "\0";     /* load average   */
  char dt[DTBUF] = "\0";     /* date/time      */
  char full[FULLSTR] = "\0"; /* full string    */


  if (!(dpy = XOpenDisplay(NULL))) {
    xerror("Cannot open display.\n");
    return 1;
  }

  for (;;sleep(1)) {
    get_load_avg(la);
    lnk = get_wifi();
    bat = get_battery();
    get_datetime(dt);

    snprintf(full, FULLSTR, "%s | %d | %0.1f%% | %s", la, lnk, bat, dt);
    set_status(full);
  }
  
  XCloseDisplay(dpy);
  
  return 0; 
}

void
set_status(char *str) {
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
}

void
xerror(const char *msg, ...) {
  va_list ap;

  fprintf(stderr, "%s", ERR_PREFIX); 

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
}

void
get_load_avg(char *buf) {
  read_str(LA_PATH, buf, LABUF);
}

float
get_battery(void) {
  int now, full;

  now = read_int(BAT_NOW);
  full = read_int(BAT_FULL);
  
  return (now * 100) / full;
}

void
get_datetime(char *buf) {
  time_t rawtime;

  time(&rawtime);
  snprintf(buf, DTBUF, "%s", ctime(&rawtime));
  buf[DTBUF-1] = '\0';
}

int
get_wifi(void) {
  int wifi = read_int(LNK_PATH);
  return wifi;
}

int
read_int(const char *path) {
  FILE *fh;
  int i = 0;

  fh = fopen(path, "r");
  if (fh == NULL) {
    xerror("Cannot open %s for reading.\n", path);
    return -1;
  }
  if (fscanf(fh, "%d", &i) < 0) {
    xerror("Cannot read from %s\n", path);
  }

  fclose(fh);
  return i;
}

void
read_str(const char *path, char *buf, size_t sz) {
  FILE *fh;
  size_t rcv;

  fh = fopen(path, "r");
  if (fh == NULL) {
    xerror("Cannot open %s for reading.\n", path);
    return;
  }

  rcv = fread(buf, 1, sz, fh);
  if (ferror(fh) != 0) {
    xerror("Cannot read from %s.\n", path);
    clearerr(fh);
  }
  fclose(fh);

  buf[rcv-1] = '\0';
}

/*  EOF  */

