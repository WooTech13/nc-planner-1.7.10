#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_VALUE HEL

pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;


struct ThreadArgs {
    unsigned char *matrix;
    int x, y, z, X, Y, Z;
    struct listHead *lHeadPtr;
};

struct reactor{
    unsigned char *matrix;
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

enum bool{
    false,
    true
};

char print[] = {'x','R','C','#','H'};


enum blockType {
    DEF,
    RED,
    CRYO,
    CELL,
    HEL, 
};

int offset(int x, int y, int z, int X, int Z) { 
    return (y * Z * X) + (z * X) + x;
}

unsigned char* setMatrix( int X, int Y, int Z){
    unsigned char *matrix = malloc(X * Y * Z * sizeof(unsigned char));
    memset(matrix, DEF, X * Y * Z * sizeof(unsigned char));
    
    return matrix;
}


void freeList(struct listItem* head){
    struct listItem* tmp;

    while (head != NULL){
        tmp = head;
        head = head->next;
        if(tmp->r != NULL){
            free(tmp->r->matrix);
            free(tmp->r);
        }

        free(tmp);
    }
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
        if(matrix[offset(x-1, y, z, X, Z)] == type) adj++;
    }
    if((x+1) < X){
        if(matrix[offset(x+1, y, z, X, Z)] == type) adj++;
    }
    if((y-1) >= 0){
        if(matrix[offset(x, y-1, z, X, Z)] == type) adj++;
    }
    if((y+1) < Y){
        if(matrix[offset(x, y+1, z, X, Z)] == type) adj++;
    }
    if((z-1) >= 0){
        if(matrix[offset(x, y, z-1, X, Z)] == type) adj++;
    }
    if((z+1) < Z){
        if(matrix[offset(x, y, z+1, X, Z)] == type) adj++;
    }

    return adj;
}

void calculatePowerHeat(struct reactor *r, int X, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                unsigned char toCheck = r->matrix[offset(x, y, z, X, Z)];
                int adjCELL;
                switch (toCheck) {
                    case RED:
                        adjCELL = getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CELL);
                        if(adjCELL != 0){
                            r->totalHeat = r->totalHeat - 160;
                        } else{
                            r->totalHeat = r->totalHeat - 80;
                        }
                        break;
                    case CELL:
                        adjCELL = getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CELL);
                        r->totalPower = r->totalPower + (adjCELL + 1) * r->basePower;
                        r->totalHeat = r->totalHeat + (((adjCELL+1) * (adjCELL + 2))/(double)2) * r->baseHeat;
                        break;
                    case CRYO:
                        adjCELL = getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CRYO);
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

void copyMatrix(unsigned char *matrixSrc, unsigned char *matrixDst, int X, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                matrixDst[offset(x, y, z, X, Z)] = matrixSrc[offset(x, y, z, X, Z)];
            }
        }
    }
}

enum bool checkWholeMatrix(struct reactor *r, int X, int Y, int Z){
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                unsigned char toCheck = r->matrix[offset(x, y, z, X, Z)];
                int adjCELL;
                switch (toCheck) {
                    case RED:
                        adjCELL = getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CELL);
                        if(adjCELL == 0){
                            return false;
                        }
                    case CELL:
                        continue;
                    case CRYO:
                        adjCELL = getAdjacentBlock(r->matrix, x, y, z, X, Y, Z, CRYO);
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
    r->basePower = 150;
    r->baseHeat = 25;
    r->totalPower = 0;
    r->totalHeat = 0;
    calculatePowerHeat(r, X, Y, Z);
    return r->totalHeat <= 0;
    
}


void printMatrix(unsigned char *matrix, int X, int Y, int Z) {
    for (int y = 0; y < Y; y++) {
        for (int z = 0; z < Z; z++) {
            for (int x = 0; x < X; x++) {
                printf("%c ", print[matrix[offset(x, y, z, X, Z)]]);
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
                printf("%c ", print[r->matrix[offset(x, y, z, X, Z)]]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void generateCombinations(unsigned char *matrix, int x, int y, int z, int X, int Y, int Z, struct listHead *lHeadPtr) {
    if(x+z+y != 0){
        if (x == X) {
            struct reactor *r = (struct reactor *)malloc(sizeof(struct reactor));
            r->matrix = setMatrix(X, Y, Z);
            copyMatrix(matrix, r->matrix, X, Y, Z);
            pthread_barrier_wait(&barrier);
            addToList(lHeadPtr, r);
            return;
        }
    }

    for (int value = RED; value <= MAX_VALUE; value++) {
        matrix[offset(x, y, z, X, Z)] = value;
        if (z + 1 < Z) {
            generateCombinations(matrix, x, y, z + 1, X, Y, Z, lHeadPtr);
        }else{
            generateCombinations(matrix, x + 1, y, 0, X, Y, Z, lHeadPtr);
        }        
    }
}

void *threadFunction(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    generateCombinations(args->matrix, 0, args->y, 0, args->X, args->Y, args->Z, args->lHeadPtr);
    free(arg);
    return NULL;
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

    unsigned char *matrix = setMatrix(X, Y, Z);
    printMatrix(matrix, X, Y, Z);
    
    
    struct listHead lHead;
    lHead.head = NULL;

    clock_t t;
    double cpu_time_used; 
    t = clock();

    pthread_barrier_init(&barrier, NULL, Y);
    pthread_t *threads = malloc(Y * sizeof(pthread_t));

    for (int y = 0; y < Y; y++) {
        struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
        *args = (struct ThreadArgs){matrix, 0, y, 0, X, Y, Z, &lHead};
        
        if (pthread_create(&threads[y], NULL, threadFunction, args) != 0) {
            fprintf(stderr, "Error creating thread for Y=%d\n", y);
            return -1;
        } else {
            printf("Created thread %d\n",y);
        }
    }
    

    for (int y = 0; y < Y; y++) {
        pthread_join(threads[y], NULL);
        printf("Thread %d done\n",y);
    }
    pthread_barrier_destroy(&barrier);


    
    struct listItem *oldItem = lHead.head;
    struct reactor r;

    struct listHead newLHead;
    newLHead.head = NULL;
    enum bool res = checkWholeMatrix(oldItem->r, X, Y, Z);
    if(res == true){
        addToList(&newLHead, &r);
    }
    int item = 0;
    while(oldItem->next != NULL){
        oldItem = oldItem->next;
        enum bool res = checkWholeMatrix(oldItem->r, X, Y, Z);
        if(res == true){
            addToList(&newLHead, oldItem->r);
            oldItem->r = NULL;
        }
        item++;
    }


    struct reactor *best = getBestReactor(&newLHead);
    printReactor(best, X, Y, Z);

    t = clock() - t;
    cpu_time_used = ((double) t) / CLOCKS_PER_SEC;
    printf("Took %f seconds to execute \n", cpu_time_used);
    free(matrix);
    free(threads);
    freeList(newLHead.head);
    freeList(lHead.head);
    return 0;
}
