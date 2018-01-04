//
// Created by Julie on 29/12/2017.
//

#ifndef PROJET_TRAIN_HEADER_H
#define PROJET_TRAIN_HEADER_H

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#define N_STATIONS 5
#define N_TRAINS 3
#define N_TRAJETS 10
#define MAX_L_TRAJET 6
#define N_LIAISONS 11

enum {
    A,
    B,
    C,
    D,
    E
};
const unsigned char noms_stations [N_STATIONS] = {'A', 'B', 'C', 'D', 'E'};
const unsigned char all_liaisons [N_LIAISONS][2] = {{A, B}, {B, A}, {B, C}, {C, B}, {B, D}, {D, B}, {D, C}, {C, E}, {E, C}, {E, A}, {A, E}};
const unsigned char l_trajets[N_TRAINS] = {5, 6, 6};
const unsigned char trajets[N_TRAINS][MAX_L_TRAJET] = {{A, B, C, B, A}, {A, B, D, C, B, A}, {A, B, D, C, E, A}};

typedef struct liaison {
    char valid;
    pthread_mutex_t mutex;
    unsigned char train_fifo[N_TRAINS];
} liaison;

typedef struct train {
    unsigned char id;
    unsigned char trajet[MAX_L_TRAJET];
    unsigned char l_trajet;
} train;

#endif //PROJET_TRAIN_HEADER_H
