##
##  Makefile
##

CFLAGS += -Wall -W -pipe -O2 -std=gnu99
CC ?= gcc
DBFLAGS += -g -O0
SRC_DIR = src
BIN_DIR = bin

all: clean planner

debug: clean plannerDebug

clean:
	rm -f bin/*

planner:
	mkdir -p $(BIN_DIR)
	${CC} ${CFLAGS} -o $(BIN_DIR)/planner.bin $(SRC_DIR)/planner.c

plannerDebug:
	mkdir -p $(BIN_DIR)
	${CC} ${DBFLAGS} -o $(BIN_DIR)/planner.bin $(SRC_DIR)/planner.c