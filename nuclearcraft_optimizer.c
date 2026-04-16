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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>

#ifdef _OPENMP
#include <omp.h>
#endif

/* ─── Types de blocs ─────────────────────────────────────────────────── */
/* AIR, STANDARD et WATER retirés :
 *   - Le réacteur est supposé toujours plein (pas de case vide)
 *   - STANDARD : refroidissement trop faible (30-60) pour justifier une case
 *   - WATER     : refroidissement identique à STANDARD, condition bord triviale
 *   => 7 types restants, espace de recherche réduit de ~30%               */
typedef enum {
    FUEL_CELL = 0,
    GRAPHITE,
    GELID_CRYOTHEUM,
    ENDERIUM,
    GLOWSTONE,
    LIQUID_HELIUM,
    REDSTONE,
    NUM_BLOCK_TYPES
} BlockType;

static const char *BLOCK_NAMES[] = {
    "FUEL_CELL", "GRAPHITE", "GELID_CRYOTHEUM",
    "ENDERIUM", "GLOWSTONE", "LIQUID_HELIUM", "REDSTONE"
};

/* ─── Paramètres de l'algorithme ─────────────────────────────────────── */
#define POP_SIZE         200      /* taille population par île */
#define NUM_ISLANDS      4        /* îles parallèles */
#define GENERATIONS      2000     /* générations max */
#define TOURNAMENT_K     5        /* taille tournoi */
#define MUTATION_RATE    0.05     /* taux de mutation initial */
#define CROSSOVER_RATE   0.75     /* taux de croisement */
#define ELITE_RATIO      0.10     /* élite préservée */
#define MIGRATION_FREQ   50       /* migration tous les N gen */
#define MIGRATION_N      5        /* individus migrés */
#define SA_ITERATIONS    500      /* itérations recuit simulé local */
#define SA_TEMP_INIT     1000.0
#define SA_COOLING       0.995
#define STAGNATION_LIMIT 200      /* restart si stagnation */

/* ─── Poids du score de fitness ──────────────────────────────────────── */
/* On maximise energy, on pénalise heat > 0 très lourdement               */
#define HEAT_OVERLOAD_PENALTY  1e9
#define HEAT_CLOSE_TO_ZERO_BONUS_FACTOR 0.001  /* bonus si heat légèrement négatif */

/* ─── Structure réacteur ─────────────────────────────────────────────── */
typedef struct {
    int x, y, z;          /* dimensions intérieures */
    BlockType *grid;       /* taille x*y*z */
    double energy;
    double heat;
    double fitness;
} Reactor;

/* ─── Paramètres globaux ─────────────────────────────────────────────── */
static int GX, GY, GZ;
static int GSIZE;
static double BASE_POWER, BASE_HEAT;

/* ═══════════════════════════════════════════════════════════════════════
 * ACCÈS GRILLE
 * ═══════════════════════════════════════════════════════════════════════ */
static inline int IDX(int x, int y, int z) {
    return x + GX * (y + GY * z);
}

static inline int in_bounds(int x, int y, int z) {
    return (x >= 0 && x < GX && y >= 0 && y < GY && z >= 0 && z < GZ);
}

/* 6 voisins orthogonaux */
static const int DX[6] = {1,-1,0,0,0,0};
static const int DY[6] = {0,0,1,-1,0,0};
static const int DZ[6] = {0,0,0,0,1,-1};

/* ═══════════════════════════════════════════════════════════════════════
 * ÉVALUATION DU RÉACTEUR
 * ═══════════════════════════════════════════════════════════════════════ */

/* Compte les voisins d'un type donné pour la cellule (cx,cy,cz) */
static int count_adjacent(const BlockType *grid, int cx, int cy, int cz, BlockType type) {
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

static void evaluate(Reactor *r) {
    const BlockType *g = r->grid;
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
        BlockType b = g[IDX(x,y,z)];
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
static Reactor *reactor_new(void) {
    Reactor *r = (Reactor*)malloc(sizeof(Reactor));
    r->x = GX; r->y = GY; r->z = GZ;
    r->grid = (BlockType*)malloc(GSIZE * sizeof(BlockType));
    r->energy = r->heat = r->fitness = 0.0;
    return r;
}

static void reactor_free(Reactor *r) {
    free(r->grid);
    free(r);
}

static void reactor_copy(Reactor *dst, const Reactor *src) {
    memcpy(dst->grid, src->grid, GSIZE * sizeof(BlockType));
    dst->energy  = src->energy;
    dst->heat    = src->heat;
    dst->fitness = src->fitness;
}

/* ═══════════════════════════════════════════════════════════════════════
 * GÉNÉRATEUR ALÉATOIRE THREAD-LOCAL
 * ═══════════════════════════════════════════════════════════════════════ */
static unsigned int tl_seed[NUM_ISLANDS * POP_SIZE + 8];

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

/* Probabilités de tirage par type de bloc (heuristique domain-specific) */
/* Pas d'AIR : le réacteur est toujours plein.
 * Pas de STANDARD/WATER : refroidissement trop faible, retirés de l'espace. */
static const double BLOCK_PROBS[NUM_BLOCK_TYPES] = {
    0.25,  /* FUEL_CELL      - cœur du réacteur, favorisé */
    0.12,  /* GRAPHITE       - boosteur d'énergie */
    0.18,  /* GELID_CRYOTHEUM- meilleur refroidisseur isolé */
    0.12,  /* ENDERIUM       - bon si près graphite */
    0.08,  /* GLOWSTONE      - très puissant mais rare (besoin 6 graphites) */
    0.13,  /* LIQUID_HELIUM  - refroidissement constant, fiable */
    0.12,  /* REDSTONE       - bon si près fuel cell */
};

static BlockType random_block(int tid) {
    double r = tl_randf(tid);
    double cum = 0.0;
    for (int i = 0; i < NUM_BLOCK_TYPES; i++) {
        cum += BLOCK_PROBS[i];
        if (r < cum) return (BlockType)i;
    }
    return FUEL_CELL;
}

static void randomize(Reactor *r, int tid) {
    for (int i = 0; i < GSIZE; i++)
        r->grid[i] = random_block(tid);
    evaluate(r);
}

/* Graine heuristique : FUEL_CELL entouré de refroidisseurs */
static void seed_heuristic(Reactor *r, int tid) {
    /* Commence avec tous GELID_CRYOTHEUM (meilleur refroidisseur de base) */
    for (int i = 0; i < GSIZE; i++)
        r->grid[i] = GELID_CRYOTHEUM;

    /* Place des FUEL_CELL de manière espacée */
    int spacing = 2;
    for (int z = 0; z < GZ; z += spacing)
    for (int y = 0; y < GY; y += spacing)
    for (int x = 0; x < GX; x += spacing) {
        if (tl_randf(tid) < 0.5)
            r->grid[IDX(x,y,z)] = FUEL_CELL;
    }

    /* Ajoute des GRAPHITE proches des FUEL_CELL */
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        if (r->grid[IDX(x,y,z)] != FUEL_CELL) {
            if (count_adjacent(r->grid, x,y,z, FUEL_CELL) > 0 && tl_randf(tid) < 0.3)
                r->grid[IDX(x,y,z)] = GRAPHITE;
        }
    }

    /* Quelques REDSTONE près des FUEL_CELL restants */
    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        int idx = IDX(x,y,z);
        if (r->grid[idx] == GELID_CRYOTHEUM &&
            count_adjacent(r->grid, x,y,z, FUEL_CELL) > 0 && tl_randf(tid) < 0.25)
            r->grid[idx] = REDSTONE;
    }
    evaluate(r);
}

/* ═══════════════════════════════════════════════════════════════════════
 * OPÉRATEURS GÉNÉTIQUES
 * ═══════════════════════════════════════════════════════════════════════ */

/* Sélection par tournoi */
static int tournament_select(Reactor **pop, int pop_size, int k, int tid) {
    int best = tl_randi(tid, pop_size);
    for (int i = 1; i < k; i++) {
        int candidate = tl_randi(tid, pop_size);
        if (pop[candidate]->fitness > pop[best]->fitness)
            best = candidate;
    }
    return best;
}

/* Croisement uniforme 3D */
static void crossover(Reactor *child, const Reactor *p1, const Reactor *p2, int tid) {
    for (int i = 0; i < GSIZE; i++)
        child->grid[i] = (tl_randf(tid) < 0.5) ? p1->grid[i] : p2->grid[i];
}

/* Croisement par plan (coupe sur un axe aléatoire) */
static void crossover_plane(Reactor *child, const Reactor *p1, const Reactor *p2, int tid) {
    int axis = tl_randi(tid, 3);
    int cut;
    if (axis == 0)      cut = tl_randi(tid, GX);
    else if (axis == 1) cut = tl_randi(tid, GY);
    else                cut = tl_randi(tid, GZ);

    for (int z = 0; z < GZ; z++)
    for (int y = 0; y < GY; y++)
    for (int x = 0; x < GX; x++) {
        int idx = IDX(x,y,z);
        int val = (axis==0)?x : (axis==1)?y : z;
        child->grid[idx] = (val < cut) ? p1->grid[idx] : p2->grid[idx];
    }
}

/* Mutation : change aléatoirement quelques blocs */
static void mutate(Reactor *r, double rate, int tid) {
    for (int i = 0; i < GSIZE; i++) {
        if (tl_randf(tid) < rate)
            r->grid[i] = random_block(tid);
    }
}

/* Mutation ciblée : si chaleur > 0, remplace des FUEL_CELL par des refroidisseurs */
static void mutate_targeted(Reactor *r, double rate, int tid) {
    if (r->heat > 0) {
        /* Remplace des FUEL_CELL ou blocs peu utiles par des refroidisseurs */
        for (int i = 0; i < GSIZE; i++) {
            if (tl_randf(tid) < rate) {
                BlockType coolers[] = {GELID_CRYOTHEUM, LIQUID_HELIUM, ENDERIUM, REDSTONE, GLOWSTONE};
                r->grid[i] = coolers[tl_randi(tid, 5)];
            }
        }
    } else {
        /* Marge thermique disponible : favorise FUEL_CELL et GRAPHITE */
        for (int i = 0; i < GSIZE; i++) {
            double rnd = tl_randf(tid);
            if (rnd < rate * 0.4)
                r->grid[i] = FUEL_CELL;
            else if (rnd < rate * 0.6)
                r->grid[i] = GRAPHITE;
            else if (rnd < rate)
                r->grid[i] = random_block(tid);
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 * RECUIT SIMULÉ LOCAL
 * ═══════════════════════════════════════════════════════════════════════ */
static void simulated_annealing(Reactor *r, int tid) {
    Reactor *neighbor = reactor_new();
    reactor_copy(neighbor, r);

    double temp = SA_TEMP_INIT;
    Reactor *current = reactor_new();
    reactor_copy(current, r);

    for (int iter = 0; iter < SA_ITERATIONS; iter++) {
        reactor_copy(neighbor, current);

        /* Perturbation : change 1 à 3 blocs aléatoires */
        int n_changes = 1 + tl_randi(tid, 3);
        for (int k = 0; k < n_changes; k++) {
            int idx = tl_randi(tid, GSIZE);
            neighbor->grid[idx] = random_block(tid);
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
    const Reactor *ra = *(const Reactor **)a;
    const Reactor *rb = *(const Reactor **)b;
    if (ra->fitness > rb->fitness) return -1;
    if (ra->fitness < rb->fitness) return  1;
    return 0;
}

/* ═══════════════════════════════════════════════════════════════════════
 * AFFICHAGE
 * ═══════════════════════════════════════════════════════════════════════ */
static void print_reactor(const Reactor *r) {
    printf("\n══════════════════════════════════════════════\n");
    printf("  MEILLEUR RÉACTEUR TROUVÉ\n");
    printf("══════════════════════════════════════════════\n");
    printf("  Dimensions intérieures : %dx%dx%d\n", r->x, r->y, r->z);
    printf("  Énergie produite       : %.2f RF/t\n", r->energy);
    printf("  Chaleur nette          : %.2f H/t\n", r->heat);
    printf("  Score fitness          : %.4f\n", r->fitness);
    printf("══════════════════════════════════════════════\n\n");

    /* Comptage des blocs */
    int counts[NUM_BLOCK_TYPES] = {0};
    for (int i = 0; i < GSIZE; i++)
        counts[(int)r->grid[i]]++;

    printf("  Composition :\n");
    for (int t = 0; t < NUM_BLOCK_TYPES; t++)
        if (counts[t] > 0)
            printf("    %-20s : %d\n", BLOCK_NAMES[t], counts[t]);

    printf("\n  Disposition par couche Z :\n");
    for (int z = 0; z < r->z; z++) {
        printf("  -- Couche Z=%d --\n", z);
        for (int y = 0; y < r->y; y++) {
            printf("    ");
            for (int x = 0; x < r->x; x++) {
                BlockType b = r->grid[IDX(x,y,z)];
                /* Abréviations 2 caractères : ordre = enum BlockType */
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
int main(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <X> <Y> <Z> <base_power> <base_heat>\n", argv[0]);
        fprintf(stderr, "  X,Y,Z       : dimensions intérieures du réacteur\n");
        fprintf(stderr, "  base_power  : puissance de base du combustible\n");
        fprintf(stderr, "  base_heat   : chaleur de base du combustible\n");
        return 1;
    }

    GX = atoi(argv[1]);
    GY = atoi(argv[2]);
    GZ = atoi(argv[3]);
    BASE_POWER = atof(argv[4]);
    BASE_HEAT  = atof(argv[5]);
    GSIZE = GX * GY * GZ;

    if (GX <= 0 || GY <= 0 || GZ <= 0 || GSIZE > 100000) {
        fprintf(stderr, "Erreur : dimensions invalides ou trop grandes (max ~46³)\n");
        return 1;
    }

    printf("NuclearCraft Reactor Optimizer\n");
    printf("Dimensions : %dx%dx%d | base_power=%.1f | base_heat=%.1f\n",
           GX, GY, GZ, BASE_POWER, BASE_HEAT);
    printf("Algorithme : Island Genetic + Simulated Annealing + OpenMP\n\n");

    /* Initialisation des seeds */
    srand((unsigned)time(NULL));
    for (int i = 0; i < NUM_ISLANDS * POP_SIZE + 8; i++)
        tl_seed[i] = (unsigned)rand() ^ ((unsigned)i * 2654435761u);

    /* ── Allocation des populations par île ──────────────────────────── */
    Reactor ***islands = (Reactor***)malloc(NUM_ISLANDS * sizeof(Reactor**));
    for (int isl = 0; isl < NUM_ISLANDS; isl++) {
        islands[isl] = (Reactor**)malloc(POP_SIZE * sizeof(Reactor*));
        for (int i = 0; i < POP_SIZE; i++)
            islands[isl][i] = reactor_new();
    }

    /* Meilleur global */
    Reactor *global_best = reactor_new();
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
    Reactor *child = NULL;
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
            qsort(islands[isl], POP_SIZE, sizeof(Reactor*), cmp_fitness_desc);
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
            Reactor **pop = islands[isl];

            /* Alloue l'enfant local */
            Reactor *lchild = reactor_new();

            for (int i = elite_n; i < POP_SIZE; i++) {
                if (tl_randf(tid + i) < CROSSOVER_RATE) {
                    int pa = tournament_select(pop, elite_n + POP_SIZE/2, TOURNAMENT_K, tid+i);
                    int pb = tournament_select(pop, elite_n + POP_SIZE/2, TOURNAMENT_K, tid+i);
                    if (tl_randf(tid+i) < 0.5)
                        crossover(lchild, pop[pa], pop[pb], tid+i);
                    else
                        crossover_plane(lchild, pop[pa], pop[pb], tid+i);
                } else {
                    int pa = tournament_select(pop, POP_SIZE, TOURNAMENT_K, tid+i);
                    reactor_copy(lchild, pop[pa]);
                }

                /* Mutation */
                if (tl_randf(tid+i) < 0.5)
                    mutate(lchild, mutation_rate, tid+i);
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

    /* ── Affichage résultat ───────────────────────────────────────────── */
    print_reactor(global_best);

    /* ── Libération mémoire ───────────────────────────────────────────── */
    for (int isl = 0; isl < NUM_ISLANDS; isl++) {
        for (int i = 0; i < POP_SIZE; i++)
            reactor_free(islands[isl][i]);
        free(islands[isl]);
    }
    free(islands);
    reactor_free(global_best);

    return 0;
}