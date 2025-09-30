#include "../include/display.h"

const char print[] = {'R','C','#','H','x'};

void printTotalBlocks(uint8_t *matrix, args_t *args){
    int count_REDSTONE = 0, count_CRYOTHEUM = 0, count_FUEL_CELL = 0, count_LIQUID_HELIUM = 0;
    for (int y = 0; y < args->Y; y++) {
        for (int z = 0; z < args->Z; z++) {
            for (int x = 0; x < args->X; x++) {
                switch (matrix[OFFSET(x, y, z, args->Y, args->Z)]) {
                    case REDSTONE:
                        count_REDSTONE++;
                        break;
                    case FUEL_CELL:
                        count_FUEL_CELL++;
                        break;
                    case GELID_CRYOTHEUM:
                        count_CRYOTHEUM++;
                        break;
                    case LIQUID_HELIUM:
                        count_LIQUID_HELIUM++;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    printf("Total number of blocks used");
    printf("\n");
    printf("Casing outline: %d",8*(args->X + args->Y + args->Z + 1));
    printf("\n");
    printf("Casing: %d",2*(args->X * args->Y + args->X * args->Z + args->Y * args->Z));
    printf("\n");
    printf("Redstone cooler: %d",count_REDSTONE);
    printf("\n");
    printf("GELID_CRYOTHEUM cooler: %d",count_CRYOTHEUM);
    printf("\n");
    printf("Reactor FUEL_CELL: %d",count_FUEL_CELL);
    printf("\n");
    printf("Helium cooler: %d",count_LIQUID_HELIUM);
    printf("\n"); 
}

void printMatrix(uint8_t *matrix, args_t *args){
    for (int y = 0; y < args->Y; y++) {
        for (int z = 0; z < args->Z; z++) {
            for (int x = 0; x < args->X; x++) {
                printf("%c ", print[matrix[OFFSET(x, y, z, args->Y, args->Z)]]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void printReactor(reactor_t *r, args_t *args) {
    printMatrix(r->matrix, args);
    printf("With base power %f and base heat %f:\nPower: %f\nHeat: %f\nFitness: %f\nMalus: %f\n",r->basePower, r->baseHeat, r->totalPower, r->totalHeat, r->fitness, r->malus);
    printf("\n");
    printTotalBlocks(r->matrix, args);
}