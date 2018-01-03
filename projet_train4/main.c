#include "header.h"

#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif
#define PRINT(...) if (debug) printf(__VA_ARGS__)

liaison liaisons [N_STATIONS][N_STATIONS]; //tableau représentant toutes les liaisons entre deux stations
train trains[N_TRAINS];
mqd_t mqueue_fifo;

struct mq_attr attr;
attr.mq_flags = 0;
attr.mq_maxmsg = N_TRAINS; //jamais plus de N_TRAINS trains au même endroit
attr.mq_msgsize = 1; //seul l'id du train sera lu
attr.mq_curmsgs = 0;

void relier_stations(unsigned char station1, unsigned char station2) {
    liaisons[station1][station2].valid = 1;
    mqueue_statut_names[0] = noms_stations[station1];
    mqueue_statut_names[1] = noms_stations[station2];
    liaisons[station1][station2].mqueue_status = mq_open(mqueue_statut_names, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    mq_send(liaisons[station1][station2].mqueue_status, 'Y', 1, 0); //pour que le premier receive ne bloque pas
    mqueue_engage_names[0] = noms_stations[station1];
    mqueue_engage_names[1] = noms_stations[station2];
    liaisons[station1][station2].mqueue_engage = mq_open(mqueue_engage_names, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
}

void creer_trajet(unsigned char id_train, const unsigned char trajet[], unsigned char l_trajet) {
    trains[id_train].l_trajet = l_trajet;
    for (unsigned char i = 0; i < l_trajet; ++i) {
        trains[id_train].trajet[i] = trajet[i];
    }
}

void init_reseau(){
    mqueue_fifo = mq_open(mqueue_fifo_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    mq_send(mqueue_fifo, 'Y', 1, 0); //pour que le premier receive ne bloque pas

    memset(liaisons, 0, sizeof(liaison)*N_STATIONS*N_STATIONS);
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        relier_stations(all_liaisons[i][0], all_liaisons[i][1]);
    }

    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        trains[i].id = i+(unsigned char)1;
        creer_trajet(i, trajets[i], l_trajets[i]);
        mqueue_train_start_names[0] = i+(unsigned char)1;
        trains[i].mqueue_arrive_start = mq_open(mqueue_train_start_names, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
        mqueue_train_ended_names[0] = i+(unsigned char)1;
        trains[i].mqueue_arrive_ended = mq_open(mqueue_train_ended_names, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attr);
    }
}

void clean_semaphore(unsigned char station1, unsigned char station2) {
    mq_close(liaisons[station1][station2].mqueue_status);
    mqueue_statut_names[0] = noms_stations[station1];
    mqueue_statut_names[1] = noms_stations[station2];
    mq_unlink(mqueue_statut_names);
    mq_close(liaisons[station1][station2].mqueue_engage);
    mqueue_engage_names[0] = noms_stations[station1];
    mqueue_engage_names[1] = noms_stations[station2];
    mq_unlink(mqueue_engage_names);
}

void clean_semaphores(){
    for (unsigned char i = 0; i < N_LIAISONS; ++i) {
        clean_semaphore(all_liaisons[i][0], all_liaisons[i][1]);
    }
    for (unsigned char i = 0 ; i < N_TRAINS ; i++) {
        mq_close(trains[i].mqueue_arrive_start);
        mqueue_train_start_names[0] = i+(unsigned char)1;
        mq_unlink(mqueue_train_start_names);
        mq_close(trains[i].mqueue_arrive_ended);
        mqueue_train_ended_names[0] = i+(unsigned char)1;
        mq_unlink(mqueue_train_ended_names);
    }

    mq_close(mqueue_fifo);
    mq_unlink(mqueue_fifo_name);
}

void voyage(unsigned char id_train, unsigned char station1, unsigned char station2) {
    if (!liaisons[station1][station2].valid) {
        PRINT("Le train %d tente d'emprunter la ligne non reliée %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]);
        return;
    }
    char msg;

    mq_receive(mqueue_fifo, &msg, 1, NULL); // évite l'interblocage entre deux trains allant dans des direction opposées //le contenu du message nous importe peu, la présence du message seul est suffisant
    mq_receive(liaisons[station1][station2].mqueue_status, &msg, 1, NULL); //verrouille l'accès concurrent à nb_train des autres trains voulant faire le même trajet
    if (liaisons[station1][station2].nb_trains == 0) { //si premier train //on ne peux pas utiliser train fifo car risque de concurrence puisqu'elle est manipulée par gere_arrivee_gare, un train dans le mauvais sens risquerait de passer si elle est vidée juste après qu'un train ne passe ce block d'instructions
        mq_receive(liaisons[station2][station1].mqueue_status, &msg, 1, NULL); // on verrouille l'accès à la ligne en sens contraire //interblocage évité ici par mqueue_fifo
    }
    liaisons[station1][station2].nb_trains++;
    PRINT("Le train %d s'engage sur la ligne %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]); //les prints se font avant le déverrouillage des sémaphores pour éviter des affichages incohérents
    mq_send(liaisons[station1][station2].mqueue_engage, id_train, 1, 0); //préviens le thread qui gère la liaison que le train id_train s'est engagé sur cette liaison
    mq_send(liaisons[station1][station2].mqueue_status, 'Y', 1, 0); //débloque le prochain receive
    mq_send(mqueue_fifo, 'Y', 1, 0);

    sleep((unsigned char)rand() % (unsigned char)3 + (unsigned char)1);


    mq_send(trains[id_train-1].mqueue_arrive_start, 'Y', 1, 0); //préviens le thread qui gère la liaison que ce train est arrivé
    mq_receive(trains[id_train-1].mqueue_arrive_ended, &msg, 1, NULL); //attend que le thread qui gère la liaison ait indiqué que ce train est arrivé

    mq_receive(liaisons[station1][station2].mqueue_status, &msg, 1, NULL); //verrouille l'accès concurrent à nb_train des autres trains voulant faire le même trajet
    PRINT("Le train %d est arrivé à destination sur la ligne %c-%c\n", id_train, noms_stations[station1], noms_stations[station2]); //les prints se font avant le déverrouillage des sémaphores pour éviter des affichages incohérents
    liaisons[station1][station2].nb_trains--;
    if (liaisons[station1][station2].nb_trains == 0) { // Si dernier train
        mq_send(liaisons[station2][station1].mqueue_status, 'Y', 1, 0); // on déverrouille l'accès à la ligne en sens contraire
    }
    mq_send(liaisons[station1][station2].mqueue_status, 'Y', 1, 0);
}

void* gere_arrivee_gare(void *arg) {
    liaison *liaison = arg;
    char id_train, msg;

    for(;;) { //boucle sans fin, s'arrêtera quand on dira au thread qui fait tourner cette fonction de s'arrêter
        mq_receive(liaison->mqueue_engage, &id_train, 1, NULL); //reçoit l'id du train engager à cette liaison, dans l'ordre
        mq_receive(trains[id_train-1].mqueue_arrive_start, &msg, 1, NULL); //attend que le train correspondant arrive
        mq_send(trains[id_train-1].mqueue_arrive_ended, 'Y', 1, 0); //préviens le train correspondant qu'il est arrivé, évite que des trains puissent se doubler
    }
    return NULL;
}


void* roule(void *arg) {
    unsigned char i, j;
    unsigned char id = *((unsigned char*) arg);

    for (i=0 ; i < N_TRAJETS ; i++) {
        for (j=1 ; j < trains[id].l_trajet ; j++) {
            voyage(trains[id].id, trains[id].trajet[j-1], trains[id].trajet[j]);
        }
    }

    return NULL;
}

void handle_sig(int sig){
    signal(sig, SIG_IGN); //pas d'appel au handler lorsqu'il traite déjà un signal de ce type
    clean_semaphores();
    exit(EXIT_SUCCESS);
}

int main() {
    //TODO benchmark tests
    unsigned char i;
    unsigned char index_trains[N_TRAINS];
    struct sigaction act;
    pthread_t thread_train_ids[N_TRAINS], thread_liaison_ids[N_LIAISONS];

    srand(time(NULL)); // initialisation de rand
    memset(&act, '\0', sizeof(act));
    act.sa_handler = handle_sig;

    init_reseau();
    sigaction(SIGINT, &act, NULL);

    for (i = 0; i < N_TRAINS; i++) {
        index_trains[i] = i;
        if (pthread_create(&thread_train_ids[i], NULL, roule, index_trains+i) != 0) {
            perror("THREAD ERROR : ");
            for (unsigned char j = 0 ; j < i ; j++) {
                pthread_cancel(thread_train_ids[j]);
            }
            clean_semaphores();
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < N_LIAISONS; i++) {
        if (pthread_create(&thread_liaison_ids[i], NULL, gere_arrivee_gare, liaisons[all_liaisons[i][0]]+all_liaisons[i][1]) != 0) {
            unsigned char j;
            perror("THREAD ERROR : ");
            for (j = 0 ; j < i ; j++) {
                pthread_cancel(thread_liaison_ids[j]);
            }
            for (j = 0 ; j < N_TRAINS ; j++) {
                pthread_cancel(thread_train_ids[j]);
            }
            clean_semaphores();
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < N_TRAINS; i++) {
        pthread_join(thread_train_ids[i], NULL);
    }
    for (i = 0; i < N_LIAISONS; i++) {
        pthread_cancel(thread_liaison_ids[i]); //Ces threads font une boucle infinie, les arrêter car leur travail est fini
    }

    clean_semaphores();
    return 0;
}