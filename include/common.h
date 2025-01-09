#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_VALUE HEL

#define OFFSET(x, y, z, Y, Z) ((x) * (Y * Z) + (y) * Z + (z))

typedef enum {
    RED,
    CRYO,
    CELL,
    HEL,
    DEF,
} blockType_t;


typedef struct {
    const int X;
    const int Y;
    const int Z;
    const size_t populationSize;
    const size_t genMax;
    const bool xSym;
    const bool ySym;
    const bool zSym;
} args_t;

typedef struct {
    uint8_t *matrix;
    double basePower;
    double baseHeat;
    double totalPower;
    double totalHeat;
    double malus;
    double fitness;
} reactor_t ;

typedef struct listItem{
    reactor_t *r;
    struct listItem *next;
} listItem_t;

typedef struct {
    listItem_t *head;
} listHead_t;



uint8_t* setMatrix(args_t *args);

void freeList(listHead_t *head);

int listLen(listHead_t *head);

reactor_t* getListItemFromIndex(listHead_t *head, int index);

reactor_t* popListFromValue(listHead_t *head, reactor_t *r);

void addToList(listHead_t *lHeadPtr, reactor_t *r);

int getAdjacentBlock(unsigned char *matrix, int x, int y, int z, blockType_t type, args_t *args);

void calculatePowerHeat(reactor_t *r, args_t *args);

void copyMatrix(uint8_t *matrixSrc, uint8_t *matrixDst, args_t *args);

reactor_t* getBestReactor(listHead_t *lHeadPtr);

#endif