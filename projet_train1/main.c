#include "header.h"

#ifdef DEBUG
    bool debug = true;
#else
    bool debug = false;
#endif
#define PRINT_DEBUG(...) if (debug) printf(__VA_ARGS__)

#ifdef TEST
bool test = true;
#else
bool test = false;
#endif
#define PRINT_TEST(...) if (test) fprintf(stderr, __VA_ARGS__)

liaison liaisons [N_STATIONS][N_STATIONS]; //tableau représentant toutes les liaisons entre deux stations
train trains[N_TRAINS];
pthread_mutex_t mutex_fifo = PTHREAD_MUTEX_INITIALIZER; //mutex général pour tous les trains
pthread_cond_t cond_fifo = PTHREAD_COND_INITIALIZER; //attente conditionnelle si un train fait la queue derrière un autre sur la même liaison
double temps_trajet[MAX_SEED][N_TRAJETS][N_TRAINS]; //mesure de tous les temps de trajet


void creer_trajet(unsigned char id_train, const unsigned char trajet[], unsigned char l_trajet) {
    trains[id_train].l_trajet = l_trajet;
    for (unsigned char i = 0; i < l_trajet; ++i) {
        trains[id_train].trajet[i] = trajet[i];
    }
}

void init_reseau(){

    memset(liaisons, 0, sizeof(liaison)*N_STATIONS*N_STATIONS);
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        liaisons[all_liaisons[i][0]][all_liaisons[i][1]].valid = true;
        pthread_mutex_init(&liaisons[all_liaisons[i][0]][all_liaisons[i][1]].mutex, NULL);
    }

    unsigned char l_trajets[N_TRAINS] = {5, 6, 6};
    unsigned char trajets[N_TRAINS][6] = {{A, B, C, B, A}, {A, B, D, C, B, A}, {A, B, D, C, E, A}};
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        trains[i].id = i+(unsigned char)1;
        creer_trajet(i, trajets[i], l_trajets[i]);
    }
}

void clean_mutex(){
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        pthread_mutex_destroy(&liaisons[all_liaisons[i][0]][all_liaisons[i][1]].mutex);
    }

    pthread_mutex_destroy(&mutex_fifo);
    pthread_cond_destroy(&cond_fifo);
}

bool push_fifo(unsigned char station1, unsigned char station2, unsigned char id_train){ //ajoute le train id_train à la file d'attente de la liaison voulue et renvoie vrai s'il est le premier
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        if (liaisons[station1][station2].train_fifo[i] == 0){
            liaisons[station1][station2].train_fifo[i] = id_train;
            return i == 0;
        }
    }
    return false; //ne devrait jamais arriver ici
}

bool pop_fifo(unsigned char station1, unsigned char station2){ //retire le premier train de la file d'attente de la liaison voulue, décale les autres et renvoie vrai s'il était aussi le dernier
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        if (i == N_TRAINS-1) {
            liaisons[station1][station2].train_fifo[i] = 0;
        } else {
            liaisons[station1][station2].train_fifo[i] = liaisons[station1][station2].train_fifo[i + 1];
        }
        if (liaisons[station1][station2].train_fifo[i] == 0){
            return i == 0;
        }
    }
    return false; //ne devrait jamais arriver ici
}

void voyage(unsigned char id_train, unsigned char station1, unsigned char station2) {
    if (!liaisons[station1][station2].valid) {
        PRINT_DEBUG("Le train %d tente d'emprunter la ligne non reliée %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]);
        return;
    }
    pthread_mutex_lock(&mutex_fifo); // évite l'interblocage entre deux trains allant dans des direction opposées
    pthread_mutex_lock(&liaisons[station1][station2].mutex); //verrouille l'accès concurrent à train_fifo des autres trains voulant faire le même trajet
    if (push_fifo(station1, station2, id_train) && liaisons[station2][station1].valid) { //ajoute le train à la file d'attente de cette liaison
        pthread_mutex_lock(&liaisons[station2][station1].mutex); //Si premier train, on verrouille l'accès à la ligne en sens contraire //interblocage évité ici par mutex_fifo
    }
    PRINT_DEBUG("Le train %d s'engage sur la ligne %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]); //les prints se font avant le déverrouillage des mutex pour éviter des affichages incohérents
    pthread_mutex_unlock(&liaisons[station1][station2].mutex);
    pthread_mutex_unlock(&mutex_fifo);
    sleep((unsigned)rand() % 3 + 1);
    pthread_mutex_lock(&liaisons[station1][station2].mutex); //verrouille l'accès concurrent à train_fifo des autres trains voulant faire le même trajet
    while (liaisons[station1][station2].train_fifo[0] != id_train) { //si le train id_train n'est pas le premier de la file d'attente, on déverrouille le mutex et on attend pour laisser passer celui ou ceux d'avant
        pthread_cond_wait(&cond_fifo, &liaisons[station1][station2].mutex);
    }
    if (pop_fifo(station1, station2) && liaisons[station2][station1].valid) { //retire le train de la file d'attente de cette liaison
        pthread_mutex_unlock(&liaisons[station2][station1].mutex); //Si tous les trains sont passés, on déverrouille l'accès à la ligne en sens contraire
    } else if (liaisons[station1][station2].train_fifo[0] != 0){
        pthread_cond_broadcast(&cond_fifo); //sinon on signale aux trains qui attendent derrière de vérifier si c'est à eux de continuer //utilisation précédente de pthread_cond_signal_thread_np, très pratique mais disponible seulement sur mac
    }
    PRINT_DEBUG("Le train %d est arrivé à destination sur la ligne %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]); //les prints se font avant le déverrouillage des mutex pour éviter des affichages incohérents
    pthread_mutex_unlock(&liaisons[station1][station2].mutex);
}

void* roule(void *arg) {
    unsigned char i, j;
    thread_infos infos = *((thread_infos*) arg);
    struct timespec start, stop;

    for (i=0 ; i < N_TRAJETS ; i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);
        for (j=1 ; j < trains[infos.index_train].l_trajet ; j++) {
            voyage(trains[infos.index_train].id, trains[infos.index_train].trajet[j-1], trains[infos.index_train].trajet[j]);
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
        temps_trajet[infos.seed-1][i][infos.index_train] = (stop.tv_sec - start.tv_sec) + (stop.tv_nsec - start.tv_nsec) / 1e9; //calcul du temps mis pour un trajet en secondes
    }

    return NULL;
}

void affiche_tests_results(){
    for (unsigned char index_train = 0 ; index_train < N_TRAINS ; ++index_train) {
        double moyenne = 0, ecart_type = 0, tmp;
        for (unsigned short seed = 0; seed < MAX_SEED; ++seed) {
            for (unsigned char index_trajet = 0; index_trajet < N_TRAJETS; ++index_trajet) {
                moyenne += temps_trajet[seed][index_trajet][index_train];
            }
        }
        moyenne /= N_TRAJETS+MAX_SEED;
        PRINT_TEST("\nMoyenne trajet train %d = %f sec\n", trains[index_train].id, moyenne);
        for (unsigned short seed = 0; seed < MAX_SEED; ++seed) {
            for (unsigned char index_trajet = 0; index_trajet < N_TRAJETS; ++index_trajet) {
                tmp = moyenne - temps_trajet[seed][index_trajet][index_train];
                ecart_type += tmp * tmp;
            }
        }
        ecart_type /= N_TRAJETS+MAX_SEED;
        ecart_type = sqrt(ecart_type);
        PRINT_TEST("Écart type trajet train %d = %f sec\n", trains[index_train].id, ecart_type);
    }
}

void handle_sig(int sig){
    signal(sig, SIG_IGN); //pas d'appel au handler lorsqu'il traite déjà un signal de ce type
    clean_mutex();
    exit(EXIT_SUCCESS);
}

int main() {
    unsigned short i;
    unsigned char j;
    thread_infos infos[N_TRAINS];
    struct sigaction act;
    pthread_t thread_ids[N_TRAINS];

    memset(&act, '\0', sizeof(act));
    act.sa_handler = handle_sig;

    init_reseau();
    sigaction(SIGINT, &act, NULL);
    PRINT_TEST("Tests pour %d trajets, pour des seeds allant de 1 à %d\n\n", N_TRAJETS, MAX_SEED);

    for (i = 1 ; i <= MAX_SEED ; i++) {
        srand(i);
        PRINT_TEST("Mesurements started for srand(%d)\n", i);
        for (j = 0; j < N_TRAINS; j++) {
            infos[j].seed = i;
            infos[j].index_train = j;
            if (pthread_create(&thread_ids[j], NULL, roule, infos + j) != 0) {
                perror("THREAD ERROR : ");
                for (unsigned char k = 0; k < j; k++) {
                    pthread_cancel(thread_ids[k]);
                }
                clean_mutex();
                exit(EXIT_FAILURE);
            }
        }
        for (j = 0; j < N_TRAINS; j++) {
            pthread_join(thread_ids[j], NULL);
        }
    }

    affiche_tests_results();
    clean_mutex();
    return 0;
}