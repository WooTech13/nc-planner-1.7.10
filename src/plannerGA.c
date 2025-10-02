#include "../include/plannerGA.h"

void flush(){
    int c;
    while ( (c = getchar()) != '\n' && c != EOF ) { }
}

bool getSym(char dim){
    char sym;
    while(true){
        printf("%c symetry [Y/n]:",dim);
        sym = getchar();
        if(sym == '\n'){
            printf("Using symetry on %c\n",dim);
            return true;
        } else{
            flush();
            if(sym == 'Y' || sym == 'y'){
                printf("Using symetry on %c\n",dim);
                return true;
            } else if(sym == 'n' || sym == 'N'){
                printf("Not using symetry on %c\n",dim);
                return false;
            } else {
                printf("Error: please enter Y, y, N or n\n");
                flush();
            }
        }
    }
}

void setSymBlock(uint8_t *matrix, int x, int y, int z, int val, args_t *args){
    int xValues[] = {x, args->X - 1 - x};
    int yValues[] = {y, args->Y - 1 - y};
    int zValues[] = {z, args->Z - 1 - z};

    for (int i = 0; i <= args->ySym; i++) {
        for (int j = 0; j <= args->zSym; j++) {
            for (int k = 0; k <= args->xSym; k++) {
                matrix[OFFSET(xValues[k], yValues[i], zValues[j], args->Y, args->Z)] = val;
            }
        }
    }
}

void setFitness(reactor_t *r, args_t *args){
    double totalBlock = args->X * args->Y * args->Z;
    double malusRatio = 1 - (double)(r->malus / totalBlock);
    r->fitness =  (r->totalPower / r->totalHeat) * -1 * malusRatio;
}

void fineTunePopulation(listHead_t *population, args_t *args) {
    listItem_t *item = population->head;
    while(item != NULL) {
        reactor_t *r = item->r;

        for (int y = 0; y < args->Y; y++) {
            for (int z = 0; z < args->Z; z++) {
                for (int x = 0; x < args->X; x++) {
                    uint8_t *current_block = &r->matrix[OFFSET(x, y, z, args->Y, args->Z)];
                    switch (*current_block) {
                        case REDSTONE:
                            if(getAdjacentBlock(r->matrix, x, y, z, FUEL_CELL, args) == 0){
                                if(r->totalHeat >= 0){
                                    if(getAdjacentBlock(r->matrix, x, y, z, GELID_CRYOTHEUM, args) == 0) {
                                        *current_block = GELID_CRYOTHEUM;
                                    } else {
                                        *current_block = LIQUID_HELIUM;
                                    }
                                } else {
                                    *current_block = FUEL_CELL;
                                }
                            }
                            break;
                        case FUEL_CELL:
                            break;
                        case GELID_CRYOTHEUM:
                            if(getAdjacentBlock(r->matrix, x, y, z, GELID_CRYOTHEUM, args) != 0){
                            if(r->totalHeat >= 0){
                                    if(getAdjacentBlock(r->matrix, x, y, z, FUEL_CELL, args) != 0) {
                                        *current_block = REDSTONE;
                                    } else {
                                        *current_block = LIQUID_HELIUM;
                                    }
                                } else {
                                    *current_block = FUEL_CELL;
                                }
                            }
                            break;
                        case LIQUID_HELIUM:
                            if(r->totalHeat >= 0){
                                if(getAdjacentBlock(r->matrix, x, y, z, FUEL_CELL, args) != 0) {
                                    *current_block = REDSTONE;
                                } else if(getAdjacentBlock(r->matrix, x, y, z, GELID_CRYOTHEUM, args) == 0) {
                                    *current_block = GELID_CRYOTHEUM;
                                }
                            } else {
                                *current_block = FUEL_CELL;
                            }
                            break;
                        default:
                            break;
                    }
                    calculatePowerHeat(r, args);
                }
            }
        }
        item = item->next;
    }
}

reactor_t* initializeReactor(args_t *args){
    reactor_t* r = (reactor_t*)malloc(sizeof(reactor_t));
    r->matrix=setMatrix(args);

    int xBound = args->X;
    if(args->xSym){
        if(args->X % 2 != 0){
            xBound = args->X / 2 + 1;
        } else {
            xBound = args->X / 2;
        }
    }

    int yBound = args->Y;
    if(args->ySym){
        if(args->Y % 2 != 0){
            yBound = args->Y / 2 + 1;
        } else {
            yBound = args->Y / 2;
        }
    }

    int zBound = args->Z;
    if(args->zSym){
        if(args->Z % 2 != 0){
            zBound = args->Z / 2 + 1;
        } else {
            zBound = args->Z / 2;
        }
    }

    for(int y = 0; y < yBound ; y++) {
        for(int z = 0; z < zBound ; z++) {
            for(int x = 0; x < xBound; x++) {
                int val = rand() % 4;
                setSymBlock(r->matrix, x, y, z, val, args);
            }
        }
    }

    r->basePower = 150;
    r->baseHeat = 25;

    calculatePowerHeat(r, args);
    setFitness(r, args);
    return r;
}

reactor_t* copyReactor(reactor_t *oldReactor, args_t *args){
    reactor_t* r = (reactor_t*)malloc(sizeof(reactor_t));

    r->matrix = setMatrix(args);
    copyMatrix(oldReactor->matrix, r->matrix, args);
    r->basePower = oldReactor->basePower;
    r->baseHeat = oldReactor->baseHeat;
    r->totalPower = oldReactor->totalPower;
    r->totalHeat = oldReactor->totalHeat;
    r->fitness = oldReactor->fitness;
    r->malus = oldReactor->malus;

    return r;
}

void initializePopulation(listHead_t *population, args_t *args){
    size_t i = 0;
    while(i < args->populationSize){
        reactor_t *r = initializeReactor(args);
        //if(r->fitness > 0) {
            addToList(population, r);
            i++;
        //}        
    }
}

double calculateDiversity(listHead_t *population){
    double sumFitness = 0.0;
    double sumSquaredFitness = 0.0;
    int count = 0;
    listItem_t *item = population->head;

    while(item != NULL) {
        sumFitness += item->r->fitness;
        sumSquaredFitness += item->r->fitness * item->r->fitness;
        count++;
        item = item->next;
    }

    double meanFitness = sumFitness / count;
    double variance = (sumSquaredFitness / count) - (meanFitness * meanFitness);
    return sqrt(variance) / meanFitness; // Coefficient of variation as diversity measure
}

double calculateAdaptiveMutationRate(double baseMutationRate, double diversity, double maxDiversity){
    double normalizedDiversity = diversity / maxDiversity;
    double adaptiveFactor = exp(-normalizedDiversity);
    return baseMutationRate * (1.0 + adaptiveFactor);
}

void mutate(reactor_t *r, double adaptiveMutationRate, args_t *args){
    
    int xBound = args->X;
    if(args->xSym){
        if(args->X % 2 != 0){
            xBound = args->X / 2 + 1;
        } else {
            xBound = args->X / 2;
        }
    }

    int yBound = args->Y;
    if(args->ySym){
        if(args->Y % 2 != 0){
            yBound = args->Y / 2 + 1;
        } else {
            yBound = args->Y / 2;
        }
    }

    int zBound = args->Z;
    if(args->zSym){
        if(args->Z % 2 != 0){
            zBound = args->Z / 2 + 1;
        } else {
            zBound = args->Z / 2;
        }
    }
    
    for(int y = 0; y < yBound ; y++) {
        for(int z = 0; z < zBound ; z++) {
            for(int x = 0; x < xBound; x++) {
                if((rand() / (double)RAND_MAX) < adaptiveMutationRate) {
                    int val = rand() % 4;
                    setSymBlock(r->matrix, x, y, z, val, args);
                }
            }
        }
    }
    calculatePowerHeat(r, args);
    setFitness(r, args);
}

void crossover(uint8_t *parent1, uint8_t *parent2, uint8_t *newMatrix1, uint8_t *newMatrix2, args_t *args) {
    int crossoverPoint1 = rand() % (args->X * args->Y * args->Z);
    int crossoverPoint2 = rand() % (args->X * args->Y * args->Z);
    
    if(crossoverPoint1 > crossoverPoint2){
        int tmp = crossoverPoint2;
        crossoverPoint2 = crossoverPoint1;
        crossoverPoint1 = tmp;
    }

    int xBound = args->X;
    if(args->xSym){
        if(args->X % 2 != 0){
            xBound = args->X / 2 + 1;
        } else {
            xBound = args->X / 2;
        }
    }

    int yBound = args->Y;
    if(args->ySym){
        if(args->Y % 2 != 0){
            yBound = args->Y / 2 + 1;
        } else {
            yBound = args->Y / 2;
        }
    }

    int zBound = args->Z;
    if(args->zSym){
        if(args->Z % 2 != 0){
            zBound = args->Z / 2 + 1;
        } else {
            zBound = args->Z / 2;
        }
    }
    
    for(int y = 0; y < yBound ; y++) {
        for(int z = 0; z < zBound ; z++) {
            for(int x = 0; x < xBound; x++) {
                int offset = OFFSET(x, y, z, args->Y, args->Z);
                if(offset < crossoverPoint1 || offset >= crossoverPoint2) {
                    setSymBlock(newMatrix1, x, y, z, parent1[offset], args);
                    setSymBlock(newMatrix2, x, y, z, parent2[offset], args);
                } else {
                    setSymBlock(newMatrix1, x, y, z, parent2[offset], args);
                    setSymBlock(newMatrix2, x, y, z, parent1[offset], args);
                }
            }
        }
    }
}

reactor_t* getBestReactorsFromGen(listHead_t *population, listHead_t *newPopulation, int bestCount, args_t *args){
    reactor_t *bestOfAll = population->head->r;

    for(int i=1; i<=bestCount; i++){
        listItem_t *lastBestItem = population->head;
        double lastBestFitness = -RAND_MAX;
        listItem_t *currentItem = population->head;
        while(currentItem!=NULL){
            if((currentItem->r->fitness > lastBestFitness) && (isInList(newPopulation, currentItem) == false)){                
                lastBestItem = currentItem;
                lastBestFitness = currentItem->r->fitness;
            }
            currentItem = currentItem->next;
        }
        reactor_t *bestCurrentReactor = copyReactor(lastBestItem->r, args);
        addToList(newPopulation, bestCurrentReactor);
        if(i==1){
            bestOfAll = lastBestItem->r;
        }
    }
    return bestOfAll;
}

reactor_t* selectParentRoulette(listHead_t *population){
    double totalFitness = 0.0;
    listItem_t *item = population->head;
    
    while(item != NULL) {
        totalFitness += item->r->fitness;
        item = item->next;
    }

    double randomValue = (rand() / (double)RAND_MAX) * totalFitness;

    double cumulativeFitness = 0.0;
    item = population->head;
    while(item != NULL) {
        cumulativeFitness += item->r->fitness;
        if (cumulativeFitness >= randomValue) {
            return item->r;
        }
        item = item->next;
    }

    return item->r;
}

reactor_t* selectParentTournament(listHead_t *population, int tournamentSize, args_t *args){
    int index = rand() % listLen(population);
    reactor_t* best = getListItemFromIndex(population, index);
    double bestFitness = best->fitness;

    for(int i = 0; i < tournamentSize; i++) {
        listItem_t *item = population->head;
        int randomIndex = rand() % args->populationSize;
        for(int j = 0; j < randomIndex; j++) {
            item = item->next;
        }
        
        if (item->r->fitness > bestFitness) {
            best = item->r;
            bestFitness = item->r->fitness;
        }
    }

    return best;
}

void runGA(listHead_t *population, args_t *args){
    listHead_t newPopulation;
    newPopulation.head = NULL;
    int bestCount = (int) sqrt(ELITE_RATIO * args->populationSize);    
    double maxDiversity = 0.0;
    for (size_t gen = 0; gen < args->genMax; gen++) {
        printf("\nGen %zu\n\n",gen);
        

        double diversity = calculateDiversity(population);
        maxDiversity = (diversity > maxDiversity) ? diversity : maxDiversity;
        
        double adaptiveMutationRate = calculateAdaptiveMutationRate(MUTATION_RATE, diversity, maxDiversity);

        reactor_t *bestReactor =  getBestReactorsFromGen(population, &newPopulation, bestCount, args);

        printf("best fitness of this generation = %f\n",bestReactor->fitness);
        
        for(size_t i = args->populationSize - bestCount; i > 0;){
            if(((double) rand() / RAND_MAX < CROSSOVER_RATE) && (i!=1)){
                int tournamentSize = (int) args->populationSize * TOURNAMENT_SIZE_RATIO;
                reactor_t *parent1 = selectParentTournament(population, tournamentSize, args); //selectParentRoulette(population);
                reactor_t *parent2;
                size_t j = 0;
                do {
                    parent2 = selectParentTournament(population, tournamentSize, args); //selectParentRoulette(population);
                    j++;
                    
                } while((parent2 == parent1) && (j != args->populationSize));

                if(j==args->populationSize) {
                    int index = (int) rand() / bestCount;
                    reactor_t *randBestReactor = getListItemFromIndex(&newPopulation, index);
                    reactor_t *newReactor = copyReactor(randBestReactor, args);
                    
                    mutate(newReactor, adaptiveMutationRate, args);

                    addToList(&newPopulation, newReactor);
                    i--;
                } else {
                    
                    reactor_t *newReactor1 = initializeReactor(args);
                    reactor_t *newReactor2 = initializeReactor(args);
                    crossover(parent1->matrix, parent2->matrix, newReactor1->matrix, newReactor2->matrix, args);
                    calculatePowerHeat(newReactor1, args);
                    calculatePowerHeat(newReactor2, args);
                    setFitness(newReactor1, args);
                    setFitness(newReactor2, args);

                    
                    addToList(&newPopulation, newReactor1);
                    addToList(&newPopulation, newReactor2);
                    i-=2;
                }
                
            } else {
                int index = (int) rand() % bestCount;
                reactor_t *randBestReactor = getListItemFromIndex(&newPopulation, index);
                reactor_t *newReactor = copyReactor(randBestReactor, args);

                mutate(newReactor, adaptiveMutationRate, args);

                addToList(&newPopulation, newReactor);
                i--;
            }
            
        }
        freeList(population);
        population->head = newPopulation.head;
        fineTunePopulation(population, args);
        newPopulation.head = NULL; 
    }
}