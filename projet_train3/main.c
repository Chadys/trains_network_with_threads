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
pthread_rwlock_t rwlock_fifo;
double temps_trajet[MAX_SEED][N_TRAJETS][N_TRAINS]; //mesure de tous les temps de trajet


void relier_stations(unsigned char station1, unsigned char station2) {
    liaisons[station1][station2].valid = 1;
    pthread_rwlock_init(&liaisons[station1][station2].rwlock_train_statut, NULL);
    pthread_rwlock_init(&liaisons[station1][station2].rwlock_train_deplace, NULL);
}

void creer_trajet(unsigned char id_train, const unsigned char trajet[], unsigned char l_trajet) {
    trains[id_train].l_trajet = l_trajet;
    for (unsigned char i = 0; i < l_trajet; ++i) {
        trains[id_train].trajet[i] = trajet[i];
    }
}

void init_reseau(){
    pthread_rwlock_init(&rwlock_fifo, NULL);

    memset(liaisons, 0, sizeof(liaison)*N_STATIONS*N_STATIONS);
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        relier_stations(all_liaisons[i][0], all_liaisons[i][1]);
    }

    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        trains[i].id = i+(unsigned char)1;
        creer_trajet(i, trajets[i], l_trajets[i]);
    }
}

void clean_rwlock(unsigned char station1, unsigned char station2) {
    pthread_rwlock_destroy(&liaisons[station1][station2].rwlock_train_statut);
    pthread_rwlock_destroy(&liaisons[station1][station2].rwlock_train_deplace);
}

void clean_rwlocks(){
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        clean_rwlock(all_liaisons[i][0], all_liaisons[i][1]);
    }

    pthread_rwlock_destroy(&rwlock_fifo);
}

void push_fifo(unsigned char station1, unsigned char station2, unsigned char id_train){ //ajoute le train id_train à la file d'attente de la liaison voulue
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        if (liaisons[station1][station2].train_fifo[i].id_train == 0){
            liaisons[station1][station2].train_fifo[i].id_train = id_train;
            liaisons[station1][station2].train_fifo[i].arrive = false;
            return;
        }
    }
}

void update_status_fifo(unsigned char station1, unsigned char station2, unsigned char id_train){ //marque comme arrivé le train id_train dans la file d'attente de la liaison voulue
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        if (liaisons[station1][station2].train_fifo[i].id_train == id_train){
            liaisons[station1][station2].train_fifo[i].arrive = true;
            return;
        }
    }
}

bool gere_arrivee_gare(unsigned char station1, unsigned char station2, unsigned char id_train) {
    bool arrive = false;
    while (liaisons[station1][station2].train_fifo[0].id_train != 0 && liaisons[station1][station2].train_fifo[0].arrive) { //tant qu'il y a un train et que le premier est arrivé
        PRINT_DEBUG("Le train %d est arrivé à destination sur la ligne %c-%c\n", liaisons[station1][station2].train_fifo[0].id_train, noms_stations[station1], noms_stations[station2]);
        if(liaisons[station1][station2].train_fifo[0].id_train == id_train) { //si le train du thread appelant est arrivé, lui indiquer
            arrive = true;
        }
        for (unsigned char i = 0 ; i < N_TRAINS ; i++) { // décaler la fifo d'un cran
            if (i == N_TRAINS-1) {
                liaisons[station1][station2].train_fifo[i].id_train = 0;
            } else {
                liaisons[station1][station2].train_fifo[i] = liaisons[station1][station2].train_fifo[i + 1];
            }
            if (liaisons[station1][station2].train_fifo[i].id_train == 0){
                break;
            }
        }
    }
    return arrive;
}

void voyage(unsigned char id_train, unsigned char station1, unsigned char station2) {
    bool arrive;

    if (!liaisons[station1][station2].valid) {
        PRINT_DEBUG("Le train %d tente d'emprunter la ligne non reliée %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]);
        return;
    }

    pthread_rwlock_wrlock(&rwlock_fifo); // évite l'interblocage entre deux trains allant dans des direction opposées
    pthread_rwlock_wrlock(&liaisons[station1][station2].rwlock_train_statut); //verrouille l'accès concurrent à train_fifo des autres trains voulant faire le même trajet
    if (liaisons[station1][station2].train_fifo[0].id_train == 0 && liaisons[station2][station1].valid) { //Si premier train, on verrouille l'accès à la ligne en sens contraire //interblocage évité ici par rwlock_fifo
        pthread_rwlock_wrlock(&liaisons[station2][station1].rwlock_train_deplace); //verrou en écriture, bloquera tous les autres
    }
    pthread_rwlock_rdlock(&liaisons[station1][station2].rwlock_train_deplace); //indique su'un train roule sur ce trajet, verrou en lecture non bloquant pour les autres
    push_fifo(station1, station2, id_train); //marque le train id_train comme en train de voyager à sa bonne place
    PRINT_DEBUG("Le train %d s'engage sur la ligne %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]);
    pthread_rwlock_unlock(&liaisons[station1][station2].rwlock_train_statut);
    pthread_rwlock_unlock(&rwlock_fifo);

    sleep((unsigned char)rand() % (unsigned char)3 + (unsigned char)1);

    pthread_rwlock_wrlock(&liaisons[station1][station2].rwlock_train_statut); //verrouille l'accès concurrent à train_fifo des autres trains voulant faire le même trajet
    update_status_fifo(station1, station2, id_train); // marque le train id_train comme étant arrivé
    arrive = gere_arrivee_gare(station1, station2, id_train); //fait arriver les trains qui le sont (s'il ne sont pas derrière un train qui ne l'est pas) et indique au train appelant s'il est arrivé
    if (liaisons[station1][station2].train_fifo[0].id_train == 0 && liaisons[station2][station1].valid) { //Si dernier train
        pthread_rwlock_unlock(&liaisons[station2][station1].rwlock_train_deplace); // on déverrouille l'accès à la ligne en sens contraire
    }
    pthread_rwlock_unlock(&liaisons[station1][station2].rwlock_train_statut);
    pthread_rwlock_unlock(&liaisons[station1][station2].rwlock_train_deplace); //ce train ne se déplace plus
    if(!arrive) { //Si ce train n'est pas arrivé, bloque jusqu'à ce que tous les trains sur ce trajet soient arrivés pour être sûr de ne pas continuer à rouler tant que le train devant lui ne soit lui aussi arrivé
        pthread_rwlock_wrlock(&liaisons[station1][station2].rwlock_train_deplace);
        pthread_rwlock_unlock(&liaisons[station1][station2].rwlock_train_deplace); //le débloque immédiatement car uniquement utile pour attendre
    }
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
    clean_rwlocks();
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
                clean_rwlocks();
                exit(EXIT_FAILURE);
            }
        }

        for (j = 0; j < N_TRAINS; j++) {
            pthread_join(thread_ids[j], NULL);
        }
    }

    affiche_tests_results();
    clean_rwlocks();
    return 0;
}