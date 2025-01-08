#ifndef MAIN_H
#define MAIN_H
#include "../include/plannerGA.h"
#include "../include/planner.h"
#include "../include/display.h"

#include <sys/random.h>
#include <time.h>
#include <unistd.h>



void planner(const int X, const  int Y, const int Z);

void plannerGA(const int X, const  int Y, const int Z);

#endif