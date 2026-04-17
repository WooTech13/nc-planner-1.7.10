#ifndef GENALGO_H
#define GENALGO_H
#include "common.h"
#include <math.h>



#define MUTATION_RATE 0.2
#define CROSSOVER_RATE 0.8
#define ELITE_RATIO 0.1
#define TOURNAMENT_SIZE_RATIO 0.1

bool getSym(char dim);

void setSymBlock(blockType_t *matrix, int x, int y, int z, int val, args_t *args);

void setFitness(reactor_t *r, args_t *args);

void fineTuneReactor(reactor_t *r, args_t *args);

void fineTunePopulation(listHead_t *population, args_t *args);

reactor_t* initializeReactor(args_t *args);

void initializePopulation(listHead_t *population, args_t *args);

double calculateDiversity(listHead_t *population);

double calculateAdaptiveMutationRate(double baseMutationRate, double diversity, double maxDiversity);

void mutate(reactor_t *r, double diversity, args_t *args);

void crossover(blockType_t *parent1, blockType_t *parent2, blockType_t *newMatrix1, blockType_t *newMatrix2, args_t *args);

reactor_t* getBestReactorsFromGen(listHead_t *population, listHead_t *newPopulation, int bestCount, args_t *args);

reactor_t* selectParentRoulette(listHead_t *population);

reactor_t* selectParentTournament(listHead_t *population, int tournamentSize, args_t *args);

void runGenAlgo(listHead_t *population, args_t *args);

#endif