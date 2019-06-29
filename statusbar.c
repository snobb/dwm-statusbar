/*
 *  statusbar.c
 *  @author Alex Kozadaev (2015)
 */

#include "build_host.h"

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>

#ifndef DEBUG
#include <X11/Xlib.h>
#endif

#define MIN(a, b) ((a) > (b) ? (b) : (a))

#define BUFSZ  64
#define STATUSSZ  255

/* Available statuses
 *
 *  Charging
 *  Draining
 *  Unknown
 *  Full
 */
typedef enum {
    CHARGING, DRAINING, UNKNOWN, FULL
} battery_t;

static void open_display();
static void close_display();
static void set_status(char *str);
static const char *get_progress(int p);
static void get_load_average(char *dstla);
static void get_datetime(char *dstbuf);
static int get_volume(void);
static void read_str(const char *path, char *buf, size_t sz);

#ifndef DEBUG
static Display *dpy;
#endif

#ifdef BAT_STAT
/* If battery exists - show the status and enable low battery action */
const int THRESHOLD = 8;
const int TIMEOUT = 40;
const char *SUSPEND[] = { "/bin/sh", "/usr/local/bin/suspend.sh", NULL };
/* should be the same order as the battery_t enum */
const char CHARGE[] = { '+', '-', '?', '=' };

static battery_t get_battery(void);
static int read_int(const char *path);
static void spawn(const char **params);
#endif

int
main(int argc, char **argv)
{
    int   vol = 0;

#ifdef BAT_STAT
    int   timer = 0;
    float charge;      /* battery charge */
    battery_t bstat;   /* battery status */
#endif

    char  lnk[BUFSZ] = { 0 };   /* wifi link      */
    char  la[BUFSZ] = { 0 };    /* load average   */
    char  dt[BUFSZ] = { 0 };    /* date/time      */
    char  stat[BUFSZ] = { 0 };  /* full string    */

    if (argc > 1 && strcmp(argv[1], "-v") == 0) {
        printf("dwm-statusbar v%s"
#ifdef DEBUG
               " (debug)"
#endif
               " [%s %s]\n\nUsage: %s [-v]\n\n",
                BUILD_VERSION, BUILD_OS, BUILD_KERNEL, argv[0]);
        exit(0);
    }

    open_display();

    while (!sleep(1)) {
        vol = get_volume();
        get_load_average(la);
        read_str(LNK_PATH, lnk, BUFSZ);         /* link status */
        get_datetime(dt);                       /* date/time */

#ifdef BAT_STAT
        charge = ((float)read_int(BAT_NOW) /
               read_int(BAT_FULL)) * 100.0f;    /* battery charge percent */

        /* battery status (charging/discharging/full/etc) */
        bstat = get_battery();

        if (bstat == DRAINING && charge < THRESHOLD) {
            snprintf(stat, STATUSSZ, "LOW BATTERY: suspending after %d ",
                     TIMEOUT - timer);


            if (timer >= TIMEOUT) {
                spawn(SUSPEND);
                timer = 0;
            } else {
                timer++;
            }
        } else {
            snprintf(stat, STATUSSZ, "%s | vol:%s | %s | %c%0.1f%% | %s", la,
                    get_progress(vol), lnk, CHARGE[bstat], MIN(charge, 100), dt);
            timer = 0;  /* reseting the standby timer */
        }
#else
        snprintf(stat, STATUSSZ, "%s | vol:%s | %s | %s", la,
                get_progress(vol), lnk, dt);
#endif

        set_status(stat);
    }

    close_display();

    return 0;
}

void
open_display()
{
#ifndef DEBUG
    if (!(dpy = XOpenDisplay(NULL)))
        exit(1);
#endif
    signal(SIGINT, close_display);
    signal(SIGTERM, close_display);
}

void
close_display()
{
#ifndef DEBUG
    XCloseDisplay(dpy);
#endif
    exit(0);
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

const char*
get_progress(int p) {
    const char *s[] = {
        "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"
    };
    return s[(p * 7) / 100];
}

void
get_load_average(char *dstla)
{
    double avgs[3];

    if (getloadavg(avgs, 3) < 0) {
        set_status("getloadavg");
        exit(1);
    }

    sprintf(dstla, "%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

void
get_datetime(char *dstbuf)
{
    time_t rawtime = time(NULL);
    strftime(dstbuf, BUFSZ, "%a %b %d %H:%M:%S", localtime(&rawtime));
}

int
get_volume(void)
{
    long min, max, volume = 0;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_get_playback_volume(elem, 0, &volume);
    snd_mixer_close(handle);

    return ((double)volume / max) * 100;
}

void
read_str(const char *path, char *buf, size_t sz)
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

#ifdef BAT_STAT

battery_t
get_battery(void)
{
    FILE *bs;
    char st;

    if ((bs = fopen(BAT_STAT, "r")) == NULL) {
        return UNKNOWN;
    }

    st = fgetc(bs);
    fclose(bs);

    switch(tolower(st)) {
        case 'c': return CHARGING;
        case 'd': return DRAINING;
        case 'i': /* Idle - fall through */
        case 'f': return FULL;
        default : return UNKNOWN;
    }
}

int
read_int(const char *path)
{
    int i = 0;
    char buf[BUFSZ] = { 0 };

    read_str(path, buf, BUFSZ);
    i = atoi(buf);
    return i;
}

void
spawn(const char **params) {
#ifndef DEBUG
    if (fork() == 0) {
        setsid();
        execv(params[0], (char**)params);
        exit(0);
    }
#else
    printf("spawning command %s %s\n", params[0], params[1]);
#endif
}

#endif

