##
##  Makefile
##

CFLAGS += -Wall -W -pipe -O2 -std=gnu99
CC ?= gcc
SRC_DIR = src
BIN_DIR = bin

run: all exec

all: clean planner

clean:
	rm -f bin/*

planner:
	mkdir -p $(BIN_DIR)
	${CC} ${CFLAGS} -o $(BIN_DIR)/planner.bin $(SRC_DIR)/planner.c

exec:
	bin/planner.bin