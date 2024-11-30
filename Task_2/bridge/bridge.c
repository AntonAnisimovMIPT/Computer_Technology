#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_CARS_ON_BRIDGE 5

int Nwait = 0, Swait = 0, Count = 0, Dir = 0, Npass = 0, Spass = 0;

sem_t Lock; 
sem_t N; 
sem_t S; 

void* north_car(void* arg) {
    printf("Машина с севера подъехала к мосту.\n");

    // Доехать до моста
    sem_wait(&Lock);
    if (Nwait || Npass == MAX_CARS_ON_BRIDGE || (Count > 0 && Dir != 1)) {
        Nwait++;
        sem_post(&Lock);
        sem_wait(&N); // Ожидание разрешения въезда
        Nwait--;
        Spass = 0;
    }
    // Въезд с севера
    if (Swait) Npass++;
    Count++;
    Dir = 1;
    if (Nwait && (Swait == 0 || Npass < MAX_CARS_ON_BRIDGE)) {
        sem_post(&N);
    } else {
        sem_post(&Lock);
    }

    printf("Машина с севера въехала на мост.\n");
    sleep(1); // Проезжает мост

    // Выезд с севера
    sem_wait(&Lock);
    Count--;
    printf("Машина с севера покинула мост.\n");
    if (Swait && Count == 0) {
        sem_post(&S);
    } else {
        sem_post(&Lock);
    }

    return NULL;
}

void* south_car(void* arg) {
    printf("Машина с юга подъехала к мосту.\n");

    // Доехать до моста
    sem_wait(&Lock);
    if (Swait || Spass == MAX_CARS_ON_BRIDGE || (Count > 0 && Dir != 2)) {
        Swait++;
        sem_post(&Lock);
        sem_wait(&S); // Ожидание разрешения въезда
        Swait--;
        Npass = 0;
    }
    // Въезд с юга
    if (Nwait) Spass++;
    Count++;
    Dir = 2;
    if (Swait && (Nwait == 0 || Spass < MAX_CARS_ON_BRIDGE)) {
        sem_post(&S);
    } else {
        sem_post(&Lock);
    }

    printf("Машина с юга въехала на мост.\n");
    sleep(1); // Проезжает мост

    // Выезд с юга
    sem_wait(&Lock);
    Count--;
    printf("Машина с юга покинула мост.\n");
    if (Nwait && Count == 0) {
        sem_post(&N);
    } else {
        sem_post(&Lock);
    }

    return NULL;
}

int main() {
    pthread_t threads[10];
    sem_init(&Lock, 0, 1);
    sem_init(&N, 0, 0);
    sem_init(&S, 0, 0);

    // Запускаем 10 потоков (машины с севера и с юга)
    for (int i = 0; i < 10; i++) {
        if (i % 2 == 0) {
            pthread_create(&threads[i], NULL, north_car, NULL);
        } else {
            pthread_create(&threads[i], NULL, south_car, NULL);
        }
        usleep(1000); // Интервал между машинами
    }

    // Ждем завершения всех потоков
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }

    // Освобождаем ресурсы
    sem_destroy(&Lock);
    sem_destroy(&N);
    sem_destroy(&S);

    return 0;
}
