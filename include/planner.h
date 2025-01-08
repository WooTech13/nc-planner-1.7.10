#ifndef PLANNER_H
#define PLANNER_H
#include "../include/common.h"



int checkWholeMatrix(reactor_t *r, args_t *args);

int incrementMatrix(uint8_t *matrix, size_t total_size);

void generateMatrices(listHead_t *lHead, args_t *args);

#endif