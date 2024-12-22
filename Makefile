##
##  Makefile
##

CFLAGS += -Wall -W -pipe -std=gnu99 -O3 -march=native -mtune=native -ffast-math -funroll-loops 
CC ?= gcc
DBFLAGS += -g -O0
SRC_DIR = src
BIN_DIR = bin

all: clean planner planner-genAlgo

clean:
	rm -f bin/*

planner:
	mkdir -p $(BIN_DIR)
	${CC} ${CFLAGS} -o $(BIN_DIR)/planner.bin $(SRC_DIR)/planner.c

planner-genAlgo:
	mkdir -p $(BIN_DIR)
	${CC} ${CFLAGS} -lm -o $(BIN_DIR)/planner-genAlgo.bin $(SRC_DIR)/planner-genAlgo.c