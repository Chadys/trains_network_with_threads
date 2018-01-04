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
sem_t *sem_fifo;


void relier_stations(unsigned char station1, unsigned char station2) {
    liaisons[station1][station2].valid = 1;
    sem_engage_names[0] = noms_stations[station1];
    sem_engage_names[1] = noms_stations[station2];
    liaisons[station1][station2].sem_engage = sem_open(sem_engage_names, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_arrive_names[0] = noms_stations[station1];
    sem_arrive_names[1] = noms_stations[station2];
    liaisons[station1][station2].sem_arrive = sem_open(sem_arrive_names, O_CREAT, S_IRUSR | S_IWUSR, 0);
}

void creer_trajet(unsigned char id_train, const unsigned char trajet[], unsigned char l_trajet) {
    trains[id_train].l_trajet = l_trajet;
    for (unsigned char i = 0; i < l_trajet; ++i) {
        trains[id_train].trajet[i] = trajet[i];
    }
}

void init_reseau(){
    sem_fifo = sem_open(sem_fifo_name, O_CREAT, S_IRUSR | S_IWUSR, 1);

    memset(liaisons, 0, sizeof(liaison)*N_STATIONS*N_STATIONS);
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        relier_stations(all_liaisons[i][0], all_liaisons[i][1]);
    }

    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        trains[i].id = i+(unsigned char)1;
        creer_trajet(i, trajets[i], l_trajets[i]);
        sem_train_names[0] = i+(unsigned char)1;
        trains[i].sem_arrivee = sem_open(sem_train_names, O_CREAT, S_IRUSR | S_IWUSR, 0);
    }
}

void clean_semaphore(unsigned char station1, unsigned char station2) {
    sem_close(liaisons[station1][station2].sem_engage);
    sem_engage_names[0] = noms_stations[station1];
    sem_engage_names[1] = noms_stations[station2];
    sem_unlink(sem_engage_names);
    sem_close(liaisons[station1][station2].sem_arrive);
    sem_arrive_names[0] = noms_stations[station1];
    sem_arrive_names[1] = noms_stations[station2];
    sem_unlink(sem_arrive_names);
}

void clean_semaphores(){
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        clean_semaphore(all_liaisons[i][0], all_liaisons[i][1]);
    }
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        sem_close(trains[i].sem_arrivee);
        sem_train_names[0] = i+(unsigned char)1;
        sem_unlink(sem_train_names);
    }

    sem_close(sem_fifo);
    sem_unlink(sem_fifo_name);
}

void push_fifo(unsigned char station1, unsigned char station2, unsigned char id_train, unsigned char temps_trajet){ //ajoute le train id_train à la file d'attente de la liaison voulue avec le temps de trajet qui lui correspond
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        if (liaisons[station1][station2].train_fifo[i].id_train == 0){
            liaisons[station1][station2].train_fifo[i].id_train = id_train;
            liaisons[station1][station2].train_fifo[i].temps_trajet = temps_trajet;
            return;
        }
    }
}

void voyage(unsigned char id_train, unsigned char station1, unsigned char station2) {
    if (!liaisons[station1][station2].valid) {
        PRINT_DEBUG("Le train %d tente d'emprunter la ligne non reliée %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]);
        return;
    }
    sem_wait(sem_fifo); // évite l'interblocage entre deux trains allant dans des direction opposées
    sem_wait(liaisons[station1][station2].sem_engage); //verrouille l'accès concurrent à train_fifo et nb_train des autres trains voulant faire le même trajet
    if (liaisons[station1][station2].nb_trains == 0 && liaisons[station2][station1].valid) { //si premier train //on ne peux pas utiliser train fifo car risque de concurrence puisqu'elle est manipulée par gere_arrivee_gare, un train dans le mauvais sens risquerait de passer si elle est vidée juste après qu'un train ne passe ce block d'instructions
        sem_wait(liaisons[station2][station1].sem_engage); // on verrouille l'accès à la ligne en sens contraire //interblocage évité ici par sem_fifo
    }
    liaisons[station1][station2].nb_trains++;
    PRINT_DEBUG("Le train %d s'engage sur la ligne %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]); //les prints se font avant le déverrouillage des sémaphores pour éviter des affichages incohérents
    sem_post(liaisons[station1][station2].sem_engage);
    sem_post(sem_fifo);

    unsigned char temps_trajet = (unsigned char)rand() % (unsigned char)3 + (unsigned char)1;

    sem_wait(liaisons[station1][station2].sem_engage); //verrouille l'accès concurrent à train_fifo des autres trains voulant faire le même trajet
    push_fifo(station1, station2, id_train, temps_trajet);
    sem_post(liaisons[station1][station2].sem_arrive); //indique au thread gérant cette liaison qu'un train doit être pris en charge
    sem_post(liaisons[station1][station2].sem_engage);
    sem_wait(trains[id_train-1].sem_arrivee); //attend que le thread qui gère la liaison ait indiqué que ce train est arrivé

    sem_wait(liaisons[station1][station2].sem_engage); //verrouille l'accès concurrent à nb_train des autres trains voulant faire le même trajet
    PRINT_DEBUG("Le train %d est arrivé à destination sur la ligne %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]); //les prints se font avant le déverrouillage des sémaphores pour éviter des affichages incohérents
    liaisons[station1][station2].nb_trains--;
    if (liaisons[station1][station2].nb_trains == 0 && liaisons[station2][station1].valid) { // Si dernier train
        sem_post(liaisons[station2][station1].sem_engage); // on déverrouille l'accès à la ligne en sens contraire
    }
    sem_post(liaisons[station1][station2].sem_engage);
}

void* gere_arrivee_gare(void *arg) {
    trajet train_fifo[N_TRAINS];
    liaison *liaison = arg;
    unsigned char i;

    for(;;) { //boucle sans fin, s'arrêtera quand on dira au thread qui fait tourner cette fonction de s'arrêter
        char temps_attente = 0;
        unsigned char temps_total = 0;

        sem_wait(liaison->sem_arrive); //attend qu'au moins un train arrive à cette liaison
        sem_wait(liaison->sem_engage); //évite concurrence sur train_fifo
        for (i = 0; i < N_TRAINS; i++) { //récupère tous les trains actuellement sur la file d'attente et la vide
            train_fifo[i] = liaison->train_fifo[i];
            liaison->train_fifo[i].id_train = 0;
        }
        sem_post(liaison->sem_engage);
        for (i = 0; i < N_TRAINS && train_fifo[i].id_train != 0 ; i++) {
            if (i != 0){
                sem_wait(liaison->sem_arrive); //décrémente correctement le sémaphore, car chaque push_fifo a forcément été associé à un post(sem_arrive), tout en gérant efficacement les temps de trajet
            }
            temps_attente = train_fifo[i].temps_trajet-temps_total;
            if (temps_attente > 0) {
                temps_total += temps_attente;
                sleep((unsigned)temps_attente); //attend le temps qui correspond au trajet du train, moins le temps de trajet déjà effectué si des trains se suivent
            }
            sem_post(trains[train_fifo[i].id_train-1].sem_arrivee); //préviens le train correspondant qu'il est arrivé à destination
        }
    }
    return NULL;
}


void* roule(void *arg) {
    unsigned short i;
    unsigned char j;
    unsigned char id = *((unsigned char*) arg);
    struct timespec start, stop;
    double times[N_TRAJETS], moyenne = 0, ecart_type = 0, tmp;

    for (i=0 ; i < N_TRAJETS ; i++) {
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
        for (j=1 ; j < trains[id].l_trajet ; j++) {
            voyage(trains[id].id, trains[id].trajet[j-1], trains[id].trajet[j]);
        }
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);
        times[i] = (stop.tv_sec - start.tv_sec) * 1e6 + (stop.tv_nsec - start.tv_nsec);
    }

    for (i=0 ; i < N_TRAJETS ; i++) {
        moyenne += times[i];
    }
    moyenne /= N_TRAJETS;
    PRINT_TEST("Moyenne train %d = %f\n", trains[id].id, moyenne);

    for (i=0 ; i < N_TRAJETS ; i++) {
        tmp = moyenne - times[i];
        ecart_type += tmp * tmp;
    }
    ecart_type /= N_TRAJETS;
    ecart_type = sqrt(ecart_type);
    PRINT_TEST("Écart type train %d = %f\n", trains[id].id, ecart_type);

    return NULL;
}

void handle_sig(int sig){
    signal(sig, SIG_IGN); //pas d'appel au handler lorsqu'il traite déjà un signal de ce type
    clean_semaphores();
    exit(EXIT_SUCCESS);
}

int main() {
    unsigned char i, j, k;
    unsigned char index_trains[N_TRAINS];
    struct sigaction act;
    pthread_t thread_train_ids[N_TRAINS], thread_liaison_ids[N_LIAISONS];

    memset(&act, '\0', sizeof(act));
    act.sa_handler = handle_sig;

    init_reseau();
    sigaction(SIGINT, &act, NULL);
    for (i = 1 ; i < 1001 ; i++) {
        srand(i);
        PRINT_TEST("srand(%d)\n", i);
        for (j = 0; j < N_TRAINS; j++) {
            index_trains[j] = j;
            if (pthread_create(&thread_train_ids[j], NULL, roule, index_trains + j) != 0) {
                perror("THREAD ERROR : ");
                for (k = 0; k < j; k++) {
                    pthread_cancel(thread_train_ids[k]);
                }
                clean_semaphores();
                exit(EXIT_FAILURE);
            }
        }

        for (j = 0; j < N_LIAISONS; j++) {
            if (pthread_create(&thread_liaison_ids[j], NULL, gere_arrivee_gare,
                               liaisons[all_liaisons[j][0]] + all_liaisons[j][1]) != 0) {
                perror("THREAD ERROR : ");
                for (k = 0; k < j; k++) {
                    pthread_cancel(thread_liaison_ids[k]);
                }
                for (k = 0; k < N_TRAINS; k++) {
                    pthread_cancel(thread_train_ids[k]);
                }
                clean_semaphores();
                exit(EXIT_FAILURE);
            }
        }
        for (j = 0; j < N_TRAINS; j++) {
            pthread_join(thread_train_ids[j], NULL);
        }
        for (j = 0; j < N_LIAISONS; j++) {
            pthread_cancel(
                    thread_liaison_ids[j]); //Ces threads font une boucle infinie, les arrêter car leur travail est fini
        }
    }

    clean_semaphores();
    return 0;
}