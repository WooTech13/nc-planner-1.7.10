#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int checkWholeMatrix(struct reactor *r, int X, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                switch (r->matrix[OFFSET(x, y, z, Y, Z)]) {
                    case RED:
                        if(getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CELL) == 0) return 0;
                    case CELL:
                        continue;
                    case CRYO:
                        if(getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CRYO) > 0) return 0;
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
    calculatePowerHeat(r, X, Y, Z);
    return r->totalHeat <= 0;

}

int incrementMatrix(uint8_t *matrix, size_t total_size) {
    for (size_t i = 0; i < total_size; i++) {
        if (matrix[i] < HEL) {
            matrix[i]++;
            return 1;
        }
        matrix[i] = RED;
    }
    return 0;
}

void generateMatrices(size_t X, size_t Y, size_t Z, struct listHead *lHead) {
    uint8_t *matrix = setMatrix(X, Y, Z);
    uint64_t count = 0;

    for (uint64_t i = 0; i < (1ULL << (2 * X * Y * Z)); i++) {
        uint8_t *local_matrix = setMatrix(X, Y, Z);
        for (size_t j = 0; j < X * Y * Z; j++) {
            local_matrix[j] = (i >> (2 * j)) & 3;
        }
        struct reactor *r = (struct reactor *) malloc(sizeof(struct reactor));
        r->matrix = local_matrix;
        if (checkWholeMatrix(r, X, Y, Z)) {
            addToList(lHead, r);
        } else {
            free(r->matrix);
            free(r);
        }
        count++;
    }
    printf("Total matrices generated: %lu\n", count);
    free(matrix);
}
