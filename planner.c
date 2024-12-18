#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define X 2
#define Y 2
#define Z 2
#define MAX_VALUE HEL

struct reactor{
    unsigned char matrix[X][Y][Z];
    double basePower;
    double baseHeat;
    double totalPower;
    double totalHeat;
};

struct listHead{
    struct listItem *head;
};

struct listItem{
    struct reactor r;
    struct listItem *next;
};

enum bool{
    true,
    false
};

char print[] = {'x','R','C','#','H'};


enum blockType {
    DEF,
    RED,
    CRYO,
    CELL,
    HEL, 
};

void addToList(struct listHead *lHeadPtr, struct reactor r){
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

int getAdjacentBlock(unsigned char matrix[X][Y][Z], int x, int y, int z, enum blockType type){
    int adj = 0;

    if((x-1) >= 0){
        if(matrix[x-1][y][z] == type) adj++;
    }
    if((x+1) < X){
        if(matrix[x+1][y][z] == type) adj++;
    }
    if((y-1) >= 0){
        if(matrix[x][y-1][z] == type) adj++;
    }
    if((y+1) < Y){
        if(matrix[x][y+1][z] == type) adj++;
    }
    if((z-1) >= 0){
        if(matrix[x][y][z-1] == type) adj++;
    }
    if((z+1) < Z){
        if(matrix[x][y][z+1] == type) adj++;
    }

    return adj;
}

enum bool checkMatrix(unsigned char matrix[X][Y][Z], int x, int y, int z){
    unsigned char toCheck = matrix[x][y][z];
    int adjCELL;
    switch (toCheck) {
        case RED:
            adjCELL = getAdjacentBlock(matrix, x, y, z, CELL);
            if(adjCELL > 0){
                return true;
            } else{
                return false;
            }
        case CELL:
            return true;
        case CRYO:
            adjCELL = getAdjacentBlock(matrix, x, y, z, CRYO);
            if(adjCELL != 0){
                return true;
            } else{
                return false;
            }
        case HEL:
            return true;
        case DEF:
            return true;
        default:
            return false; 
    }
}

void calculatePowerHeat(unsigned char matrix[X][Y][Z], double basePower, double *totalPowerPtr, double baseHeat, double *totalHeatPtr){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                unsigned char toCheck = matrix[x][y][z];
                int adjCELL;
                switch (toCheck) {
                    case RED:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, CELL);
                        if(adjCELL != 0){
                            *totalHeatPtr = *totalHeatPtr - 160;
                        } else{
                            *totalHeatPtr = *totalHeatPtr - 80;
                        }
                        break;
                    case CELL:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, CELL);
                        *totalPowerPtr = *totalPowerPtr + (adjCELL + 1) * basePower;
                        *totalHeatPtr = *totalHeatPtr + (((adjCELL+1) * (adjCELL + 2))/2) * baseHeat;
                        break;
                    case CRYO:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, CRYO);
                        if(adjCELL == 0){
                            *totalHeatPtr = *totalHeatPtr - 160;
                        } else{
                            *totalHeatPtr = *totalHeatPtr - 80;
                        }
                        break;
                    case HEL:
                        *totalHeatPtr = *totalHeatPtr - 125;
                        break;
                    case DEF:
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void copyMatrix(unsigned char matrixSrc[X][Y][Z], unsigned char matrixDst[X][Y][Z]){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                matrixDst[x][y][z] = matrixSrc[x][y][z];
            }
        }
    }
}

enum bool checkWholeMatrix(unsigned char matrix[X][Y][Z], struct reactor *rPtr){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                unsigned char toCheck = matrix[x][y][z];
                int adjCELL;
                switch (toCheck) {
                    case RED:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, CELL);
                        if(adjCELL == 0){
                            return false;
                        }
                    case CELL:
                        continue;
                    case CRYO:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, CRYO);
                        if(adjCELL > 0){
                            return false;
                        }
                    case HEL:
                        continue;
                    case DEF:
                        return false;
                    default:
                        return false; 
                }
            }
        }
    }
    copyMatrix(matrix, rPtr->matrix);

    double basePower, totalPower, *totalPowerPtr, baseHeat, totalHeat, *totalHeatPtr;
    basePower = 150;
    totalPower = 0;
    totalPowerPtr = &totalPower;
    baseHeat = 25;
    totalHeat = 0;
    totalHeatPtr = &totalHeat;
    
    calculatePowerHeat(matrix, basePower, totalPowerPtr, baseHeat, totalHeatPtr);
    if(totalHeat <= 0){
        rPtr->basePower = basePower;
        rPtr->baseHeat = baseHeat;
        rPtr->totalPower = totalPower;
        rPtr->totalHeat = totalHeat;
        return true;
    } else{
        return false;
    }
    
    
    
}

void printMatrix(unsigned char matrix[X][Y][Z]) {
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                printf("%c ", print[matrix[x][y][z]]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void printReactor(struct reactor *r) {
    printf("With base power %f and base heat %f:\npower: %f\nheat: %f\n",r->basePower, r->baseHeat, r->totalPower, r->totalHeat);

    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                printf("%c ", print[r->matrix[x][y][z]]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void generateCombinations(unsigned char matrix[X][Y][Z], int x, int y, int z, struct listHead *lHeadPtr) {
    if(x+z+y != 0){
        enum bool res = checkMatrix(matrix, x, y, z);
        if(res != true){
            return;
        } else{
            if (x == X) {
                struct reactor r;
                res = checkWholeMatrix(matrix, &r);
                if(res == true){
                    addToList(lHeadPtr, r);
                    return;  
                } else{
                    return;  
                }
            }
        }
    }
    

    for (int value = RED; value <= MAX_VALUE; value++) {
        matrix[x][y][z] = value;
        if (z + 1 < Z) {
            generateCombinations(matrix, x, y, z + 1, lHeadPtr);
        } else if (y + 1 < Y) {
            generateCombinations(matrix, x, y + 1, 0, lHeadPtr);
        } else {
            generateCombinations(matrix, x + 1, 0, 0, lHeadPtr);
        }
    }
}

void getBestReactor(struct listHead *lHeadPtr, struct reactor *best){
    struct listItem *item = lHeadPtr->head;
    double bestPowerHeat = 0;
    while(item != NULL){
        double curPowerHeat = item->r.totalPower + item->r.totalHeat;
        if(curPowerHeat > bestPowerHeat){
            best = &item->r;
            bestPowerHeat = curPowerHeat;
        }
        
        item = item->next;
    }
    printReactor(best);

}

int main() {
    unsigned char matrix[X][Y][Z] = {DEF};
    struct listHead lHead;
    lHead.head = NULL;

    struct reactor best;

    clock_t t;
    double cpu_time_used; 
    t = clock();
    generateCombinations(matrix, 0, 0, 0, &lHead);
    getBestReactor(&lHead, &best);

    t = clock() - t;
    cpu_time_used = ((double) t) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute \n", cpu_time_used);
    return 0;
}
