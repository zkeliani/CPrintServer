#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <zconf.h>

#define MAX_JOBS 25
#define MAX_USER_JOBS 30
#define MIN_BYTES 100

int *x, *y;
sem_t *mutex, *empty, *full;
struct PrintJob *queue;

struct PrintJob
{
    int bytes;
} item;

void sig_func(int sig)
{
    signal(SIGSEGV, sig_func);
    printf("Caught signal %d, threads terminated\n", sig);
    fflush(stdout);
}

void User(int jobs, struct PrintJob pending[], int pid)
{
    struct timespec req, rem;
    int i;

    for (i = 0; i < jobs; i++)
    {
        sem_wait(empty);
        sem_wait(mutex);

        queue[*x] = pending[i];
        *x = (*x + 1) % MAX_JOBS;

        sem_post(mutex);
        sem_post(full);

        printf("Process %d submitting job, size %d\n", pid, pending[i].bytes);

        req.tv_sec = 0;
        req.tv_nsec = (rand() % 1000000000 - 100000000 + 1) + 100000000;
        nanosleep(&req, &rem);
    }
}

void *Printer()
{
    int count = 0;
    int finished = 0;
    int tcount = 0;

    while (1)
    {
        time_t start = time(NULL);

        sem_wait(full);
        sem_wait(mutex);

        count++;
        finished += queue[*y].bytes;
        *y = (*y + 1) % MAX_JOBS;

        sem_post(mutex);
        sem_post(empty);

        time_t finish = time(NULL);
        tcount += (finish - start);
        printf("Thread time elapsed: %d\n", tcount);

        printf("Thread %d printing. Total print jobs: %d. Total job size: %d\n", pthread_self(), count, finished);
    }
}

void allocate()
{
    queue = mmap(NULL, MAX_JOBS * sizeof(item), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    y = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    x = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    empty = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    full = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

void deallocate()
{
    munmap(mutex, sizeof(sem_t));
    munmap(empty, sizeof(sem_t));
    munmap(full, sizeof(sem_t));
    munmap(queue, MAX_JOBS * sizeof(item));
    munmap(x, sizeof(int));
    munmap(y, sizeof(int));
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Must have two arguments: number of threads and number of processes.\n");

        exit(1);
    }

    allocate();

    *x = 0;
    *y = 0;

    pthread_t tid[3];
    int i;

    sem_init(mutex, 1, 1);
    sem_init(empty, 1, MAX_JOBS);
    sem_init(full, 1, 0);

    int processCount = atoi(argv[1]);
    int threadCount = atoi(argv[2]);

    for (i = 0; i < processCount; i++)
    {
        int userJobs = (rand() % MAX_USER_JOBS) + 1;
        int total_bytes = 0;
        struct PrintJob pending[userJobs];

        int j;
        for (j = 0; j < userJobs; j++)
        {
            int bytes = (rand() % 901) + MIN_BYTES;
            pending[j].bytes = bytes;
            total_bytes += bytes;
        }

        if (fork() == 0)
        {
            time_t start = time(NULL);

            User(userJobs, pending, getpid());

            time_t finish = time(NULL);
            printf("Time to complete: %d\n", finish - start);

            printf(">>> Process %d finished: %d jobs with size %d \n", getpid(), userJobs, total_bytes);

            exit(0);
        }
    }

    sleep(1);

    signal(SIGINT, sig_func);
    for (i = 0; i < threadCount; i++)
    {
        if (pthread_create(&tid[i], NULL, Printer, NULL))
        {
            printf("Error: Creating thread");

            exit(1);
        }
    }

    //cleans up memory after processes are finished
    for (i = 0; i < processCount; i++)
    {
        wait(NULL);
    }

    for (i = 0; i < threadCount; i++)
    {
        pthread_kill(tid[i], SIGINT);
    }

    deallocate();

    exit(0);
}
