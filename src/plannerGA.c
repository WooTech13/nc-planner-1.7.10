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
            printf("Using sssymetry on %c\n",dim);
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

void setFitness(reactor_t *r){
    r->fitness =  (r->totalPower / r->totalHeat) * -1;
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
    r->totalPower = 0;
    r->totalHeat = 0;
    calculatePowerHeat(r, args);
    setFitness(r);
    return r;
}

reactor_t* initializeReactorFromMatrix(uint8_t *newMatrix, args_t *args){
    reactor_t* r = (reactor_t*)malloc(sizeof(reactor_t));

    r->matrix = setMatrix(args);
    copyMatrix(newMatrix, r->matrix, args);
    r->basePower = 150;
    r->baseHeat = 25;
    r->totalPower = 0;
    r->totalHeat = 0;

    calculatePowerHeat(r, args);
    setFitness(r);
    return r;
}

void initializePopulation(listHead_t *population, args_t *args){
    size_t i = 0;
    while(i < args->populationSize){
        reactor_t *r = initializeReactor(args);
        if(r->fitness > 0) {
            addToList(population, r);
            i++;
        }        
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




void mutate(reactor_t *r, double diversity, args_t *args){
    double adaptiveMutationRate = MUTATION_RATE * (1.0 + (1.0 - diversity));
    
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
    setFitness(r);
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

listHead_t* runGA(listHead_t *population, args_t *args){
    listHead_t newPopulation;
    newPopulation.head = NULL;
    const int elitismCount = 2;    
    double maxDiversity = 0.0;
    for (size_t gen = 0; gen < args->genMax; gen++) {
        printf("\nGen %zu\n\n",gen);
        

        double diversity = calculateDiversity(population);
        maxDiversity = (diversity > maxDiversity) ? diversity : maxDiversity;
        
        double adaptiveMutationRate = calculateAdaptiveMutationRate(MUTATION_RATE, diversity, maxDiversity);

        listItem_t *eliteItem = population->head;
        reactor_t *bestItemReactor1 = NULL;
        reactor_t *bestItemReactor2 = NULL;
        double bestFitness1 = 0, bestFitness2 = 0;
    
        while(eliteItem != NULL) {
            reactor_t *currentReactor = eliteItem->r;
            if(currentReactor->fitness > bestFitness1) {
                bestItemReactor2 = bestItemReactor1;
                bestFitness2 = bestFitness1;
                bestItemReactor1 = currentReactor;
                bestFitness1 = currentReactor->fitness;
            } else if(currentReactor->fitness > bestFitness2) {
                bestItemReactor2 = currentReactor;
                bestFitness2 = currentReactor->fitness;
            }
            eliteItem = eliteItem->next;
        }


        reactor_t *bestReactor1 = initializeReactorFromMatrix(bestItemReactor1->matrix, args);
        reactor_t *bestReactor2 = initializeReactorFromMatrix(bestItemReactor2->matrix, args);
        addToList(&newPopulation, bestReactor1);
        addToList(&newPopulation, bestReactor2);

        printf("best fitness of this generation = %f\n",bestReactor1->fitness);
        
        for(size_t i = args->populationSize - elitismCount; i > 0;){
            if(((double) rand() / RAND_MAX < CROSSOVER_RATE) && (i!=1)){
                //int tournamentSize = (int) args->populationSize * TOURNAMENT_SIZE_RATIO;
                reactor_t *parent1 = /*selectParentTournament(population, tournamentSize, populationSize); //*/selectParentRoulette(population);
                reactor_t *parent2;
                size_t j = 0;
                do {
                    parent2 = /*selectParentTournament(population, tournamentSize, populationSize); //*/selectParentRoulette(population);
                    j++;
                    
                } while((parent2 == parent1) && (j != args->populationSize));

                if(j==args->populationSize) {
                    reactor_t *newReactor = initializeReactor(args);
                    copyMatrix(bestReactor1->matrix, newReactor->matrix, args);
                    
                    mutate(newReactor, adaptiveMutationRate, args);

                    addToList(&newPopulation, newReactor);
                    i--;
                } else {
                    
                    uint8_t *newMatrix1 = setMatrix(args);
                    uint8_t *newMatrix2 = setMatrix(args);
                    crossover(parent1->matrix, parent2->matrix, newMatrix1, newMatrix2, args);
                    
                    reactor_t *newReactor1 = initializeReactorFromMatrix(newMatrix1,args);
                    reactor_t *newReactor2 = initializeReactorFromMatrix(newMatrix2,args);
                    addToList(&newPopulation, newReactor1);
                    addToList(&newPopulation, newReactor2);
                    free(newMatrix1);
                    free(newMatrix2);
                    i-=2;
                }
                
            } else {
                
                reactor_t *newReactor = initializeReactor(args);
                copyMatrix(bestReactor1->matrix, newReactor->matrix, args);
                mutate(newReactor, adaptiveMutationRate, args);

                addToList(&newPopulation, newReactor);
                i--;
            }
            
        }

        freeList(population);
        population->head = newPopulation.head;
        newPopulation.head = NULL; 
    }
    
    return population;
}