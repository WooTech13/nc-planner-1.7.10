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

void genAlgo(const int X, const  int Y, const int Z, const double basePower, const double baseHeat){
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

void advGenAlgo() {
    startAdvGenAlgo();
}

int main(int argc, char *argv[]) {
    printf("1\n");
    if (argc < 7) {
        fprintf(stderr, "Usage: %s <X> <Y> <Z> <base_power> <base_heat> [--sym AXES]\n", argv[0]);
        fprintf(stderr, "  X,Y,Z       : dimensions intérieures du réacteur\n");
        fprintf(stderr, "  base_power  : puissance de base du combustible\n");
        fprintf(stderr, "  base_heat   : chaleur de base du combustible\n");
        fprintf(stderr, "  --mode      : generation mode, ex: combination, genAglo, advGenAlgo\n");
        fprintf(stderr, "  --sym AXES  : axes de symétrie, ex: X  XY  XZ  XYZ  (optionnel)\n");
        fprintf(stderr, "  Exemples : ./reactor 5 5 5 60 18 --sym XZ\n");
        fprintf(stderr, "             ./reactor 7 7 7 60 18 --sym XYZ\n");
        return 1;
    }

    GX = atoi(argv[1]);
    GY = atoi(argv[2]);
    GZ = atoi(argv[3]);
    BASE_POWER = atof(argv[4]);
    BASE_HEAT  = atof(argv[5]);
    GSIZE = GX * GY * GZ;

    /* Parse --sym */
    for (int a = 7; a < argc; a++) {
        if (strcmp(argv[a], "--sym") == 0 && a + 1 < argc) {
            const char *axes = argv[a + 1];
            for (int i = 0; axes[i]; i++) {
                if (axes[i] == 'X' || axes[i] == 'x') SYM_X = true;
                if (axes[i] == 'Y' || axes[i] == 'y') SYM_Y = true;
                if (axes[i] == 'Z' || axes[i] == 'z') SYM_Z = true;
            }
            a++; /* saute la valeur */
        }
    }
    printf("2\n");
    if (GX <= 0 || GY <= 0 || GZ <= 0 || GSIZE > 100000) {
        fprintf(stderr, "Erreur : dimensions invalides ou trop grandes (max ~46³)\n");
        return 1;
    } 

    /* Parse --type */
    for (int a = 6; a < argc; a++) {
        if (strcmp(argv[a], "--mode") == 0 && a + 1 < argc) {

            const char *types = argv[a + 1];
            printf("%s\n",types);
            printf("combination : %d, genAlgo, %d, advGenAlgo : %d\n",strcmp(types,"combination"),strcmp(types,"genAlgo"),strcmp(types,"advGenAlgo"));
            if(strcmp(types,"combination") == 0){
                printf("3\n");
                combination(GX, GY, GZ, BASE_POWER, BASE_HEAT);
            } else if(strcmp(types,"genAlgo") == 0) {
                printf("4\n");
                genAlgo(GX, GY, GZ, BASE_POWER, BASE_HEAT);
            } else if(strcmp(types,"advGenAlgo") == 0){
                printf("5\n");
                advGenAlgo();
            } else {
                fprintf(stderr, "Error : wrong generation mode\n");
                return 1;
            }
            a++; /* saute la valeur */
        }
    }
    
    return 0;
}