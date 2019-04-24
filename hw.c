#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <zconf.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/wait.h>
#include <sys/mman.h>

struct Print {
    int size;
    char temp;
} item;

sem_t *mutex, *empty, *full;
struct Print *buf;
int *in, *out;

void Producer(int jobs, struct Print request[], int pid) {
    struct timespec req, rem;

    int i;
    for (i = 0; i < jobs; i++) {
        sem_wait(empty);
        sem_wait(mutex);

        buf[*out] = request[i];
        *out = (*out + 1) % 25;

        sem_post(mutex);
        sem_post(full);

        printf("Process %d submitting job, size %d\n", pid, request[i].size);

        req.tv_sec = 0;
        req.tv_nsec = (rand() % 1000000000 - 100000000 + 1) + 100000000;
        nanosleep(&req, &rem);
    }
}

void *Consumer() {
    int count = 0;
    int printed = 0;
    int time_elapsed = 0;

    while (1) {
        time_t start = time(NULL);

        sem_wait(full);
        sem_wait(mutex);

        count++;
        printed += buf[*in].size;
        *in = (*in + 1) % 25;

        sem_post(mutex);
        sem_post(empty);

        time_t end = time(NULL);
        time_elapsed += (end - start);
        printf("Thread time elapsed: %d\n", time_elapsed);

        printf("Thread %d printing, total print job: %d, total print size: %d\n", pthread_self(), count, printed);
    }
}

void sig_func(int sig) {
    signal(SIGSEGV, sig_func);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Not enough or too many arguments.\n");

        exit(1);
    }

    buf = mmap(NULL, 25 * sizeof(item), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    in = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    out = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    empty = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    full = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    *in = 0;
    *out = 0;

    pthread_t tid[3];
    int i;

    sem_init(mutex, 1, 1);
    sem_init(empty, 1, 25);
    sem_init(full, 1, 0);

    int num_process = atoi(argv[1]);
    int num_thread = atoi(argv[2]);

    for (i = 0; i < num_process; i++) {
        int jobs = (rand() % 30) + 1;
        int total_size = 0;
        struct Print request[jobs];

        int j;
        for (j = 0; j < jobs; j++) {
            int size = (rand() % 901) + 100;
            request[j].size = size;
            total_size += size;
        }

        if (fork() == 0) {
            time_t start = time(NULL);

            Producer(jobs, request, getpid());

            time_t end = time(NULL);
            printf("Process time elapsed: %d\n", end - start);

            printf(">>> Process %d completed with: %d jobs, %d size\n", getpid(), jobs, total_size);

            exit(0);
        }
    }

    sleep(1);

    signal(SIGINT, sig_func);
    for (i = 0; i < num_thread; i++) {
        if (pthread_create(&tid[i], NULL, Consumer, NULL)) {
            printf("ERROR creating thread");

            exit(1);
        }
    }

    for (i = 0; i < num_process; i++) {
        wait(NULL);
    }

    for (i = 0; i < num_thread; i++) {
        pthread_kill(tid[i], SIGINT);
    }

    munmap(buf, 25 * sizeof(item));
    munmap(in, sizeof(int));
    munmap(out, sizeof(int));
    munmap(mutex, sizeof(sem_t));
    munmap(empty, sizeof(sem_t));
    munmap(full, sizeof(sem_t));

    exit(0);
}
