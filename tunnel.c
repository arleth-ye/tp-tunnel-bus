#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define NB_X 5
#define NB_Y 4
// Nombre d’allers-retours par bus
#define ALLER_RETOURS 10

// Sémaphores pour protéger les compteurs et gérer les files d’attente
sem_t mutex;  // verrou pour l’exclusion mutuelle des compteurs partagés
sem_t semX;   // file d’attente pour les bus X→Y
sem_t semY;   // file d’attente pour les bus Y→X

// Compteurs partagés (protégés par `mutex`)
int insideX = 0;   // nombre de bus X→Y actuellement dans le tunnel
int insideY = 0;   // nombre de bus Y→X actuellement dans le tunnel
int waitingX = 0;  // nombre de bus X→Y en attente
int waitingY = 0;  // nombre de bus Y→X en attente

// Indicateur de tour pour assurer l’équité
// 0 = priorité aux bus X→Y ; 1 = priorité aux bus Y→X
int turn = 0;

/**
 * Entée dans le tunnel pour un bus de direction `dir` ('X' ou 'Y').
 * Bloque si :
 *  - Il y a déjà des bus de l’autre sens dans le tunnel.
 *  - Ce n’est pas son tour alors que l’autre sens a des bus en attente.
 */
void enter_tunnel(char dir) {
    sem_wait(&mutex);        // début section critique
    if (dir == 'X') {
        waitingX++;
        // Boucle d’attente tant que la voie est occupée ou qu’on n’a pas la priorité
        while (insideY > 0 || (turn == 1 && waitingY > 0)) {
            sem_post(&mutex);
            sem_wait(&semX);
            sem_wait(&mutex);
        }
        waitingX--;
        insideX++;           // un bus X→Y entre
    } else {
        waitingY++;
        while (insideX > 0 || (turn == 0 && waitingX > 0)) {
            sem_post(&mutex);
            sem_wait(&semY);
            sem_wait(&mutex);
        }
        waitingY--;
        insideY++;           // un bus Y→X entre
    }
    sem_post(&mutex);        // fin section critique
}

/**
 * Sortie du tunnel pour un bus de direction `dir`.
 * Met à jour les compteurs et libère éventuellement la file d’attente opposée.
 */
void exit_tunnel(char dir) {
    sem_wait(&mutex);  // début section critique
    if (dir == 'X') {
        insideX--;
        // Si plus aucun bus X→Y à l’intérieur, donne la priorité aux Y→X
        if (insideX == 0) {
            turn = 1;
            if (waitingY > 0) {
                sem_post(&semY);
            }
        }
    } else {
        insideY--;
        if (insideY == 0) {
            turn = 0;
            if (waitingX > 0) {
                sem_post(&semX);
            }
        }
    }
    sem_post(&mutex);  // fin section critique
}

/**
 * Fonction exécutée par chaque thread représentant une bus.
 * Effectue ALLER_RETOURS allers-retours complets.
 */
void *bus_thread(void *arg) {
    int id    = ((int *)arg)[0];  // identifiant du bus
    char city = ((int *)arg)[1];  // 'X' ou 'Y' : ville d’origine
    char dest = (city == 'X' ? 'Y' : 'X');

    for (int i = 1; i <= ALLER_RETOURS; i++) {
        // ==== Aller ====
        enter_tunnel(city);
        printf("Bus %d de %c : %c -> %c (Trajet %d aller)\n",
               id, city, city, dest, i);
        usleep((1000 + rand() % 500) * 1000);  // pause 1–1.5 s
        exit_tunnel(city);

        // ==== Retour ====
        enter_tunnel(dest);
        printf("Bus %d de %c : %c -> %c (Trajet %d retour)\n",
               id, city, dest, city, i);
        usleep((1000 + rand() % 500) * 1000);
        exit_tunnel(dest);
    }
    return NULL;
}

int main() {
    // Threads et paramètres
    pthread_t threads[NB_X + NB_Y];
    int params[NB_X + NB_Y][2];  // [[id, dir], ...]

    // Initialisation des sémaphores
    sem_init(&mutex, 0, 1);  // mutex binaire
    sem_init(&semX,  0, 0);  // départ fermé
    sem_init(&semY,  0, 0);
    srand(time(NULL));

    // Création des threads pour les bus de X
    for (int i = 0; i < NB_X; i++) {
        params[i][0] = i + 1;     // id
        params[i][1] = 'X';       // direction
        pthread_create(&threads[i], NULL, bus_thread, params[i]);
    }
    // Création des threads pour les bus de Y
    for (int i = 0; i < NB_Y; i++) {
        params[NB_X + i][0] = i + 1;
        params[NB_X + i][1] = 'Y';
        pthread_create(&threads[NB_X + i], NULL, bus_thread, params[NB_X + i]);
    }

    // Attente de la fin de tous les threads
    for (int i = 0; i < NB_X + NB_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destruction des sémaphores
    sem_destroy(&mutex);
    sem_destroy(&semX);
    sem_destroy(&semY);

    return 0;
}
