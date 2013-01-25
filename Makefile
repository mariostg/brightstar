#/** \file

CC      = gcc
OBJECTS = brightstar.o bright_parse.o
CFLAGS  = -g -Wall -std=gnu99 `pkg-config --cflags glib-2.0` `curl-config --cflags`
LDLIBS  = `pkg-config --libs glib-2.0 ` `curl-config --libs` -lssl -lcrypto

SRC = brightstar.c bright_parse.c
HDR = brightstar.h bright_parse.h
OBJ = $(SRC:.c=.o)

BIN = brightstar

PREFIX?=/usr
BINDIR=${PREFIX}/bin

.PHONY: default all clean install

default: all
all : $(BIN)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -rf $(BIN) $(OBJ)

install: all
	test -d ${DESDIR}${DINDIR} || mkdir -p ${DESDIR}${DINDIR}
	install -m755 ${BIN} ${DESDIR}${BINDIR}/${BIN}
