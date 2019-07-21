include commands.mk

OPTS    := -O3
CFLAGS  := -std=c99 $(OPTS)  -I/opt/avif/include -fPIC -Wall
LDFLAGS := $(shell imlib2-config --libs) -L/opt/avif/lib/x86_64-linux-gnu -lavif -lgb -ldav1d -Wl,-rpath=/opt/avif/lib/x86_64-linux-gnu


SRC = $(wildcard *.c)
OBJ = $(foreach obj, $(SRC:.c=.o), $(notdir $(obj)))
DEP = $(SRC:.c=.d)

LIBDIR    ?= $(shell pkg-config --variable=libdir imlib2)
LOADERDIR ?= $(LIBDIR)/imlib2/loaders/

ifndef DISABLE_DEBUG
	CFLAGS += -ggdb
endif

.PHONY: all clean

all: avif.so

avif.so: $(OBJ)
	$(CC) -shared -o $@ $^ $(LDFLAGS) 
	cp $@ $@.debug
	strip $@

%.o: %.c
	$(CC) -Wp,-MMD,$*.d -c $(CFLAGS) -o $@ $<

clean:
	$(RM) $(DEP)
	$(RM) $(OBJ)
	$(RM) avif.so

install:
	$(INSTALL_DIR) $(DESTDIR)$(LOADERDIR)
	$(INSTALL_LIB) avif.so $(DESTDIR)$(LOADERDIR)

uninstall:
	$(RM) $(PLUGINDIR)/avif.so

-include $(DEP)

