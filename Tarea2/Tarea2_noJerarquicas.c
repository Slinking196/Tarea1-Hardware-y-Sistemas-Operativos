#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
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

typedef struct {
    Barberia *barberia;
    int cliente;

} Parametro;

static pthread_mutex_t printf_mutex;

Barberia *createBarberia() {
    Barberia *newBarberia = (Barberia *) malloc(sizeof(Barberia));

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

Parametro *createParametro(Barberia *barberia, int cliente) {
    Parametro *newParametro = (Parametro *) malloc(sizeof(Parametro));

    newParametro->barberia = barberia;
    newParametro->cliente = cliente;

    return newParametro;
}

void leerArchivo(Barberia *barberia, char *ruta) {
    char basura[1024];
    FILE *archivo;
    Cliente aux;
    char rutaCompleta[30] = "pepe/";
    strcat(rutaCompleta, ruta);
    printf("%s\n", rutaCompleta);
    archivo = fopen(rutaCompleta, "r");

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

    barberia->barberos = (sem_t *) malloc(sizeof(sem_t) * barberia->cantBarberos);

    for(int i = 0; i < barberia->cantBarberos; i++) {
        sem_init(&barberia->barberos[i], 0, 1);
    }

    barberia->sillasBarbero = (sem_t *) malloc(sizeof(sem_t) * barberia->cantSillasBarberos);
    
    for(int i = 0; i < barberia->cantSillasBarberos; i++) {
        sem_init(&barberia->sillasBarbero[i], 0, 1);
    }
    
    barberia->sillasEspera = (sem_t *) malloc(sizeof(sem_t) * barberia->cantSillasEspera);
    
    for(int i = 0; i < barberia->cantSillasEspera; i++) {
        sem_init(&barberia->sillasEspera[i], 0, 1);
    }
}

void sem_destroy_all(Barberia *barberia) {
    for(int i = 0; i < barberia->cantBarberos; i++) {
        sem_destroy(&barberia->barberos[i]);
    }

    for(int i = 0; i < barberia->cantSillasBarberos; i++) {
        sem_destroy(&barberia->sillasBarbero[i]);
    }

    for(int i = 0; i < barberia->cantSillasEspera; i++) {
        sem_destroy(&barberia->sillasEspera[i]);
    }

    pthread_mutex_destroy(&printf_mutex);
}

void *corteDePelo(void *data) {
    Barberia *barberia = ((Parametro *) data)->barberia;
    int cliente = ((Parametro *) data)->cliente;
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
                pthread_exit(NULL);
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

                pthread_exit(NULL);
            }
        }
    }
    else {
        pthread_mutex_lock(&printf_mutex);
        printf("Sale cliente %d (no quedan sillas de espera)\n", cliente);
        pthread_mutex_unlock(&printf_mutex);
    }

    pthread_exit(NULL);
}

int contadorArchivos(const char *ruta) {
    DIR *dir;
    struct dirent *entry;
    int cantArchivos = 0;

    dir = opendir(ruta);

    if(dir == NULL) {
        printf("No se puede abrir la carpeta\n");
        exit(EXIT_FAILURE);
    }

    while((entry = readdir(dir)) != NULL) {
        if(entry->d_type == DT_REG) {
            cantArchivos++;
        }
    }

    closedir(dir);

    return cantArchivos;
}

int main() {
    int cantHebras;
    int i;
    Barberia *barberia = NULL;
    Parametro *aux = NULL;
    int casos;
    char num[8] = "";

    casos = contadorArchivos("pepe");

    for(int j = 0; j < casos; j++) {
        char ruta[15] = "file";
        printf("/******* | Caso de Prueba %d | *******\\ \n\n", j + 1);
        sprintf(num, "%d.txt", j);
        strcat(ruta, num);

        barberia = createBarberia();

        leerArchivo(barberia,ruta);

        cantHebras = barberia->cantClientes;

        pthread_t hebras[cantHebras];
        pthread_mutex_init(&printf_mutex, NULL);
        
        for(i = 0; i < cantHebras; i++) {
            sleep(barberia->clientes[i].tiempoEntrada);
            aux = createParametro(barberia, i);
            pthread_create(&hebras[i], NULL, (void *) &corteDePelo, (void *) aux);
        }  

        for(i = 0; i < cantHebras; i++) {
            pthread_join(hebras[i], NULL);
        }

        sem_destroy_all(barberia);
    }
    return EXIT_SUCCESS;
}