TARGET          := statusbar
SRC             := $(wildcard *.c)
OBJ             := $(SRC:.c=.o)
BUILD_HOST      := build_host.h
CC              ?= gcc

INSTALL         := install
INSTALL_ARGS    := -o root -g root -m 755
INSTALL_DIR     := /usr/local/bin/

# autoconfiguration
BATPATH         := $(strip $(shell find /sys -name BAT? -print0 -quit))
# Infer the wifi interface name - please override here if necessary
IFNAME          := $(shell iw dev | awk '/Interface/ { print $$2 }' | tr -d '\n')
LNKPATH         := $(shell find /sys/class/net/$(IFNAME)/ -name operstate -print0 -quit)

# version info from git
REVCNT          := $(shell git rev-list --count main 2>/dev/null)
ifeq ($(REVCNT),)
    VERSION     := devel
else
    REVHASH     := $(shell git log -1 --format="%h" 2>/dev/null)
    VERSION     := "$(REVCNT).$(REVHASH)"
endif

CFLAGS          := -Wall $(shell pkg-config --cflags alsa)
LFLAGS          := $(shell pkg-config --libs alsa)
LFLAGS          += $(shell pkg-config --libs x11)

ifeq ($(CC), $(filter $(CC), cc gcc clang))
    CFLAGS      += -std=c99 -pedantic
endif

all: debug

debug: CFLAGS   += -g -ggdb -DDEBUG
debug: LFLAGS   += -g
debug: build

release: CFLAGS += -O3 -I/usr/X11R6/include
release: clean build

build: $(BUILD_HOST) $(TARGET)

$(BUILD_HOST):
	@echo "#define BUILD_HOST \"`hostname`\""             > $(BUILD_HOST)
	@echo "#define BUILD_OS \"`uname`\""                 >> $(BUILD_HOST)
	@echo "#define BUILD_PLATFORM \"`uname -m`\""        >> $(BUILD_HOST)
	@echo "#define BUILD_KERNEL \"`uname -r`\""          >> $(BUILD_HOST)
	@echo "#define BUILD_VERSION \"$(VERSION)\""         >> $(BUILD_HOST)
	@echo "#define LNK_PATH \"$(LNKPATH)\""              >> $(BUILD_HOST)
ifdef BATPATH
	@echo "#define BAT_NOW \"$(BATPATH)/charge_now\""    >> $(BUILD_HOST)
	@echo "#define BAT_FULL \"$(BATPATH)/charge_full\""  >> $(BUILD_HOST)
	@echo "#define BAT_STAT \"$(BATPATH)/status\""       >> $(BUILD_HOST)
endif

$(TARGET): $(BUILD_HOST) $(OBJ)
	$(CC) -o $@ $(OBJ) $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

install: release
	$(INSTALL) $(INSTALL_ARGS) $(TARGET) $(INSTALL_DIR)
	@echo "DONE"

clean:
	-rm -f $(BUILD_HOST)
	-rm -f *.o $(TARGET)

.PHONY: all debug release build install clean
