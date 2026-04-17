#ifndef COMBINATION_H
#define COMBINATION_H
#include "common.h"

int checkWholeMatrix(reactor_t *r, args_t *args);

int incrementMatrix(blockType_t *matrix, size_t total_size);

void generateMatrices(listHead_t *lHead, args_t *args);

#endif