#include "../include/main.h"

void combination(const int X, const  int Y, const int Z, const double basePower, const double baseHeat){
    args_t args = {X, Y, Z, basePower, baseHeat, 0, 0, 0, 0, 0};

    listHead_t lHead;
    lHead.head = NULL;

    clock_t t;
    double cpu_time_used;
    t = clock();

    generateMatrices(&lHead, &args);

    reactor_t *best = getBestReactor(&lHead);
    printReactor(best, &args);

    t = clock() - t;
    cpu_time_used = ((double) t) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute \n", cpu_time_used);
    freeList(&lHead);
}

void plannerGA(const int X, const  int Y, const int Z, const double basePower, const double baseHeat){
    const bool xSym = getSym('X');

    const bool ySym = getSym('Y');

    const bool zSym = getSym('Z');

    const size_t populationSize = (size_t) 2 * MAX_VALUE * sqrt(X * Y * Z);

    const size_t genMax = (size_t) 5 * sqrt(X * Y * Z);

    args_t args = {X, Y, Z, basePower, baseHeat, populationSize, genMax, xSym, ySym, zSym };

    unsigned int *randBuf = (unsigned int *) malloc(sizeof(unsigned int));
    getrandom(randBuf,sizeof(unsigned int),GRND_NONBLOCK);
    srand(*randBuf);
    
    listHead_t population;
    population.head = NULL;
    

    initializePopulation(&population, &args);

    runGenAlgo(&population, &args);

    reactor_t *best = getBestReactor(&population);
    printReactor(best, &args);

    freeList(&population);
    free(randBuf);
}

int main(int argc, char *argv[]) {
    if (argc < 7) {
        fprintf(stderr, "Usage: %s <X> <Y> <Z> <base_power> <base_heat> <gen_type>\n", argv[0]);
        fprintf(stderr, "  X,Y,Z       : inside reactor's dimension\n");
        fprintf(stderr, "  base_power  : base power\n");
        fprintf(stderr, "  base_heat   : base heat\n");
        fprintf(stderr, "  gen_type    : 1 - combination, 2 - genetic algorithm\n");
        return 1;
    }

    const int X = atoi(argv[1]);
    const int Y = atoi(argv[2]);
    const int Z = atoi(argv[3]);
    const double basePower = atof(argv[4]);
    const double baseHeat  = atof(argv[5]);
    const int genType = atoi(argv[6]);
    const int SIZE = X * Y * Z;

    if (X <= 0 || Y <= 0 || Z <= 0 || SIZE > 100000) {
        fprintf(stderr, "Error : wrong dimensions\n");
        return 1;
    }

    if (genType != 1 && genType != 2) {
        fprintf(stderr, "Error : wrong dimensions\n");
        return 1;
    }

    if(genType == 1){
        printf("Staring the combination generation with X=%d, Y=%d, Z=%d\n",X,Y,Z);
        combination(X, Y, Z, basePower, baseHeat);
    } else if(genType == 2){
        printf("Staring the genetic algorithm generation with X=%d, Y=%d, Z=%d\n",X,Y,Z);
        plannerGA(X, Y, Z, basePower, baseHeat);
    } else {
        printf("Error: please enter Y, y, N or n\n");
    }
    
    return 0;
}
