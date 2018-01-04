//
// Created by Julie on 30/12/2017.
//

#ifndef PROJET_TRAIN2_HEADER_H
#define PROJET_TRAIN2_HEADER_H

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

#define N_STATIONS 5
#define N_TRAINS 3
#define N_TRAJETS 10
#define MAX_L_TRAJET 6
#define N_LIAISONS 11
#define SEM_LIAISON_NAME_SIZE 15
#define SEM_TRAIN_NAME_SIZE 12

const char * sem_fifo_name = "Fifo_sem";
char sem_train_names[SEM_TRAIN_NAME_SIZE] = "\0_train_sem";
char sem_engage_names[SEM_LIAISON_NAME_SIZE] = "\0\0_engage_sem";
char sem_arrive_names[SEM_LIAISON_NAME_SIZE] = "\0\0_arrive_sem";

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

typedef struct trajet {
    unsigned char id_train;
    unsigned char temps_trajet;
} trajet;

typedef struct liaison {
    char valid;
    sem_t *sem_engage;
    sem_t *sem_arrive;
    trajet train_fifo[N_TRAINS];
    unsigned char nb_trains;
} liaison;

typedef struct train {
    unsigned char id;
    unsigned char trajet[MAX_L_TRAJET];
    unsigned char l_trajet;
    sem_t *sem_arrivee;
} train;

#endif //PROJET_TRAIN2_HEADER_H
