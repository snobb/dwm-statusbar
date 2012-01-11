/*  statusbar.c  */
/*  Copyright (C) 2012 Alex Kozadaev [akozadaev at yahoo com]  */

#include "build_host.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>


#define ERR_PREFIX    "!! "

#define LABUF     15
#define LAPATH   "/proc/loadavg"

#define INTBUF    10
#define DTBUF     20
#define FULLSTR   60

void xerror(const char *msg, ...);
void setstatus(char *str);
void get_load_avg(char *buf);
float get_battery(void);
void get_datetime(char *buf);
unsigned int get_wifi(void);
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
//    printf("%s", full);
    setstatus(full);
  }
  
  XCloseDisplay(dpy);
  
  return 0; 
}

void
setstatus(char *str) {
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
}

void
xerror(const char *msg, ...) {
  va_list ap;

  fprintf(stderr, "%sERROR: ", ERR_PREFIX); 

  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
}

void
get_load_avg(char *buf) {
  read_str(LAPATH, buf, LABUF);
}

float
get_battery(void) {
  char now[INTBUF] = "\0";
  char full[INTBUF] = "\0";
  unsigned int nowi, fulli;
  
  read_str(BAT_NOW, now, INTBUF);
  read_str(BAT_FULL, full, INTBUF);
  nowi = atoi(now);
  fulli = atoi(full);
  return (nowi / fulli) * 100;
}

void
get_datetime(char *buf) {
  time_t rawtime;

  time(&rawtime);
  snprintf(buf, DTBUF, "%s", ctime(&rawtime));
  buf[DTBUF-1] = '\0';
}

unsigned int
get_wifi(void) {
  char wifi[INTBUF] = "\0";
  
  read_str(LNK_PATH, wifi, INTBUF);
  return atoi(wifi);
}

void
read_str(const char *path, char *buf, size_t sz) {
  FILE *fh;

  fh = fopen(path, "r");
  if (fh == NULL) {
    xerror("Cannot open %s for reading.\n", path);
    return;
  }

  size_t loaded = fread(buf, 1, sz, fh);
  if (ferror(fh) != 0) {
    xerror("Cannot read from %s.\n", path);
    clearerr(fh);
  }
  fclose(fh);

  if (loaded < sz) 
    buf[loaded-1] = '\0';
  else
    buf[sz-1] = '\0';
}

/*  EOF  */

