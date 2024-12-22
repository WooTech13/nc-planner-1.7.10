#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MUTATION_RATE 0.1
#define CROSSOVER_RATE 0.8
#define ELITE_RATIO 0.01
#define TOURNAMENT_SIZE_RATIO 0.03

#define MAX_VALUE HEL

#define OFFSET(x, y, z, Y, Z) ((x) * (Y * Z) + (y) * Z + (z))

struct reactor {
    uint8_t *matrix;
    double basePower;
    double baseHeat;
    double totalPower;
    double totalHeat;
    double fitness;
};

struct listHead{
    struct listItem *head;
};

struct listItem{
    struct reactor *r;
    struct listItem *next;
};

char print[] = {'R','C','#','H'};


enum blockType {
    RED,
    CRYO,
    CELL,
    HEL,
};

uint8_t* setMatrix(int X, int Y, int Z){
    uint8_t *matrix = (uint8_t *) malloc(X * Y * Z * sizeof(uint8_t));
    memset(matrix, RED, X * Y * Z * sizeof(uint8_t));
    
    return matrix;
}


void freeList(struct listHead* head){
    struct listItem* current = head->head;
    struct listItem *tmp;

    while (current != NULL){
        tmp = current;
        current = current->next;

        free(tmp->r->matrix);
        free(tmp->r);
        free(tmp);
    }
}

int listLen(struct listHead *head){
    struct listItem *item = head->head;
    int count = 0;
    if(item == NULL){
        return count;
    } else {
        count++;
        item = item->next;
        while(item != NULL) {
            item = item->next;
            count++;
        }
        return count;
    }
}

struct reactor* popListFromValue(struct listHead *head, struct reactor *r){
    struct listItem *del;
    struct reactor *ret;
    if(head->head->r == r){
        del = head->head;
        head->head = del->next;
        ret = del->r;
        free(del);
        return ret;
    } else {
        struct listItem *current = head->head;
        while(current->next != NULL){
            if(current->next->r == r){
                del = current->next;
                current->next = del->next;
                ret = del->r;
                free(del);
                return ret;
            } else {
                current = current->next;
            }
        }
        
    }
    return NULL;
}

void printMatrix(uint8_t *matrix, int X, int Y, int Z) {
    for (int y = 0; y < Y; y++) {
        for (int x = 0; x < X; x++) {
            for (int z = 0; z < Z; z++) {
                printf("%c ", print[matrix[OFFSET(x, y, z, Y, Z)]]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void printReactor(struct reactor *r, int X, int Y, int Z) {
    printMatrix(r->matrix, X, Y, Z);
    printf("With base power %f and base heat %f:\npower: %f\nheat: %f\n",r->basePower, r->baseHeat, r->totalPower, r->totalHeat);
}

void addToList(struct listHead *lHeadPtr, struct reactor *r){
    struct listItem *item = lHeadPtr->head;

    if(item != NULL){
        while(item->next != NULL) item = item->next;
        item->next = (struct listItem *) malloc(sizeof(struct listItem));
        item->next->r = r;
        item->next->next = NULL;
    } else {
        lHeadPtr->head = (struct listItem *) malloc(sizeof(struct listItem));
        lHeadPtr->head->r = r;
        lHeadPtr->head->next = NULL;
    }
}

int getAdjacentBlock(unsigned char *matrix, int x, int y, int z, int X, int Y, int Z, enum blockType type){
    int adj = 0;

    if((x-1) >= 0){
        if(matrix[OFFSET(x-1, y, z, Y, Z)] == type) adj++;
    }
    if((x+1) < X){
        if(matrix[OFFSET(x+1, y, z, Y, Z)] == type) adj++;
    }
    if((y-1) >= 0){
        if(matrix[OFFSET(x, y-1, z, Y, Z)] == type) adj++;
    }
    if((y+1) < Y){
        if(matrix[OFFSET(x, y+1, z, Y, Z)] == type) adj++;
    }
    if((z-1) >= 0){
        if(matrix[OFFSET(x, y, z-1, Y, Z)] == type) adj++;
    }
    if((z+1) < Z){
        if(matrix[OFFSET(x, y, z+1, Y, Z)] == type) adj++;
    }

    return adj;
}

void calculatePowerHeat(struct reactor *r, int X, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                switch (r->matrix[OFFSET(x, y, z, Y, Z)]) {
                    case RED:
                        if(getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CELL) != 0){
                            r->totalHeat = r->totalHeat - 160;
                        } else{
                            r->totalHeat = r->totalHeat - 80;
                        }
                        break;
                    case CELL:
                        int adjCELL = getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CELL);
                        r->totalPower = r->totalPower + (adjCELL + 1) * r->basePower;
                        r->totalHeat = r->totalHeat + (((adjCELL+1) * (adjCELL + 2))/(double)2) * r->baseHeat;
                        break;
                    case CRYO:
                        if(getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CRYO) == 0){
                            r->totalHeat = r->totalHeat - 160;
                        } else{
                            r->totalHeat = r->totalHeat - 80;
                        }
                        break;
                    case HEL:
                        r->totalHeat = r->totalHeat - 125;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void copyMatrix(uint8_t *matrixSrc, uint8_t *matrixDst, int X, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                matrixDst[OFFSET(x, y, z, Y, Z)] = matrixSrc[OFFSET(x, y, z, Y, Z)];
            }
        }
    }
}


struct reactor* getBestReactor(struct listHead *lHeadPtr){
    struct listItem *item = lHeadPtr->head;
    struct reactor *best = item->r;
    double bestPower = item->r->totalPower;
    while(item != NULL){
        if(item->r->totalHeat <= 0){
            double curFitness = item->r->fitness;
            if(curFitness >= bestPower){
                best = item->r;
                bestPower = curFitness;
            }
        }
        item = item->next;
    }
    return best;
}

double setFitness(struct reactor *r){
    return (r->totalPower / r->totalHeat) * -1;
}

struct reactor* initializeReactor(int X, int Y, int Z){
    struct reactor* r = (struct reactor*)malloc(sizeof(struct reactor));

    r->matrix=setMatrix(X,Y,Z);

    for(int i=0;i<X*Y*Z;i++){
        r->matrix[i]=rand() % 4;
    }
    r->basePower = 150;
    r->baseHeat = 25;
    r->totalPower = 0;
    r->totalHeat = 0;
    calculatePowerHeat(r,X,Y,Z);
    r->fitness = setFitness(r);
    return r;
}

struct reactor* initializeReactorFromMatrix(uint8_t *offspring1, int X, int Y, int Z){
    struct reactor* r = (struct reactor*)malloc(sizeof(struct reactor));

    r->matrix = setMatrix(X, Y, Z);
    copyMatrix(offspring1, r->matrix, X, Y, Z);
    r->basePower = 150;
    r->baseHeat = 25;
    r->totalPower = 0;
    r->totalHeat = 0;

    calculatePowerHeat(r,X,Y,Z);
    r->fitness = setFitness(r);
    return r;
}

void initializePopulation(struct listHead *population, int X, int Y, int Z, size_t populationSize) {
    size_t i = 0;
    while(i < populationSize){
        struct reactor *r = initializeReactor(X, Y, Z);
        if(r->fitness > 0) {
            addToList(population, r);
            i++;
        }        
    }
}

double calculateDiversity(struct listHead *population) {
    double sumFitness = 0.0;
    double sumSquaredFitness = 0.0;
    int count = 0;
    struct listItem *item = population->head;

    while (item != NULL) {
        sumFitness += item->r->fitness;
        sumSquaredFitness += item->r->fitness * item->r->fitness;
        count++;
        item = item->next;
    }

    double meanFitness = sumFitness / count;
    double variance = (sumSquaredFitness / count) - (meanFitness * meanFitness);
    return sqrt(variance);
}


void mutate(struct reactor *r, int X, int Y, int Z, double baseMutationRate, double diversity) {
    double adaptiveMutationRate = baseMutationRate * (1.0 + (1.0 - diversity));
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                if ((rand() / (double)RAND_MAX) < adaptiveMutationRate) {
                    if(r->fitness > 0){
                        r->matrix[OFFSET(x, y, z, Y, Z)] = CELL;
                    }else{
                        if (getAdjacentBlock(r->matrix, x,  y,  z,  X,  Y,  Z, CRYO) == 0){
                            r->matrix[OFFSET(x, y, z, Y, Z)] = CRYO;
                        } else if(getAdjacentBlock(r->matrix, x,  y,  z,  X,  Y,  Z, CELL) > 0) {
                            r->matrix[OFFSET(x, y, z, Y, Z)] = RED;
                        } else {
                            r->matrix[OFFSET(x, y, z, Y, Z)] = HEL;
                        }
                    }
                    calculatePowerHeat(r, X, Y, Z);
                }
            }
        }
    }
}

void crossover(uint8_t *parent1, uint8_t *parent2, uint8_t *offspring1, uint8_t *offspring2, size_t size) {
    size_t crossoverPoint = rand() % size;
    for (size_t i = 0; i < size; i++) {
        if (i < crossoverPoint) {
            offspring1[i] = parent1[i];
            offspring2[i] = parent2[i];
        } else {
            offspring1[i] = parent2[i];
            offspring2[i] = parent1[i];
        }
    }
}

struct reactor* selectParentRoulette(struct listHead *population) {
    double totalFitness = 0.0;
    struct listItem *item = population->head;
    
    while (item != NULL) {
        totalFitness += item->r->fitness;
        item = item->next;
    }

    double randomValue = (rand() / (double)RAND_MAX) * totalFitness;

    double cumulativeFitness = 0.0;
    item = population->head;
    while (item != NULL) {
        cumulativeFitness += item->r->fitness;
        if (cumulativeFitness >= randomValue) {
            return item->r;
        }
        item = item->next;
    }

    return item->r;
}

struct reactor* selectParentTournament(struct listHead *population, int tournamentSize, size_t populationSize) {
    struct reactor* best = NULL;
    double bestFitness = -INFINITY;

    for (int i = 0; i < tournamentSize; i++) {
        struct listItem *item = population->head;
        int randomIndex = rand() % populationSize;
        for (int j = 0; j < randomIndex; j++) {
            item = item->next;
        }
        
        if (item->r->fitness > bestFitness) {
            best = item->r;
            bestFitness = item->r->fitness;
        }
    }

    return best;
}

struct listHead* runGA(struct listHead *population, int X, int Y, int Z, size_t populationSize, size_t genMax) {
    struct listHead newPopulation;
    newPopulation.head = NULL;
    const int elitismCount = 2;    

    for (size_t gen = 0; gen < genMax; gen++) {
        printf("\nGen %zu\n\n",gen);

        struct listItem *eliteItem = population->head;
        struct reactor *bestItemReactor1 = NULL;
        struct reactor *bestItemReactor2 = NULL;
        double bestFitness1 = 0, bestFitness2 = 0;
    
        while (eliteItem != NULL) {
            struct reactor *currentReactor = eliteItem->r;
            if (currentReactor->fitness > bestFitness1) {
                bestItemReactor2 = bestItemReactor1;
                bestFitness2 = bestFitness1;
                bestItemReactor1 = currentReactor;
                bestFitness1 = currentReactor->fitness;
            } else if (currentReactor->fitness > bestFitness2) {
                bestItemReactor2 = currentReactor;
                bestFitness2 = currentReactor->fitness;
            }
            eliteItem = eliteItem->next;
        }


        struct reactor *bestReactor1 = initializeReactorFromMatrix(bestItemReactor1->matrix, X, Y, Z);
        struct reactor *bestReactor2 = initializeReactorFromMatrix(bestItemReactor2->matrix, X, Y, Z);
        addToList(&newPopulation, bestReactor1);
        addToList(&newPopulation, bestReactor2);

        printf("best fitness of this generation = %f\n",bestReactor1->fitness);

        for(size_t i = populationSize - elitismCount; i > 0;){
            
            if(((double) rand() / RAND_MAX < CROSSOVER_RATE) && (i!=1)){
                int tournamentSize = (int) populationSize * TOURNAMENT_SIZE_RATIO;
                struct reactor *parent1 = selectParentTournament(population, tournamentSize, populationSize); //selectParentRoulette(population);
                struct reactor *parent2;
                size_t j = 0;
                do {
                    parent2 = selectParentTournament(population, tournamentSize, populationSize); //selectParentRoulette(population);
                    j++;
                    
                } while((parent2 == parent1) && (j != populationSize));

                if(j==populationSize) {
                    struct reactor *newReactor = initializeReactor(X, Y, Z);
                    copyMatrix(bestReactor1->matrix, newReactor->matrix, X, Y, Z);
                    mutate(newReactor, X , Y , Z, MUTATION_RATE, calculateDiversity(population));

                    addToList(&newPopulation, newReactor);
                    i--;
                } else {
                    uint8_t *newMatrix1 = setMatrix(X, Y, Z);
                    uint8_t *newMatrix2 = setMatrix(X, Y, Z);
                    crossover(parent1->matrix, parent2->matrix, newMatrix2, newMatrix2, X * Y * Z);

                    struct reactor *newReactor1 = initializeReactorFromMatrix(newMatrix1,X, Y, Z);
                    struct reactor *newReactor2 = initializeReactorFromMatrix(newMatrix2,X, Y, Z);
                    addToList(&newPopulation, newReactor1);
                    addToList(&newPopulation, newReactor2);
                    free(newMatrix1);
                    free(newMatrix2);
                    i-=2;
                }
                
            } else {
                struct reactor *newReactor = initializeReactor(X, Y, Z);
                copyMatrix(bestReactor1->matrix, newReactor->matrix, X, Y, Z);
                mutate(newReactor, X , Y , Z, MUTATION_RATE, calculateDiversity(population));

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

int main(int argc, char *argv[]) {
    if (argc != 4)
    {
        fprintf (stderr, "Error: usage: %s <X size> <Y size> <Z size>. Exiting program.\n", argv[0]);
        return (-1);
    }

    const int X = atoi (argv[1]);

    if(X <= 0){
        fprintf (stderr, "Error: X <= 0 Exiting program.");
        return (-1);
    }

    const int Y = atoi (argv[2]);

    if(Y <= 0){
        fprintf (stderr, "Error: Y <= 0 Exiting program.");
        return (-1);
    }

    const int Z = atoi (argv[3]);

    if(Z <= 0){
        fprintf (stderr, "Error: Z <= 0 Exiting program.");
        return (-1);
    }

    srand(time(NULL));
    struct listHead population;
    population.head = NULL;
    size_t populationSize = (size_t) 2 * MAX_VALUE * sqrt(X * Y * Z);
    size_t genMax = (size_t) 5 * sqrt(X * Y * Z);

    initializePopulation(&population, X, Y, Z, populationSize);

    struct listHead *newPopulation = runGA(&population, X, Y, Z, populationSize, genMax);

    struct reactor *best = getBestReactor(newPopulation);
    printReactor(best, X, Y, Z);

    freeList(&population);
    return 0;
}
