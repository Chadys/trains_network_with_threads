//
// Created by Julie on 03/01/2018.
//

#ifndef PROJET_TRAIN4_HEADER_H
#define PROJET_TRAIN4_HEADER_H

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <mqueue.h>
#include <time.h>
#include <math.h>

#define N_STATIONS 5
#define N_TRAINS 3
#define N_TRAJETS 10
#define MAX_L_TRAJET 6
#define N_LIAISONS 11
#define MQUEUE_LIAISON_NAME_SIZE 18
#define MQUEUE_TRAIN_NAME_SIZE 15
#define MAX_SEED 1000

const char * mqueue_fifo_name = "Fifo_mqueue";
char mqueue_train_start_names[MQUEUE_TRAIN_NAME_SIZE] = "\0_start_mqueue";
char mqueue_train_ended_names[MQUEUE_TRAIN_NAME_SIZE] = "\0_ended_mqueue";
char mqueue_engage_names[MQUEUE_LIAISON_NAME_SIZE] = "\0\0_engage_mqueue";
char mqueue_statut_names[MQUEUE_LIAISON_NAME_SIZE] = "\0\0_statut_mqueue";

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
    bool valid;
    mqd_t mqueue_status;
    mqd_t mqueue_engage;
    unsigned char nb_trains;
} liaison;

typedef struct train {
    unsigned char id;
    unsigned char trajet[MAX_L_TRAJET];
    unsigned char l_trajet;
    mqd_t mqueue_arrive_start;
    mqd_t mqueue_arrive_ended;
} train;

typedef struct thread_infos {
    unsigned char index_train;
    unsigned short seed;
} thread_infos;

#endif //PROJET_TRAIN4_HEADER_H
