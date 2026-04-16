##
##  Makefile
##

SRC_DIR = src
INC_DIR = include

CC      = gcc
CFLAGS  =  -fPIC -Wall -W -pipe -std=gnu99 -O3 -march=native -mtune=native -ffast-math -funroll-loops
DBFLAGS = -Wall -W -pipe -std=gnu99 -g -O0

all: clean main

debug: clean
	$(CC) $(DBFLAGS) -o main $(SRC_DIR)/*.c -I$(INC_DIR) -lm

main : 
	$(CC) $(CFLAGS) -o main $(SRC_DIR)/*.c -I$(INC_DIR) -lm

clean: 
	rm -rRf main