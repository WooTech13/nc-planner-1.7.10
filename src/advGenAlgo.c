/*
 * NuclearCraft Fission Reactor Optimizer
 * =======================================
 * Algorithme hybride : Génétique + Recuit Simulé + Parallélisation OpenMP
 *
 * Stratégie :
 *   1. Génération d'une population initiale aléatoire (avec heuristiques de seed)
 *   2. Évaluation parallèle (OpenMP) de chaque individu
 *   3. Sélection par tournoi + croisement 3D + mutation adaptative
 *   4. Recuit simulé local sur les meilleurs individus pour convergence fine
 *   5. Migration entre îles (island model) pour diversité génétique
 *
 * Compile : gcc -O3 -fopenmp -lm -o reactor nuclearcraft_optimizer.c
 * Usage   : ./reactor <X> <Y> <Z> <base_power> <base_heat>
 */

#include "../include/advGenAlgo.h"

/* ═══════════════════════════════════════════════════════════════════════
 * ACCÈS GRILLE
 * ═══════════════════════════════════════════════════════════════════════ */
static inline int IDX(int x, int y, int z) {
    return x + GX * (y + GY * z);
}

static inline int in_bounds(int x, int y, int z) {
    return (x >= 0 && x < GX && y >= 0 && y < GY && z >= 0 && z < GZ);
}

/* ─── Symétrie ───────────────────────────────────────────────────────── */

static inline int canon(int v, int dim) {
    return (v < (dim + 1) / 2) ? v : dim - 1 - v;
}

static void apply_symmetry(blockType_t *g) {
    if (!SYM_X && !SYM_Y && !SYM_Z) return;
    for (int y = 0; y < GY; y++)
    for (int z = 0; z < GZ; z++)
    for (int x = 0; x < GX; x++) {
        int cx = SYM_X ? canon(x, GX) : x;
        int cy = SYM_Y ? canon(y, GY) : y;
        int cz = SYM_Z ? canon(z, GZ) : z;
        if (cx != x || cy != y || cz != z)
            g[IDX(x,y,z)] = g[IDX(cx,cy,cz)];
    }
}

static inline int is_canonical(int x, int y, int z) {
    if (SYM_X && x > (GX - 1) / 2) return 0;
    if (SYM_Y && y > (GY - 1) / 2) return 0;
    if (SYM_Z && z > (GZ - 1) / 2) return 0;
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════════
 * ÉVALUATION DU RÉACTEUR
 * ═══════════════════════════════════════════════════════════════════════ */

/* Compte les voisins d'un type donné pour la cellule (cx,cy,cz) */
static int count_adjacent(const blockType_t *grid, int cx, int cy, int cz, blockType_t type) {
    int count = 0;
    for (int d = 0; d < 6; d++) {
        int nx = cx + DX[d];
        int ny = cy + DY[d];
        int nz = cz + DZ[d];
        if (in_bounds(nx, ny, nz) && grid[IDX(nx,ny,nz)] == type)
            count++;
    }
    return count;
}

static void evaluate(reactor_t *r) {
    /* Propage la symétrie avant tout calcul */
    apply_symmetry(r->matrix);
    const blockType_t *g = r->matrix;
    double total_energy = 0.0;
    double total_heat   = 0.0;

    /* Compte le nombre total de FUEL_CELL (compartiments c) */
    int num_fuel_cells = 0;
    for (int i = 0; i < GSIZE; i++)
        if (g[i] == FUEL_CELL) num_fuel_cells++;

    /* Itère chaque cellule intérieure */
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        blockType_t b = g[IDX(x,y,z)];
        int adj_fuel    = count_adjacent(g, x,y,z, FUEL_CELL);
        int adj_graphite= count_adjacent(g, x,y,z, GRAPHITE);
        int adj_gelid   = count_adjacent(g, x,y,z, GELID_CRYOTHEUM);

        switch (b) {
        case FUEL_CELL: {
            double e = (adj_fuel + 1.0) * BASE_POWER;
            double h = ((adj_fuel + 1.0) * (adj_fuel + 2.0) / 2.0) * BASE_HEAT;
            total_energy += e;
            total_heat   += h;
            break;
        }
        case GRAPHITE: {
            if (adj_fuel > 0) {
                double c = (double)num_fuel_cells;
                total_energy += c * BASE_POWER / 10.0;
                total_heat   += c * BASE_POWER / 5.0;
            }
            break;
        }
        case GELID_CRYOTHEUM: {
            double cooling = (adj_gelid == 0) ? 160.0 : 80.0;
            total_heat -= cooling;
            break;
        }
        case ENDERIUM: {
            double cooling = (adj_graphite >= 1) ? 160.0 : 80.0;
            total_heat -= cooling;
            break;
        }
        case GLOWSTONE: {
            double cooling = (adj_graphite == 6) ? 320.0 : 80.0;
            total_heat -= cooling;
            break;
        }
        case LIQUID_HELIUM: {
            total_heat -= 125.0;
            break;
        }
        case REDSTONE: {
            double cooling = (adj_fuel >= 1) ? 160.0 : 80.0;
            total_heat -= cooling;
            break;
        }
        default:
            break;
        }
    }

    r->energy = total_energy;
    r->heat   = total_heat;

    /* ── Calcul fitness ──────────────────────────────────────────────── */
    /* Objectif : maximiser énergie, chaleur < 0 et la plus proche de 0  */
    if (total_heat > 0.0) {
        /* Réacteur en surchauffe : pénalité massive proportionnelle */
        r->fitness = -HEAT_OVERLOAD_PENALTY - total_heat * 1000.0;
    } else {
        /* Chaleur négative : on veut max énergie, puis minimiser |heat| */
        /* fitness = energy - epsilon * |heat|                            */
        r->fitness = total_energy - fabs(total_heat) * HEAT_CLOSE_TO_ZERO_BONUS_FACTOR;
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 * ALLOCATION / LIBÉRATION
 * ═══════════════════════════════════════════════════════════════════════ */
static reactor_t *reactor_new(void) {
    reactor_t *r = (reactor_t*)malloc(sizeof(reactor_t));
    r->x = GX; r->y = GY; r->z = GZ;
    r->matrix = (blockType_t*)malloc(GSIZE * sizeof(blockType_t));
    r->energy = r->heat = r->fitness = 0.0;
    return r;
}

static void reactor_free(reactor_t *r) {
    free(r->matrix);
    free(r);
}

static void reactor_copy(reactor_t *dst, const reactor_t *src) {
    memcpy(dst->matrix, src->matrix, GSIZE * sizeof(blockType_t));
    dst->energy  = src->energy;
    dst->heat    = src->heat;
    dst->fitness = src->fitness;
}

/* ═══════════════════════════════════════════════════════════════════════
 * GÉNÉRATEUR ALÉATOIRE THREAD-LOCAL
 * ═══════════════════════════════════════════════════════════════════════ */

static inline unsigned int tl_rand(int tid) {
    tl_seed[tid] = tl_seed[tid] * 1664525u + 1013904223u;
    return tl_seed[tid];
}

static inline double tl_randf(int tid) {
    return (double)(tl_rand(tid) >> 1) / (double)0x7FFFFFFF;
}

static inline int tl_randi(int tid, int n) {
    return (int)(tl_randf(tid) * n);
}

/* ═══════════════════════════════════════════════════════════════════════
 * INITIALISATION
 * ═══════════════════════════════════════════════════════════════════════ */

static blockType_t random_block(int tid) {
    double r = tl_randf(tid);
    double cum = 0.0;
    for (int i = 0; i < NUM_BLOCK_TYPES; i++) {
        cum += BLOCK_PROBS[i];
        if (r < cum) return (blockType_t)i;
    }
    return FUEL_CELL;
}

static void randomize(reactor_t *r, int tid) {
    /* N'initialise que la zone canonique ; apply_symmetry() dans evaluate() complète */
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++)
        if (is_canonical(x, y, z))
            r->matrix[IDX(x,y,z)] = random_block(tid);
    evaluate(r);
}

/* Graine heuristique : FUEL_CELL entouré de refroidisseurs */
static void seed_heuristic(reactor_t *r, int tid) {
    /* Initialise uniquement la zone canonique */
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++)
        if (is_canonical(x, y, z))
            r->matrix[IDX(x,y,z)] = GELID_CRYOTHEUM;

    int spacing = 2;
    for (int z = 0; z < GZ; z += spacing)
    for (int y = 0; y < GY; y += spacing)
    for (int x = 0; x < GX; x += spacing)
        if (is_canonical(x, y, z) && tl_randf(tid) < 0.5)
            r->matrix[IDX(x,y,z)] = FUEL_CELL;

    /* Propage d'abord pour que count_adjacent() voie la grille complète */
    apply_symmetry(r->matrix);

    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        if (!is_canonical(x, y, z)) continue;
        if (r->matrix[IDX(x,y,z)] != FUEL_CELL &&
            count_adjacent(r->matrix, x,y,z, FUEL_CELL) > 0 && tl_randf(tid) < 0.3)
            r->matrix[IDX(x,y,z)] = GRAPHITE;
    }

    apply_symmetry(r->matrix);

    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        if (!is_canonical(x, y, z)) continue;
        int idx = IDX(x,y,z);
        if (r->matrix[idx] == GELID_CRYOTHEUM &&
            count_adjacent(r->matrix, x,y,z, FUEL_CELL) > 0 && tl_randf(tid) < 0.25)
            r->matrix[idx] = REDSTONE;
    }
    evaluate(r);
}

/* ═══════════════════════════════════════════════════════════════════════
 * OPÉRATEURS GÉNÉTIQUES
 * ═══════════════════════════════════════════════════════════════════════ */

/* Sélection par tournoi */
static int tournament_select(reactor_t **pop, int pop_size, int k, int tid) {
    int best = tl_randi(tid, pop_size);
    for (int i = 1; i < k; i++) {
        int candidate = tl_randi(tid, pop_size);
        if (pop[candidate]->fitness > pop[best]->fitness)
            best = candidate;
    }
    return best;
}

/* Croisement uniforme — ne touche que la zone canonique */
static void acrossover(reactor_t *child, const reactor_t *p1, const reactor_t *p2, int tid) {
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        if (!is_canonical(x, y, z)) continue;
        int i = IDX(x,y,z);
        child->matrix[i] = (tl_randf(tid) < 0.5) ? p1->matrix[i] : p2->matrix[i];
    }
}

/* Croisement par plan — ne touche que la zone canonique */
static void crossover_plane(reactor_t *child, const reactor_t *p1, const reactor_t *p2, int tid) {
    int axis = tl_randi(tid, 3);
    int cut;
    if (axis == 0)      cut = tl_randi(tid, (GX + 1) / 2);
    else if (axis == 1) cut = tl_randi(tid, (GY + 1) / 2);
    else                cut = tl_randi(tid, (GZ + 1) / 2);

    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        if (!is_canonical(x, y, z)) continue;
        int idx = IDX(x,y,z);
        int val = (axis==0)?x : (axis==1)?y : z;
        child->matrix[idx] = (val < cut) ? p1->matrix[idx] : p2->matrix[idx];
    }
}

/* Mutation simple — zone canonique uniquement */
static void amutate(reactor_t *r, double rate, int tid) {
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        if (!is_canonical(x, y, z)) continue;
        if (tl_randf(tid) < rate)
            r->matrix[IDX(x,y,z)] = random_block(tid);
    }
}

/* Mutation ciblée — zone canonique uniquement */
static void mutate_targeted(reactor_t *r, double rate, int tid) {
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        if (!is_canonical(x, y, z)) continue;
        int i = IDX(x,y,z);
        if (r->heat > 0) {
            if (tl_randf(tid) < rate) {
                blockType_t coolers[] = {GELID_CRYOTHEUM, LIQUID_HELIUM, ENDERIUM, REDSTONE, GLOWSTONE};
                r->matrix[i] = coolers[tl_randi(tid, 5)];
            }
        } else {
            double rnd = tl_randf(tid);
            if (rnd < rate * 0.4)       r->matrix[i] = FUEL_CELL;
            else if (rnd < rate * 0.6)  r->matrix[i] = GRAPHITE;
            else if (rnd < rate)        r->matrix[i] = random_block(tid);
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 * RECUIT SIMULÉ LOCAL
 * ═══════════════════════════════════════════════════════════════════════ */
static void simulated_annealing(reactor_t *r, int tid) {
    reactor_t *neighbor = reactor_new();
    reactor_copy(neighbor, r);

    double temp = SA_TEMP_INIT;
    reactor_t *current = reactor_new();
    reactor_copy(current, r);

    for (int iter = 0; iter < SA_ITERATIONS; iter++) {
        reactor_copy(neighbor, current);

        /* Perturbation : change 1 à 3 blocs dans la zone canonique */
        int n_changes = 1 + tl_randi(tid, 3);
        for (int k = 0; k < n_changes; k++) {
            /* Tirage d'une cellule canonique aléatoire */
            int cx = SYM_X ? tl_randi(tid, (GX + 1) / 2) : tl_randi(tid, GX);
            int cy = SYM_Y ? tl_randi(tid, (GY + 1) / 2) : tl_randi(tid, GY);
            int cz = SYM_Z ? tl_randi(tid, (GZ + 1) / 2) : tl_randi(tid, GZ);
            neighbor->matrix[IDX(cx,cy,cz)] = random_block(tid);
        }
        evaluate(neighbor);

        double delta = neighbor->fitness - current->fitness;
        if (delta > 0 || tl_randf(tid) < exp(delta / temp)) {
            reactor_copy(current, neighbor);
        }

        temp *= SA_COOLING;
    }

    if (current->fitness > r->fitness)
        reactor_copy(r, current);

    reactor_free(current);
    reactor_free(neighbor);
}

/* ═══════════════════════════════════════════════════════════════════════
 * COMPARATEUR POUR TRI
 * ═══════════════════════════════════════════════════════════════════════ */
static int cmp_fitness_desc(const void *a, const void *b) {
    const reactor_t *ra = *(const reactor_t **)a;
    const reactor_t *rb = *(const reactor_t **)b;
    if (ra->fitness > rb->fitness) return -1;
    if (ra->fitness < rb->fitness) return  1;
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * AFFICHAGE
 * ═══════════════════════════════════════════════════════════════════════ */
static void print_reactor(const reactor_t *r) {
    printf("\n══════════════════════════════════════════════\n");
    printf("  MEILLEUR RÉACTEUR TROUVÉ\n");
    printf("══════════════════════════════════════════════\n");
    printf("  Dimensions intérieures : %dx%dx%d\n", r->x, r->y, r->z);
    if (SYM_X || SYM_Y || SYM_Z)
        printf("  Symétrie active        :%s%s%s\n",
               SYM_X?" X":"", SYM_Y?" Y":"", SYM_Z?" Z":"");
    printf("  Énergie produite       : %.2f RF/t\n", r->energy);
    printf("  Chaleur nette          : %.2f H/t\n", r->heat);
    printf("  Score fitness          : %.4f\n", r->fitness);
    printf("══════════════════════════════════════════════\n\n");

    /* Comptage des blocs */
    int counts[NUM_BLOCK_TYPES] = {0};
    for (int i = 0; i < GSIZE; i++)
        counts[(int)r->matrix[i]]++;

    printf("  Composition :\n");
    for (int t = 0; t < NUM_BLOCK_TYPES; t++)
        if (counts[t] > 0)
            printf("    %-20s : %d\n", BLOCK_NAMES[t], counts[t]);

    printf("\n  Disposition par couche Z :\n");
    for (int y = 0; y < r->y; y++) {
        printf("  -- Couche Y=%d --\n", y);
        for (int z = 0; z < r->z; z++) {
            printf("    ");
            for (int x = 0; x < r->x; x++) {
                blockType_t b = r->matrix[IDX(x,y,z)];
                /* Abréviations 2 caractères : ordre = enum blockType_t */
                const char *abbr[] = {"FC","GR","GC","EN","GL","LH","RE"};
                printf("%s ", abbr[(int)b]);
            }
            printf("\n");
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 * ALGORITHME PRINCIPAL : ISLAND GENETIC ALGORITHM
 * ═══════════════════════════════════════════════════════════════════════ */
static reactor_t* runAdvGenAlgo() {
    /* Calcul de la taille effective de l'espace de recherche canonique */
    int cx = SYM_X ? (GX + 1) / 2 : GX;
    int cy = SYM_Y ? (GY + 1) / 2 : GY;
    int cz = SYM_Z ? (GZ + 1) / 2 : GZ;
    int canon_size = cx * cy * cz;

    printf("NuclearCraft reactor_t Optimizer\n");
    printf("Dimensions : %dx%dx%d (%d blocs) | base_power=%.1f | base_heat=%.1f\n",
           GX, GY, GZ, GSIZE, BASE_POWER, BASE_HEAT);
    printf("Symétrie   : [%s] [%s] [%s]  →  zone canonique %dx%dx%d (%d blocs, espace /%.0f)\n",
           SYM_X?"X":" ", SYM_Y?"Y":" ", SYM_Z?"Z":" ",
           cx, cy, cz, canon_size,
           (double)GSIZE / canon_size);
    printf("Algorithme : Island Genetic + Simulated Annealing + OpenMP\n\n");

    /* Initialisation des seeds */
    srand((unsigned)time(NULL));
    for (int i = 0; i < NUM_ISLANDS * POP_SIZE + 8; i++)
        tl_seed[i] = (unsigned)rand() ^ ((unsigned)i * 2654435761u);

    /* ── Allocation des populations par île ──────────────────────────── */
    reactor_t ***islands = (reactor_t***)malloc(NUM_ISLANDS * sizeof(reactor_t**));
    for (int isl = 0; isl < NUM_ISLANDS; isl++) {
        islands[isl] = (reactor_t**)malloc(POP_SIZE * sizeof(reactor_t*));
        for (int i = 0; i < POP_SIZE; i++)
            islands[isl][i] = reactor_new();
    }

    /* Meilleur global */
    reactor_t *global_best = reactor_new();
    global_best->fitness = -DBL_MAX;

    /* ── Initialisation parallèle ─────────────────────────────────────── */
    printf("Initialisation de la population...\n");
    #pragma omp parallel for schedule(dynamic)
    for (int isl = 0; isl < NUM_ISLANDS; isl++) {
        int tid = isl * POP_SIZE;
        for (int i = 0; i < POP_SIZE; i++) {
            if (i < POP_SIZE / 4)
                seed_heuristic(islands[isl][i], tid + i);
            else
                randomize(islands[isl][i], tid + i);
        }
    }

    /* ── Boucle principale ────────────────────────────────────────────── */
    reactor_t *child = NULL;
    double best_fitness_prev = -DBL_MAX;
    int stagnation = 0;
    double mutation_rate = MUTATION_RATE;
    int elite_n = (int)(POP_SIZE * ELITE_RATIO);

    printf("Évolution en cours...\n\n");
    printf("  Gen  | Fitness     | Énergie    | Chaleur   | Mutation\n");
    printf("  -----|-------------|------------|-----------|----------\n");

    for (int gen = 0; gen < GENERATIONS; gen++) {

        /* ── Évaluation + tri par île (parallèle) ──────────────────── */
        #pragma omp parallel for schedule(dynamic)
        for (int isl = 0; isl < NUM_ISLANDS; isl++) {
            for (int i = 0; i < POP_SIZE; i++)
                evaluate(islands[isl][i]);
            qsort(islands[isl], POP_SIZE, sizeof(reactor_t*), cmp_fitness_desc);
        }

        /* ── Mise à jour du meilleur global ──────────────────────── */
        for (int isl = 0; isl < NUM_ISLANDS; isl++) {
            if (islands[isl][0]->fitness > global_best->fitness)
                reactor_copy(global_best, islands[isl][0]);
        }

        /* ── Affichage périodique ──────────────────────────────────── */
        if (gen % 50 == 0 || gen == GENERATIONS - 1) {
            printf("  %4d | %11.2f | %10.2f | %9.2f | %.4f\n",
                   gen, global_best->fitness,
                   global_best->energy, global_best->heat, mutation_rate);
            fflush(stdout);
        }

        /* ── Détection stagnation + restart adaptatif ─────────────── */
        if (global_best->fitness > best_fitness_prev + 0.01) {
            best_fitness_prev = global_best->fitness;
            stagnation = 0;
            mutation_rate = MUTATION_RATE;
        } else {
            stagnation++;
            if (stagnation > STAGNATION_LIMIT / 2)
                mutation_rate = fmin(0.40, mutation_rate * 1.05);
        }

        if (stagnation >= STAGNATION_LIMIT) {
            printf("  [Restart partiel à la génération %d]\n", gen);
            /* Remplace la moitié inférieure de chaque île par de nouveaux */
            #pragma omp parallel for schedule(dynamic)
            for (int isl = 0; isl < NUM_ISLANDS; isl++) {
                int tid = isl * POP_SIZE;
                for (int i = POP_SIZE / 2; i < POP_SIZE; i++)
                    randomize(islands[isl][i], tid + i);
            }
            stagnation = 0;
            mutation_rate = MUTATION_RATE;
        }

        /* ── Migration entre îles ──────────────────────────────────── */
        if (gen > 0 && gen % MIGRATION_FREQ == 0) {
            for (int isl = 0; isl < NUM_ISLANDS; isl++) {
                int next_isl = (isl + 1) % NUM_ISLANDS;
                for (int m = 0; m < MIGRATION_N; m++) {
                    /* Envoie les meilleurs de isl vers les pires de next_isl */
                    reactor_copy(islands[next_isl][POP_SIZE - 1 - m],
                                 islands[isl][m]);
                }
            }
        }

        /* ── Recuit simulé sur le meilleur de chaque île ──────────── */
        if (gen % 100 == 0) {
            #pragma omp parallel for schedule(dynamic)
            for (int isl = 0; isl < NUM_ISLANDS; isl++) {
                int tid = isl * POP_SIZE;
                simulated_annealing(islands[isl][0], tid);
                evaluate(islands[isl][0]);
            }
        }

        /* ── Nouvelle génération par île (parallèle) ──────────────── */
        #pragma omp parallel for schedule(dynamic)
        for (int isl = 0; isl < NUM_ISLANDS; isl++) {
            int tid = isl * POP_SIZE;
            reactor_t **pop = islands[isl];

            /* Alloue l'enfant local */
            reactor_t *lchild = reactor_new();

            for (int i = elite_n; i < POP_SIZE; i++) {
                if (tl_randf(tid + i) < CROSSOVER_RATE) {
                    int pa = tournament_select(pop, elite_n + POP_SIZE/2, TOURNAMENT_K, tid+i);
                    int pb = tournament_select(pop, elite_n + POP_SIZE/2, TOURNAMENT_K, tid+i);
                    if (tl_randf(tid+i) < 0.5)
                        acrossover(lchild, pop[pa], pop[pb], tid+i);
                    else
                        crossover_plane(lchild, pop[pa], pop[pb], tid+i);
                } else {
                    int pa = tournament_select(pop, POP_SIZE, TOURNAMENT_K, tid+i);
                    reactor_copy(lchild, pop[pa]);
                }

                /* Mutation */
                if (tl_randf(tid+i) < 0.5)
                    amutate(lchild, mutation_rate, tid+i);
                else
                    mutate_targeted(lchild, mutation_rate, tid+i);

                evaluate(lchild);
                reactor_copy(pop[i], lchild);
            }
            reactor_free(lchild);
        }

        /* Injecte le meilleur global dans chaque île (élitisme global) */
        if (gen % 20 == 0) {
            for (int isl = 0; isl < NUM_ISLANDS; isl++)
                reactor_copy(islands[isl][0], global_best);
        }
    }

    /* ── Recuit simulé final sur le meilleur ──────────────────────────── */
    printf("\nRecuit simulé final sur le meilleur individu...\n");
    simulated_annealing(global_best, 0);
    evaluate(global_best);

    /* ── Libération mémoire ───────────────────────────────────────────── */
    for (int isl = 0; isl < NUM_ISLANDS; isl++) {
        for (int i = 0; i < POP_SIZE; i++)
            reactor_free(islands[isl][i]);
        free(islands[isl]);
    }
    free(islands);

    return global_best;
}


int startAdvGenAlgo() {

    reactor_t *global_best = runAdvGenAlgo();

    /* ── Affichage résultat ───────────────────────────────────────────── */
    print_reactor(global_best);

    /* ── Libération mémoire ───────────────────────────────────────────── */
    
    reactor_free(global_best);

    return 0;
}