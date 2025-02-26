#include "../include/common.h"


uint8_t* setMatrix(args_t *args){
    uint8_t *matrix = (uint8_t *) malloc(args->X * args->Y * args->Z * sizeof(uint8_t));
    memset(matrix, RED, args->X * args->Y * args->Z * sizeof(uint8_t));
    
    return matrix;
}

void freeList(listHead_t *head){
    listItem_t* current = head->head;
    listItem_t *tmp;

    while (current != NULL){
        tmp = current;
        current = current->next;

        free(tmp->r->matrix);
        free(tmp->r);
        free(tmp);
    }
}

int listLen(listHead_t *head){
    listItem_t *item = head->head;
    int count = 0;
    if(item == NULL){
        return count;
    } else {
        count++;
        item = item->next;
        while(item != NULL) {
            item = item->next;
            count++;
        }
        return count;
    }
}

reactor_t* getListItemFromIndex(listHead_t *head, int index){
    listItem_t *ret = head->head;
    if(index == 0){
        return ret->r;
    } else {
        ret = ret->next;
        int i = 1;
        while(ret != NULL){
            if(i == index) {
                return ret->r;
            } else {
                ret = ret->next;
                i++;
            }
            
        }
    }
    return NULL;
}

reactor_t* popListFromValue(listHead_t *head, reactor_t *r){
    listItem_t *del;
    reactor_t *ret;
    if(head->head->r == r){
        del = head->head;
        head->head = del->next;
        ret = del->r;
        free(del);
        return ret;
    } else {
        listItem_t *current = head->head;
        while(current->next != NULL){
            if(current->next->r == r){
                del = current->next;
                current->next = del->next;
                ret = del->r;
                free(del);
                return ret;
            } else {
                current = current->next;
            }
        }
        
    }
    return NULL;
}

void addToList(listHead_t *lHeadPtr, reactor_t *r){
    listItem_t *item = lHeadPtr->head;

    if(item != NULL){
        while(item->next != NULL) item = item->next;
        item->next = (listItem_t *) malloc(sizeof(listItem_t));
        item->next->r = r;
        item->next->next = NULL;
    } else {
        lHeadPtr->head = (listItem_t *) malloc(sizeof(listItem_t));
        lHeadPtr->head->r = r;
        lHeadPtr->head->next = NULL;
    }
}

bool isInList(listHead_t *lHeadPtr, listItem_t *item){
    listItem_t *current = lHeadPtr->head;
    while(current != NULL){
        if(current == item){
            return true;
        }
        current = current->next;
    }
    return false;   
}

int getAdjacentBlock(unsigned char *matrix, int x, int y, int z, blockType_t type, args_t *args){
    int adj = 0;

    if((x-1) >= 0){
        if(matrix[OFFSET(x-1, y, z, args->Y, args->Z)] == type) adj++;
    }
    if((x+1) < args->X){
        if(matrix[OFFSET(x+1, y, z, args->Y, args->Z)] == type) adj++;
    }
    if((y-1) >= 0){
        if(matrix[OFFSET(x, y-1, z, args->Y, args->Z)] == type) adj++;
    }
    if((y+1) < args->Y){
        if(matrix[OFFSET(x, y+1, z, args->Y, args->Z)] == type) adj++;
    }
    if((z-1) >= 0){
        if(matrix[OFFSET(x, y, z-1, args->Y, args->Z)] == type) adj++;
    }
    if((z+1) < args->Z){
        if(matrix[OFFSET(x, y, z+1, args->Y, args->Z)] == type) adj++;
    }

    return adj;
}

void calculatePowerHeat(reactor_t *r, args_t *args){
    r->totalPower = 0;
    r->totalHeat = 0;
    r->malus = 0;
    
    for (int y = 0; y < args->Y; y++) {
        for (int z = 0; z < args->Z; z++) {
            for (int x = 0; x < args->X; x++) {
                switch (r->matrix[OFFSET(x, y, z, args->Y, args->Z)]) {
                    case RED:
                        if(getAdjacentBlock(r->matrix, x, y, z, CELL, args) != 0){
                            r->totalHeat = r->totalHeat - 160;
                        } else{
                            r->totalHeat = r->totalHeat - 80;
                            r->malus++;
                        }
                        break;
                    case CELL:
                        int adjCELL = getAdjacentBlock(r->matrix, x, y, z, CELL, args);
                        r->totalPower = r->totalPower + (adjCELL + 1) * r->basePower;
                        r->totalHeat = r->totalHeat + (((adjCELL+1) * (adjCELL + 2))/(double)2) * r->baseHeat;

                        adjCELL = getAdjacentBlock(r->matrix, x, y, z, RED, args);
                        if(adjCELL == 0){
                            r->malus++;
                        }
                        break;
                    case CRYO:
                        if(getAdjacentBlock(r->matrix, x, y, z, CRYO, args) == 0){
                            r->totalHeat = r->totalHeat - 160;
                        } else{
                            r->totalHeat = r->totalHeat - 80;
                            r->malus++;
                        }
                        break;
                    case HEL:
                        r->totalHeat = r->totalHeat - 125;
                        adjCELL = getAdjacentBlock(r->matrix, x, y, z, CELL, args);
                        int adjCRYO = getAdjacentBlock(r->matrix, x, y, z, CRYO, args);
                        if((adjCELL > 0) && (adjCRYO == 0)){
                            r->malus++;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void copyMatrix(uint8_t *matrixSrc, uint8_t *matrixDst, args_t *args){
    for (int y = 0; y < args->Y; y++) {
        for (int z = 0; z < args->Z; z++) {
            for (int x = 0; x < args->X; x++) {
                matrixDst[OFFSET(x, y, z, args->Y, args->Z)] = matrixSrc[OFFSET(x, y, z, args->Y, args->Z)];
            }
        }
    }
}


reactor_t* getBestReactor(listHead_t *lHeadPtr){
    listItem_t *item = lHeadPtr->head;
    reactor_t *best = item->r;
    double bestPower = -RAND_MAX;
    while(item != NULL){
        if(item->r->totalHeat <= 0){
            double curFitness = item->r->fitness;
            if(curFitness >= bestPower){
                best = item->r;
                bestPower = curFitness;
            }
        }
        item = item->next;
    }
    return best;
}