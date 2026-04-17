#ifndef MAIN_H
#define MAIN_H
#include "advGenAlgo.h"
#include "genAlgo.h"
#include "combination.h"
#include "display.h"
#include "common.h"

#include <sys/random.h>
#include <time.h>
#include <unistd.h>

void combination(const int X, const  int Y, const int Z, const double basePower, const double baseHeat);

void genAlgo(const int X, const  int Y, const int Z, const double basePower, const double baseHeat);

void advGenAlgo();

#endif