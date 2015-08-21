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

#define MIN(a, b) ((a) > (b) ? (b) : (a))

#define THRESHOLD 8
#define TIMEOUT   40
#define SUSPEND   { "/bin/sh", "/usr/local/bin/suspend.sh", NULL }

#define INTBUF    64
#define LABUF     14
#define DTBUF     20
#define LNKBUF    64
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


static void open_display(void)          __attribute__ ((unused));
static void close_display()             __attribute__ ((unused));
static void spawn(const char **params)  __attribute__ ((unused));
static void set_status(char *str);
static void get_datetime(char *dstbuf);
static status_t get_status();
static int read_int(const char *path);
static void read_str(const char *path, char *buf, size_t sz);

static Display *dpy;

int main(int argc, char **argv)
{
    int   timer = 0;
    float bat;                /* battery status */
    char  lnk[STR] = { 0 };   /* wifi link      */
    char  la[STR] = { 0 };    /* load average   */
    char  dt[STR] = { 0 };    /* date/time      */
    char  stat[STR] = { 0 };  /* full string    */
    status_t st;              /* battery status */
    /* should be the same order as the enum above (C, D, U, F) */
    char  status[] = { '+', '-', '?', '=' };

    if (argc > 1 && strcmp(argv[1], "-v") == 0) {
        printf("dwm-statusbar v%s"
#ifdef DEBUG
               " (debug)"
#endif
               " [%s %s]\n\nUsage: %s [-v]\n\n",
                BUILD_VERSION, BUILD_OS, BUILD_KERNEL, argv[0]);
        exit(0);
    }

#ifndef DEBUG
    open_display();
#endif

    while (!sleep(1)) {
        read_str(LA_PATH, la, LABUF);           /* load average */
        read_str(LNK_PATH, lnk, LNKBUF);        /* link status */
        get_datetime(dt);                       /* date/time */
        bat = ((float)read_int(BAT_NOW) /
               read_int(BAT_FULL)) * 100.0f;    /* battery */
        /* battery status (charging/discharging/full/etc) */
        st = get_status();

        if (st == D && bat < THRESHOLD) {
            snprintf(stat, STR, "LOW BATTERY: suspending after %d ",
                     TIMEOUT - timer);
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
            snprintf(stat, STR, "%s | %s | %c%0.1f%% | %s",
                     la,
                     lnk,
                     status[st],
                     MIN(bat, 100), dt);
            set_status(stat);
            timer = 0;  /* reseting the standby timer */
        }
    }

#ifndef DEBUG
    close_display();
#endif
    return 0;
}

static void open_display(void)
{
    if (!(dpy = XOpenDisplay(NULL)))
        exit(1);
    signal(SIGINT, close_display);
    signal(SIGTERM, close_display);
}

static void close_display()
{
    XCloseDisplay(dpy);
    exit(0);
}

static void spawn(const char **params) {
    if (fork() == 0) {
        setsid();
        execv(params[0], (char**)params);
        exit(0);
    }
}

static void set_status(char *str)
{
#ifndef DEBUG
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
#else
    puts(str);
#endif
}

static void get_datetime(char *dstbuf)
{
    time_t rawtime;
    time(&rawtime);
    snprintf(dstbuf, DTBUF, "%s", ctime(&rawtime));
}

static status_t get_status()
{
    FILE *bs;
    char st;

    if ((bs = fopen(BAT_STAT, "r")) == NULL) {
        return U;
    }

    st = fgetc(bs);
    fclose(bs);

    switch(tolower(st)) {
        case 'c': return C;     /* Charging */
        case 'd': return D;     /* Discharging */
        case 'i':               /* Idle - fall through */
        case 'f': return F;     /* Full */
        default : return U;     /* Unknown */
    }
}

static int read_int(const char *path)
{
    int i = 0;
    char buf[INTBUF] = { 0 };

    read_str(path, buf, INTBUF);
    i = atoi(buf);
    return i;
}

static void read_str(const char *path, char *buf, size_t sz)
{
    FILE *fh;
    char ch = 0;
    int idx = 0;

    if (!(fh = fopen(path, "r"))) return;

    while ((ch = fgetc(fh)) != EOF &&
            ch != '\0' && ch != '\n' &&
            idx < sz) {
        buf[idx++] = ch;
    }

    buf[idx] = '\0';
    fclose(fh);
}

/* vim: set ts=4 sts=8 sw=4 smarttab et si tw=80 cino=t0l1(0k2s fo=crtocl */
