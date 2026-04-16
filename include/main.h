#ifndef MAIN_H
#define MAIN_H
#include "../include/genAlgo.h"
#include "../include/combination.h"
#include "../include/display.h"

#include <sys/random.h>
#include <time.h>
#include <unistd.h>



void combination(const int X, const  int Y, const int Z, const double basePower, const double baseHeat);

void plannerGA(const int X, const  int Y, const int Z, const double basePower, const double baseHeat);

#endif