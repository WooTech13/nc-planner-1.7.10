#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_VALUE HEL

struct reactor{
    unsigned char ***matrix;
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

unsigned char*** setMatrix( int X, int Y, int Z){
    unsigned char ***matrix = (unsigned char ***)malloc(Y*sizeof(unsigned char**));
    for (int y = 0; y < Y; y++) {
        unsigned char **tmpY = (unsigned char **)malloc(Z*sizeof(unsigned char*));
        matrix[y] = tmpY;
    }
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            unsigned char *tmpZ = (unsigned char *)malloc(X*sizeof(unsigned char));
            matrix[y][z] = tmpZ;
        }
    }
    
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                matrix[y][z][x] = DEF;                
            }
        }
    }
    
    return matrix;
}

void freeMatrix(unsigned char ***matrix, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            free(matrix[y][z]);
        }
        free(matrix[y]);
    }
    free(matrix);
}

void freeList(struct listItem* head, int Y, int Z)
{
   struct listItem* tmp;

   while (head != NULL)
    {
       tmp = head;
       head = head->next;
       freeMatrix(tmp->r.matrix, Y, Z);
       free(tmp);
    }
}

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

int getAdjacentBlock(unsigned char ***matrix, int x, int y, int z, int X, int Y, int Z, enum blockType type){
    int adj = 0;

    if((x-1) >= 0){
        if(matrix[y][z][x-1] == type) adj++;
    }
    if((x+1) < X){
        if(matrix[y][z][x+1] == type) adj++;
    }
    if((y-1) >= 0){
        if(matrix[y-1][z][x] == type) adj++;
    }
    if((y+1) < Y){
        if(matrix[y+1][z][x] == type) adj++;
    }
    if((z-1) >= 0){
        if(matrix[y][z-1][x] == type) adj++;
    }
    if((z+1) < Z){
        if(matrix[y][z+1][x] == type) adj++;
    }

    return adj;
}

enum bool checkMatrix(unsigned char ***matrix, int x, int y, int z, int X, int Y, int Z){
    unsigned char toCheck = matrix[y][z][x];
    int adjCELL;
    switch (toCheck) {
        case RED:
            adjCELL = getAdjacentBlock(matrix, x, y, z, X, Y, Z, CELL);
            if(adjCELL > 0){
                return true;
            } else{
                return false;
            }
        case CELL:
            return true;
        case CRYO:
            adjCELL = getAdjacentBlock(matrix, x, y, z, X, Y, Z, CRYO);
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

void calculatePowerHeat(unsigned char ***matrix, int X, int Y, int Z, struct reactor *r){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                unsigned char toCheck = matrix[y][z][x];
                int adjCELL;
                switch (toCheck) {
                    case RED:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, X, Y, Z, CELL);
                        if(adjCELL != 0){
                            r->totalHeat = r->totalHeat - 160;
                        } else{
                            r->totalHeat = r->totalHeat - 80;
                        }
                        break;
                    case CELL:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, X, Y, Z, CELL);
                        r->totalPower = r->totalPower + (adjCELL + 1) * r->basePower;
                        r->totalHeat = r->totalHeat + (((adjCELL+1) * (adjCELL + 2))/(double)2) * r->baseHeat;
                        break;
                    case CRYO:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, X, Y, Z, CRYO);
                        if(adjCELL == 0){
                            r->totalHeat = r->totalHeat - 160;
                        } else{
                            r->totalHeat = r->totalHeat - 80;
                        }
                        break;
                    case HEL:
                        r->totalHeat = r->totalHeat - 125;
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

void copyMatrix(unsigned char ***matrixSrc, unsigned char ***matrixDst, int X, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                matrixDst[y][z][x] = matrixSrc[y][z][x];
            }
        }
    }
}

enum bool checkWholeMatrix(unsigned char ***matrix, int X, int Y, int Z, struct reactor *rPtr){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                unsigned char toCheck = matrix[y][z][x];
                int adjCELL;
                switch (toCheck) {
                    case RED:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, X, Y, Z, CELL);
                        if(adjCELL == 0){
                            return false;
                        }
                    case CELL:
                        continue;
                    case CRYO:
                        adjCELL = getAdjacentBlock(matrix, x, y, z, X, Y, Z, CRYO);
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
    struct reactor r;
    r.matrix = matrix;
    r.basePower = 150;
    r.baseHeat = 25;
    r.totalPower = 0;
    r.totalHeat = 0;
    
    calculatePowerHeat(matrix, X, Y, Z, &r);
    if(r.totalHeat <= 0){
        rPtr->matrix = setMatrix(X, Y, Z);
        copyMatrix(matrix, rPtr->matrix, X, Y, Z);
        rPtr->basePower = r.basePower;
        rPtr->baseHeat = r.baseHeat;
        rPtr->totalPower = r.totalPower;
        rPtr->totalHeat = r.totalHeat;
        return true;
    } else{
        return false;
    }
    
}

void printMatrix(unsigned char ***matrix, int X, int Y, int Z) {
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                printf("%c ", print[matrix[y][z][x]]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void printReactor(struct reactor *r, int X, int Y, int Z) {
    printf("With base power %f and base heat %f:\npower: %f\nheat: %f\n",r->basePower, r->baseHeat, r->totalPower, r->totalHeat);

    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                printf("%c ", print[r->matrix[y][z][x]]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void generateCombinations(unsigned char ***matrix, int x, int y, int z, int X, int Y, int Z, struct listHead *lHeadPtr) {
    if(x+z+y != 0){
        enum bool res;
        if (x == X) {
            struct reactor r;
            res = checkWholeMatrix(matrix, X, Y, Z, &r);
            if(res == true){
                addToList(lHeadPtr, r);  
            }
            return;
        }
    }

    for (int value = RED; value <= MAX_VALUE; value++) {
        matrix[y][z][x] = value;
        if (z + 1 < Z) {
            generateCombinations(matrix, x, y, z + 1, X, Y, Z, lHeadPtr);
        } else if (y + 1 < Y) {
            generateCombinations(matrix, x, y + 1, 0, X, Y, Z, lHeadPtr);
        } else{
            generateCombinations(matrix, x + 1, 0, 0, X, Y, Z, lHeadPtr);
        }        
    }
}

struct reactor* getBestReactor(struct listHead *lHeadPtr){
    struct listItem *item = lHeadPtr->head;
    struct reactor *best;
    double bestPowerHeat = 0;
    while(item != NULL){
        double curPowerHeat = item->r.totalPower + item->r.totalHeat;
        if(curPowerHeat >= bestPowerHeat){
            best = &item->r;
            bestPowerHeat = curPowerHeat;
        }
        
        item = item->next;
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

    unsigned char ***matrix = setMatrix(X, Y, Z);
    printMatrix(matrix, X, Y, Z);
    
    struct listHead lHead;
    lHead.head = NULL;

    clock_t t;
    double cpu_time_used; 
    t = clock();
    generateCombinations(matrix, 0, 0, 0, X, Y, Z, &lHead);
    freeMatrix(matrix, Y, Z);

    struct reactor *best = getBestReactor(&lHead);
    printReactor(best, X, Y, Z);

    t = clock() - t;
    cpu_time_used = ((double) t) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute \n", cpu_time_used);
    
    
    freeList(lHead.head, Y, Z);
    return 0;
}
