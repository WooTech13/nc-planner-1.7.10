#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>



#define OFFSET(x, y, z, Y, Z) ((x) * (Y * Z) + (y) * Z + (z))

int GX = 0, GY, GZ;
int GSIZE;
double BASE_POWER, BASE_HEAT;
int SYM_X = 0, SYM_Y = 0, SYM_Z = 0;

#define MAX_VALUE REDSTONE

typedef enum {
    FUEL_CELL = 0,
    GRAPHITE,
    GELID_CRYOTHEUM,
    ENDERIUM,
    GLOWSTONE,
    LIQUID_HELIUM,
    REDSTONE,
    NUM_BLOCK_TYPES
} blockType_t;

const char *BLOCK_NAMES[] = {
    "FUEL_CELL", "GRAPHITE", "GELID_CRYOTHEUM",
    "ENDERIUM", "GLOWSTONE", "LIQUID_HELIUM", "REDSTONE"
};

typedef struct {
    const int X;
    const int Y;
    const int Z;
    const int SIZE;
    const double energy;
    const double baseHeat;
    const size_t populationSize;
    const size_t genMax;
    const bool xSym;
    const bool ySym;
    const bool zSym;
} args_t;

typedef struct {
    int x, y, z;
    blockType_t *matrix;
    double energy;
    double heat;
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



blockType_t* setMatrix(args_t *args);

void freeList(listHead_t *head);

int listLen(listHead_t *head);

reactor_t* getListItemFromIndex(listHead_t *head, int index);

reactor_t* popLastFromList(listHead_t *head);

reactor_t* popListFromValue(listHead_t *head, reactor_t *r);

void addToList(listHead_t *lHeadPtr, reactor_t *r);

bool isInList(listHead_t *lHeadPtr, listItem_t *item);

int getAdjacentBlock(blockType_t *matrix, int x, int y, int z, blockType_t type, args_t *args);

void calculatePowerHeat(reactor_t *r, args_t *args);

void copyMatrix(blockType_t *matrixSrc, blockType_t *matrixDst, args_t *args);

reactor_t* getBestReactor(listHead_t *lHeadPtr);

#endif