TARGET = statusbar
SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}
HDR = ${wildcard *.h}
CC ?= cc
CFLAGS = -Wall
LFLAGS = -lX11
INSTALL = install
INSTALL_ARGS = -o root -g wheel -m 755 
INSTALL_DIR = /usr/local/bin/

ifeq (${CC}, cc)
CFLAGS += -std=c99 -pedantic
endif

# autoconfiguration
BATPATH=`find /sys -name BAT0 -print0 -quit`
LNKPATH=`find /sys -name link -print0 -quit`
LAPATH=`find /proc -name loadavg -print0 -quit`
BOXSUSPEND=`which boxsuspend`

all: debug

debug: CFLAGS += -g -DDEBUG
debug: LFLAGS += -g
debug: build

release: CFLAGS += -Os
release: LFLAGS += -s
release: clean build

build: build_host.h ${TARGET}

build_host.h:
	@echo "#define BUILD_HOST \"`hostname`\""  > build_host.h
	@echo "#define BUILD_OS \"`uname`\""          >> build_host.h
	@echo "#define BUILD_PLATFORM \"`uname -m`\"" >> build_host.h
	@echo "#define BUILD_KERNEL \"`uname -r`\""   >> build_host.h
	@echo "#define LA_PATH \"${LAPATH}\""  >> build_host.h
	@echo "#define BAT_NOW \"${BATPATH}/charge_now\""  >> build_host.h
	@echo "#define BAT_FULL \"${BATPATH}/charge_full\""  >> build_host.h
	@echo "#define BAT_STAT \"${BATPATH}/status\""  >> build_host.h
	@echo "#define LNK_PATH \"${LNKPATH}\"" >> build_host.h
	@echo "#define BOX_SUSPEND \"${BOXSUSPEND}\"" >> build_host.h

install: release
	${INSTALL} ${INSTALL_ARGS} ${TARGET} ${INSTALL_DIR}
	@echo "DONE"

${TARGET}: build_host.h ${OBJ}
	${CC} ${LFLAGS} -o $@ ${OBJ}	

%.o : %.c
	${CC} ${CFLAGS} -c $?

clean:
	-rm -f build_host.h 
	-rm -f *.o ${TARGET} 

.PHONY : all debug release build install clean 
