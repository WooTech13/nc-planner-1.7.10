#ifndef ADVGENALGO_H
#define ADVGENALGO_H
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

#include "common.h"

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

/* ═══════════════════════════════════════════════════════════════════════
 * ACCÈS GRILLE
 * ═══════════════════════════════════════════════════════════════════════ */
static inline int IDX(int x, int y, int z);

static inline int in_bounds(int x, int y, int z);

/* 6 voisins orthogonaux */
static const int DX[6] = {1,-1,0,0,0,0};
static const int DY[6] = {0,0,1,-1,0,0};
static const int DZ[6] = {0,0,0,0,1,-1};

static inline int canon(int v, int dim);

static void apply_symmetry(blockType_t *g);

static inline int is_canonical(int x, int y, int z);

/* ═══════════════════════════════════════════════════════════════════════
 * ÉVALUATION DU RÉACTEUR
 * ═══════════════════════════════════════════════════════════════════════ */

/* Compte les voisins d'un type donné pour la cellule (cx,cy,cz) */
static int count_adjacent(const blockType_t *grid, int cx, int cy, int cz, blockType_t type);

static void evaluate(reactor_t *r);

/* ═══════════════════════════════════════════════════════════════════════
 * ALLOCATION / LIBÉRATION
 * ═══════════════════════════════════════════════════════════════════════ */
static reactor_t *reactor_new(void);

static void reactor_free(reactor_t *r);

static void reactor_copy(reactor_t *dst, const reactor_t *src);

/* ═══════════════════════════════════════════════════════════════════════
 * GÉNÉRATEUR ALÉATOIRE THREAD-LOCAL
 * ═══════════════════════════════════════════════════════════════════════ */
static unsigned int tl_seed[NUM_ISLANDS * POP_SIZE + 8];

static inline unsigned int tl_rand(int tid);

static inline double tl_randf(int tid);

static inline int tl_randi(int tid, int n);

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

static blockType_t random_block(int tid);

static void randomize(reactor_t *r, int tid);

/* Graine heuristique : FUEL_CELL entouré de refroidisseurs */
static void seed_heuristic(reactor_t *r, int tid);

/* ═══════════════════════════════════════════════════════════════════════
 * OPÉRATEURS GÉNÉTIQUES
 * ═══════════════════════════════════════════════════════════════════════ */

/* Sélection par tournoi */
static int tournament_select(reactor_t **pop, int pop_size, int k, int tid);

/* Croisement uniforme — ne touche que la zone canonique */
static void acrossover(reactor_t *child, const reactor_t *p1, const reactor_t *p2, int tid);

/* Croisement par plan — ne touche que la zone canonique */
static void crossover_plane(reactor_t *child, const reactor_t *p1, const reactor_t *p2, int tid);

/* Mutation simple — zone canonique uniquement */
static void amutate(reactor_t *r, double rate, int tid);

/* Mutation ciblée — zone canonique uniquement */
static void mutate_targeted(reactor_t *r, double rate, int tid);

/* ═══════════════════════════════════════════════════════════════════════
 * RECUIT SIMULÉ LOCAL
 * ═══════════════════════════════════════════════════════════════════════ */
static void simulated_annealing(reactor_t *r, int tid);

/* ═══════════════════════════════════════════════════════════════════════
 * COMPARATEUR POUR TRI
 * ═══════════════════════════════════════════════════════════════════════ */
static int cmp_fitness_desc(const void *a, const void *b);

/* ═══════════════════════════════════════════════════════════════════════
 * AFFICHAGE
 * ═══════════════════════════════════════════════════════════════════════ */
static void print_reactor(const reactor_t *r);

/* ═══════════════════════════════════════════════════════════════════════
 * ALGORITHME PRINCIPAL : ISLAND GENETIC ALGORITHM
 * ═══════════════════════════════════════════════════════════════════════ */
static reactor_t* runAdvGenAlgo();

int startAdvGenAlgo();

#endif