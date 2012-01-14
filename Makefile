TARGET = statusbar
SRC = ${wildcard *.c}
OBJ = ${SRC:.c=.o}
HDR = ${wildcard *.h}
CC = cc
CFLAGS = -Wall -std=c99 -pedantic
LFLAGS = -lX11
STRIP = strip
INSTALL = install
INSTALL_ARGS = -o root -g wheel -m 755 
INSTALL_DIR = /usr/local/bin/

# autoconfiguration
BATPATH=`find /sys -name BAT0 -print0 -quit`
LNKPATH=`find /sys -name link -print0 -quit`
LAPATH=`find /proc -name loadavg -print0 -quit`

all: debug

debug: CFLAGS += -g -O2 -DDEBUG
debug: LFLAGS += -g
debug: build

release: CFLAGS += -Os
release: LFLAGS += -s
release: clean build
	${STRIP} ${TARGET}

build: build_host.h ${TARGET}

build_host.h:
	@echo "#define BUILD_HOST \"`hostname`\""  > build_host.h
	@echo "#define BUILD_OS \"`uname`\""          >> build_host.h
	@echo "#define BUILD_PLATFORM \"`uname -m`\"" >> build_host.h
	@echo "#define BUILD_KERNEL \"`uname -r`\""   >> build_host.h
	@echo "#define LA_PATH \"${LAPATH}\""  >> build_host.h
	@echo "#define BAT_NOW \"${BATPATH}/charge_now\""  >> build_host.h
	@echo "#define BAT_FULL \"${BATPATH}/charge_full\""  >> build_host.h
	@echo "#define LNK_PATH \"${LNKPATH}\"" >> build_host.h

install: release
	${INSTALL} ${INSTALL_ARGS} ${TARGET} ${INSTALL_DIR}
	@echo "DONE"

${TARGET}: build_host.h ${OBJ}
	${CC} ${LFLAGS} -o $@ ${OBJ}	

clean:
	-rm -f build_host.h 
	-rm -f *.o ${TARGET} 

.PHONY : all debug release build install clean 
