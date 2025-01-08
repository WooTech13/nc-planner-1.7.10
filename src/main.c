#include "../include/main.h"

void planner(const int X, const  int Y, const int Z){
    args_t args = {X, Y, Z, 0, 0, 0, 0, 0};

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

void plannerGA(const int X, const  int Y, const int Z){
    const bool xSym = getSym('X');

    const bool ySym = getSym('Y');

    const bool zSym = getSym('Z');

    const size_t populationSize = (size_t) 4 * MAX_VALUE * sqrt(X * Y * Z);

    const size_t genMax = (size_t) 10 * sqrt(X * Y * Z);

    args_t args = {X, Y, Z, populationSize, genMax, xSym, ySym, zSym };
    printf("%d, %d, %d, %zu, %zu, %d, %d, %d\n",args.X, args.Y, args.Z, args.populationSize, args.genMax, args.xSym, args.ySym, args.zSym);

    unsigned int *randBuf = (unsigned int *) malloc(sizeof(unsigned int));
    getrandom(randBuf,sizeof(unsigned int),GRND_NONBLOCK);
    srand(*randBuf);

    reactor_t *r = initializeReactor(&args);
    printReactor(r, &args);

    listHead_t population;
    population.head = NULL;
    

    initializePopulation(&population, &args);

    listHead_t *newPopulation = runGA(&population, &args);

    reactor_t *best = getBestReactor(newPopulation);
    printReactor(best, &args);

    freeList(&population);
}

int main(int argc, char *argv[]) {
    if (argc != 4)
    {
        fprintf(stderr, "Error: usage: %s <X size> <Y size> <Z size>. Exiting program.\n", argv[0]);
        return -1;
    }

    const int X = atoi (argv[1]);

    if(X <= 0){
        fprintf(stderr, "Error: X <= 0 Exiting program.");
        return -1;
    }

    const int Y = atoi (argv[2]);

    if(Y <= 0){
        fprintf(stderr, "Error: Y <= 0 Exiting program.");
        return -1;
    }

    const int Z = atoi (argv[3]);

    if(Z <= 0){
        fprintf(stderr, "Error: Z <= 0 Exiting program.");
        return -1;
    }

    char choice;
    while(true){
        printf("Choose the generation type:\n[1] combination\n[2] genetic algorithm\nGeneration type ? ");
        choice = getchar();
        if(choice == '1'){
            printf("Staring the combination generation with X=%d, Y=%d, Z=%d\n",X,Y,Z);
            sleep(1);
            flush();
            planner(X, Y, Z);
            break;
        } else if(choice == '2'){
            printf("Staring the genetic algorithm generation with X=%d, Y=%d, Z=%d\n",X,Y,Z);
            flush();
            plannerGA(X, Y, Z);
            break;
        } else {
            printf("Error: please enter Y, y, N or n\n");
            flush();
        }
    }
    
    return 0;
}
