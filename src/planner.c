#include "../include/planner.h"

int checkWholeMatrix(reactor_t *r, args_t *args){
    for (int y = 0; y < args->Y; y++) {
        for (int z = 0; z < args->Z; z++) {
            for (int x = 0; x < args->X; x++) {
                switch (r->matrix[OFFSET(x, y, z, args->Y, args->Z)]) {
                    case RED:
                        if(getAdjacentBlock(r->matrix, x, y, z, CELL, args) == 0) return 0;
                    case CELL:
                        continue;
                    case CRYO:
                        if(getAdjacentBlock(r->matrix, x, y, z, CRYO, args) > 0) return 0;
                    case HEL:
                        continue;
                    default:
                        return 0;
                }
            }
        }
    }
    r->basePower = 150;
    r->baseHeat = 25;
    r->totalPower = 0;
    r->totalHeat = 0;
    calculatePowerHeat(r, args);
    return r->totalHeat <= 0;

}

int incrementMatrix(uint8_t *matrix, size_t total_size){
    for (size_t i = 0; i < total_size; i++) {
        if (matrix[i] < HEL) {
            matrix[i]++;
            return 1;
        }
        matrix[i] = RED;
    }
    return 0;
}

void generateMatrices(listHead_t *lHead, args_t *args) {
    uint8_t *matrix = setMatrix(args);
    uint64_t count = 0;

    for (uint64_t i = 0; i < (1ULL << (2 * args->X * args->Y * args->Z)); i++) {
        uint8_t *local_matrix = setMatrix(args);
        for (int j = 0; j < args->X * args->Y * args->Z; j++) {
            local_matrix[j] = (i >> (2 * j)) & 3;
        }
        reactor_t *r = (reactor_t *) malloc(sizeof(reactor_t));
        r->matrix = local_matrix;
        if (checkWholeMatrix(r, args)) {
            addToList(lHead, r);
        } else {
            free(r->matrix);
            free(r);
        }
        count++;
    }
    free(matrix);
}