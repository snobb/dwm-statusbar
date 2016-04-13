TARGET = statusbar
SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}
HDR = ${wildcard *.h}
ERR = $(shell which clang >/dev/null; echo $$?)
ifeq "$(ERR)" "0"
     CC = clang
else
     CC = gcc
endif

REVCNT = $(shell git rev-list --count master 2>/dev/null)
REVHASH = $(shell git log -1 --format="%h" 2>/dev/null)
ifeq (${REVCNT},)
	VERSION = devel
else
	VERSION = "${REVCNT}.${REVHASH}"
endif

CFLAGS = -Wall $(shell pkg-config --cflags alsa)
LFLAGS = $(shell pkg-config --libs alsa)
INSTALL = install
INSTALL_ARGS = -o root -g root -m 755
INSTALL_DIR = /usr/local/bin/

ifeq (${CC}, $(filter ${CC}, cc gcc clang))
CFLAGS += -std=c99 -pedantic
endif

# autoconfiguration
BATPATH=`find /sys -name BAT0 -print0 -quit`
LNKPATH=`find /sys/class/net/wlan0/ -name operstate -print0 -quit`

all: debug

debug: CFLAGS += -g -DDEBUG
debug: LFLAGS += -g
debug: build

release: CFLAGS += -O3
release: LFLAGS += -lX11
release: clean build

build: build_host.h ${TARGET}

build_host.h:
	@echo "#define BUILD_HOST \"`hostname`\""             > build_host.h
	@echo "#define BUILD_OS \"`uname`\""                 >> build_host.h
	@echo "#define BUILD_PLATFORM \"`uname -m`\""        >> build_host.h
	@echo "#define BUILD_KERNEL \"`uname -r`\""          >> build_host.h
	@echo "#define BUILD_VERSION \"${VERSION}\""         >> build_host.h
	@echo "#define BAT_NOW \"${BATPATH}/charge_now\""    >> build_host.h
	@echo "#define BAT_FULL \"${BATPATH}/charge_full\""  >> build_host.h
	@echo "#define BAT_STAT \"${BATPATH}/status\""       >> build_host.h
	@echo "#define LNK_PATH \"${LNKPATH}\""              >> build_host.h

install: release
	${INSTALL} ${INSTALL_ARGS} ${TARGET} ${INSTALL_DIR}
	@echo "DONE"

${TARGET}: build_host.h ${OBJ}
	${CC} -o $@ ${OBJ} ${LFLAGS}

%.o: %.c
	${CC} ${CFLAGS} -c $?

clean:
	-rm -f build_host.h
	-rm -f *.o ${TARGET}

.PHONY: all debug release build install clean
