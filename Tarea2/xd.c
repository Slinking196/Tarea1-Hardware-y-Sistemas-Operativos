#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <dirent.h>

sem_t *sA;
sem_t *sB;

void *escribirA() {
    int i;

    for(i = 0; i < 5; i++) {
        sem_wait(sA);
        printf("A");
        //sleep(2);
        sem_post(sB);
        
    }

    pthread_exit(NULL);
}

void *escribirB() {
    int i;
    for(i = 0; i < 5; i++) {
        sem_wait(sB);
        printf("B");
        //sleep(2);
        sem_post(sA);
    }

    pthread_exit(NULL);
}

int main() {
    sA = (sem_t *) malloc(sizeof(sem_t));
    sB = (sem_t *) malloc(sizeof(sem_t));

    sem_init(sA, 0, 0);
    sem_init(sB, 0, 1);

    pthread_t hebras[2];

    pthread_create(&hebras[0], NULL, escribirA, NULL);
    pthread_create(&hebras[1], NULL, escribirB, NULL);

    //pthread_join(hebras[0], NULL);
    pthread_join(hebras[1], NULL);

    free(sA);
    free(sB);

    return 0;
}