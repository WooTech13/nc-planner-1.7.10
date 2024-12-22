#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_VALUE HEL
#define OFFSET(x, y, z, Y, Z) ((x) * (Y * Z) + (y) * Z + (z))

struct reactor{
    uint8_t *matrix;
    double basePower;
    double baseHeat;
    double totalPower;
    double totalHeat;
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


void freeList(struct listItem* head){
    struct listItem* current = head->next;
    struct listItem *tmp;

    while (current != NULL){
        tmp = current;
        current = current->next;

        free(tmp->r->matrix);
        free(tmp->r);
        free(tmp);
    }
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
    printf("With base power %f and base heat %f:\npower: %f\nheat: %f\n",r->basePower, r->baseHeat, r->totalPower, r->totalHeat);

    printMatrix(r->matrix, X, Y, Z);
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
        if(matrix[OFFSET(x-1, y, z, X, Z)] == type) adj++;
    }
    if((x+1) < X){
        if(matrix[OFFSET(x+1, y, z, X, Z)] == type) adj++;
    }
    if((y-1) >= 0){
        if(matrix[OFFSET(x, y-1, z, X, Z)] == type) adj++;
    }
    if((y+1) < Y){
        if(matrix[OFFSET(x, y+1, z, X, Z)] == type) adj++;
    }
    if((z-1) >= 0){
        if(matrix[OFFSET(x, y, z-1, X, Z)] == type) adj++;
    }
    if((z+1) < Z){
        if(matrix[OFFSET(x, y, z+1, X, Z)] == type) adj++;
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

struct reactor* getBestReactor(struct listHead *lHeadPtr){
    struct listItem *item = lHeadPtr->head;
    struct reactor *best = item->r;
    double bestPowerHeat = item->r->totalPower + item->r->totalHeat;
    while(item->next != NULL){
        item = item->next;
        double curPowerHeat = item->r->totalPower + item->r->totalHeat;
        if(curPowerHeat >= bestPowerHeat){
            best = item->r;
            bestPowerHeat = curPowerHeat;
        }
    }
    double curPowerHeat = item->r->totalPower + item->r->totalHeat;
    if(curPowerHeat >= bestPowerHeat){
        best = item->r;
        bestPowerHeat = curPowerHeat;
    }
    return best;
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

    struct listHead lHead;
    lHead.head = NULL;

    clock_t t;
    double cpu_time_used;
    t = clock();

    generateMatrices(X, Y, Z, &lHead);

    struct reactor *best = getBestReactor(&lHead);
    printReactor(best, X, Y, Z);

    t = clock() - t;
    cpu_time_used = ((double) t) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute \n", cpu_time_used);
    freeList(lHead.head);
    return 0;
}
