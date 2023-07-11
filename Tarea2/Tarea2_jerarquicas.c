#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <dirent.h>

typedef struct {
    int tiempoEntrada;
    int tiempoEspera;
    int tiempoCorte;

} Cliente;

typedef struct {
    int cantSillasEspera;
    sem_t *sillasEspera;
    int cantBarberos;
    sem_t *barberos;
    int cantSillasBarberos;
    sem_t *sillasBarbero;
    int cantClientes;
    Cliente *clientes;

} Barberia;

static pthread_mutex_t printf_mutex;

Barberia *createBarberia() {
    Barberia *newBarberia = mmap(NULL, sizeof(Barberia), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    newBarberia->cantSillasBarberos = 0;
    newBarberia->sillasEspera = NULL;
    newBarberia->cantBarberos = 0;
    newBarberia->barberos = NULL;
    newBarberia->cantSillasEspera = 0;
    newBarberia->sillasBarbero = NULL;
    newBarberia->cantClientes = 0;
    newBarberia->clientes = NULL;

    return newBarberia;
}

void leerArchivo(Barberia *barberia, char *ruta) {
    char basura[1024];
    FILE *archivo;
    Cliente aux;
    archivo = fopen(ruta, "r");

    if(!archivo) {
        printf("El archivo no fue encontrado\n");
        exit(EXIT_FAILURE);
    }

    fscanf(archivo, "%i %i %i", &barberia->cantSillasEspera, &barberia->cantBarberos, &barberia->cantSillasBarberos);
    fgets(basura, 1023, archivo);

    while(!feof(archivo)) {
        fscanf(archivo, "%d %d %d", &aux.tiempoEntrada, &aux.tiempoEspera, &aux.tiempoCorte);

        barberia->cantClientes++;
        barberia->clientes = (Cliente *) realloc(barberia->clientes, barberia->cantClientes * sizeof(Cliente));
        barberia->clientes[barberia->cantClientes - 1] = aux;
    }

    barberia->barberos = mmap(NULL, sizeof(sem_t) * barberia->cantBarberos, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    for(int i = 0; i < barberia->cantBarberos; i++) {
        sem_init(&barberia->barberos[i], 0, 1);
    }

    barberia->sillasBarbero = mmap(NULL, sizeof(sem_t) * barberia->cantSillasBarberos, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    for(int i = 0; i < barberia->cantSillasBarberos; i++) {
        sem_init(&barberia->sillasBarbero[i], 0, 1);
    }
    
    barberia->sillasEspera = mmap(NULL, sizeof(sem_t) * barberia->cantSillasEspera, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    for(int i = 0; i < barberia->cantSillasEspera; i++) {
        sem_init(&barberia->sillasEspera[i], 0, 1);
    }
}

void sem_destroy_all(Barberia *barberia) {
    for(int i = 0; i < barberia->cantBarberos; i++) {
        sem_destroy(&barberia->barberos[i]);
    }

    munmap(barberia->barberos, barberia->cantBarberos * sizeof(sem_t));

    for(int i = 0; i < barberia->cantSillasBarberos; i++) {
        sem_destroy(&barberia->sillasBarbero[i]);
    }

    munmap(barberia->sillasBarbero, barberia->cantSillasBarberos * sizeof(sem_t));

    for(int i = 0; i < barberia->cantSillasEspera; i++) {
        sem_destroy(&barberia->sillasEspera[i]);
    }

    munmap(barberia->sillasEspera, barberia->cantSillasEspera * sizeof(sem_t));
}

void corteDePelo(Barberia *barberia, int cliente) {
    bool haySillasE = false;
    bool haySillasB = false;
    bool hayBarbero = false;
    int sillaEspera;
    int sillaBarbero;
    int barbero;
    int tiempoEspera = 0;
    int tiempoCorte = 0;

    pthread_mutex_lock(&printf_mutex);
    printf("Entra cliente %d a la barberia\n", cliente);
    pthread_mutex_unlock(&printf_mutex);

    for(sillaEspera = 0; sillaEspera < barberia->cantSillasEspera; sillaEspera++) {
        if(sem_trywait(&barberia->sillasEspera[sillaEspera]) == 0) {
            haySillasE = true;
            break;
        }
    }

    if(haySillasE) {
        pthread_mutex_lock(&printf_mutex);
        printf("Cliente %d usa silla de espera %d\n", cliente, sillaEspera);
        pthread_mutex_unlock(&printf_mutex);

        while(true) {
            if(!haySillasB && !hayBarbero) {
                for(sillaBarbero = 0; sillaBarbero < barberia->cantSillasBarberos; sillaBarbero++) {
                    if(sem_trywait(&barberia->sillasBarbero[sillaBarbero]) == 0) {
                        for(barbero = 0; barbero < barberia->cantBarberos; barbero++) {
                            if(sem_trywait(&barberia->barberos[barbero]) == 0) {
                                sem_post(&barberia->sillasEspera[sillaEspera]);
                                haySillasE = false;
                                haySillasB = true;
                                hayBarbero = true;
                                break;
                            }
                        }
                    }

                    if(haySillasB && hayBarbero) break;
                }
            }

            if(haySillasB && hayBarbero) {
                pthread_mutex_lock(&printf_mutex);
                printf("Cliente %d usa silla de barbero %d\n", cliente, sillaBarbero);
                pthread_mutex_unlock(&printf_mutex);

                pthread_mutex_lock(&printf_mutex);
                printf("Barbero %d atiende a cliente %d\n", barbero, cliente);
                pthread_mutex_unlock(&printf_mutex);
                    
                sleep(barberia->clientes[cliente].tiempoCorte);
                sem_post(&barberia->barberos[barbero]);
                sem_post(&barberia->sillasBarbero[sillaBarbero]);
                            
                pthread_mutex_lock(&printf_mutex);
                printf("Sale cliente %d (atendido por completo)\n", cliente);
                pthread_mutex_unlock(&printf_mutex);
                return;
            }
            tiempoEspera++;
            sleep(1);

            if(tiempoEspera == barberia->clientes[cliente].tiempoEspera) {

                if(haySillasE) {
                    pthread_mutex_lock(&printf_mutex);
                    printf("Sale cliente %d (Espero demasiado en silla de espera %d)\n", cliente, sillaEspera);
                    pthread_mutex_unlock(&printf_mutex);
                    sem_post(&barberia->sillasEspera[sillaEspera]);
                }
                if(haySillasB) {
                    pthread_mutex_lock(&printf_mutex);
                    printf("Sale cliente %d (Espero demasiado en silla de barbero %d)\n", cliente, sillaEspera);
                    pthread_mutex_unlock(&printf_mutex);
                    sem_post(&barberia->sillasBarbero[sillaBarbero]);
                }
                return;
            }
        }
    }
    else {
        pthread_mutex_lock(&printf_mutex);
        printf("Sale cliente %d (no quedan sillas de espera)\n", cliente);
        pthread_mutex_unlock(&printf_mutex);
        return;
    }
}

int main() {
    Barberia *barberia = createBarberia();
    int i;
    int cont = 0;
    int status;
    int tiempoEMax;
    int tiempoCMax;

    leerArchivo(barberia, "file0.txt");
    pthread_mutex_init(&printf_mutex, NULL);

    tiempoEMax = barberia->clientes[0].tiempoEntrada;
    tiempoCMax = barberia->clientes[0].tiempoCorte;

    for(i = 0; i < barberia->cantClientes; i++) {
        sleep(barberia->clientes[i].tiempoEntrada);
        if (tiempoCMax > barberia->clientes[i].tiempoCorte) tiempoCMax = barberia->clientes[i].tiempoCorte;
        if (tiempoEMax > barberia->clientes[i].tiempoEntrada) tiempoEMax = barberia->clientes[i].tiempoEntrada;
        
        if(fork() == 0) {
            corteDePelo(barberia, i);
            exit(EXIT_SUCCESS);
        }
    }

    sleep(tiempoCMax + tiempoEMax);
    sem_destroy_all(barberia);
    munmap(barberia, sizeof(Barberia));

    return EXIT_SUCCESS;
}