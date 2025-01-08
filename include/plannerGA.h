#ifndef PLANNERGA_H
#define PLANNERGA_H
#include "../include/common.h"
#include <math.h>



#define MUTATION_RATE 0.5
#define CROSSOVER_RATE 0.8
#define ELITE_RATIO 0.01
#define TOURNAMENT_SIZE_RATIO 0.03

void flush();

bool getSym(char dim);

void setSymBlock(uint8_t *matrix, int x, int y, int z, int val, args_t *args);

void setFitness(reactor_t *r);

reactor_t* initializeReactor(args_t *args);

reactor_t* initializeReactorFromMatrix(uint8_t *newMatrix, args_t *args);

void initializePopulation(listHead_t *population, args_t *args);

double calculateDiversity(listHead_t *population);

double calculateAdaptiveMutationRate(double baseMutationRate, double diversity, double maxDiversity);

void mutate(reactor_t *r, double diversity, args_t *args);

void crossover(uint8_t *parent1, uint8_t *parent2, uint8_t *newMatrix1, uint8_t *newMatrix2, args_t *args);

reactor_t* selectParentRoulette(listHead_t *population);

reactor_t* selectParentTournament(listHead_t *population, int tournamentSize, args_t *args);

listHead_t* runGA(listHead_t *population, args_t *args);

#endif