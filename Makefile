TARGET          := statusbar
SRC             := $(wildcard *.c)
OBJ             := $(SRC:.c=.o)
BUILD_HOST      := build_host.h
CC              ?= gcc

INSTALL         := install
INSTALL_ARGS    := -o root -g root -m 755
INSTALL_DIR     := /usr/local/bin/

# autoconfiguration
BATPATH         := $(shell find /sys -name BAT0 -print0 -quit)
# Please wlan0 to the wlan interface name if predictable if names are enabled.
LNKPATH         := $(shell find /sys/class/net/wlan0/ -name operstate -print0 -quit)

# version info from git
REVCNT          := $(shell git rev-list --count master 2>/dev/null)
ifeq ($(REVCNT),)
    VERSION     := devel
else
    REVHASH     := $(shell git log -1 --format="%h" 2>/dev/null)
    VERSION     := "$(REVCNT).$(REVHASH)"
endif

INCLUDES        := -I/usr/X11R6/include $(shell pkg-config --cflags alsa)
LIBS            := -L/usr/X11R6/lib -lX11 $(shell pkg-config --libs alsa)

CFLAGS          := -Wall $(INCLUDES)
LFLAGS          := $(LIBS)

ifeq ($(CC), $(filter $(CC), cc gcc clang))
    CFLAGS      += -std=c99 -pedantic
endif

all: debug

debug: CFLAGS += -g -ggdb -DDEBUG
debug: LFLAGS += -g
debug: build

release: CFLAGS += -O3
release: clean build

build: $(BUILD_HOST) $(TARGET)

$(BUILD_HOST):
	@echo "#define BUILD_HOST \"`hostname`\""             > $(BUILD_HOST)
	@echo "#define BUILD_OS \"`uname`\""                 >> $(BUILD_HOST)
	@echo "#define BUILD_PLATFORM \"`uname -m`\""        >> $(BUILD_HOST)
	@echo "#define BUILD_KERNEL \"`uname -r`\""          >> $(BUILD_HOST)
	@echo "#define BUILD_VERSION \"$(VERSION)\""         >> $(BUILD_HOST)
	@echo "#define BAT_NOW \"$(BATPATH)/charge_now\""    >> $(BUILD_HOST)
	@echo "#define BAT_FULL \"$(BATPATH)/charge_full\""  >> $(BUILD_HOST)
	@echo "#define BAT_STAT \"$(BATPATH)/status\""       >> $(BUILD_HOST)
	@echo "#define LNK_PATH \"$(LNKPATH)\""              >> $(BUILD_HOST)


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
