/**
single globally shared printer queue that all processes and threads can read from or write to, 25 print jobs
producer-consumer problem

**/


#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_JOBS 25
#define MAX_USER_JOBS 30
#define MIN_BYTES 100
#define MAX_BYTES 1000

typedef struct
{
	int bytes;
} PrintJob;

PrintJob create_job()
{
	int random_bytes = rand() % (MAX_BYTES - MIN_BYTES) + MIN_BYTES + 1;
	PrintJob job = {.bytes = random_bytes};
	return job;
}

typedef struct
{
	PrintJob jobs[MAX_USER_JOBS] ;
	int remaining_jobs;
} User;

User create_user()
{
	User user;
	int random_size = rand() % (MAX_USER_JOBS + 1);
	int i;
	
	for (i = 0; i < random_size; i++)
	{
		PrintJob new_job = create_job();
		user.jobs[i] = new_job;
	}
	
	user.remaining_jobs = random_size;
	
	return user;
}

typedef struct
{
	int byteCount;
	int jobCount;
} Printer;

Printer create_printer()
{
	Printer printer;
	printer.byteCount = 0;
	printer.jobCount = 0;
	
	return printer;
}

typedef struct
{
	PrintJob queue[MAX_JOBS];
	int count;
} GlobalQueue;

GlobalQueue global_queue;
sem_t full, empty, rw;

int main(int argc, char *argv[])
{
	int x, y;
	int arg1, arg2;

	printf("Please enter the number of producer(user) processes: \n");
	if ((arg1 = scanf("%d", &x)) == 0)
	{
		printf("Error: Input must be an integer.\n");
		exit(1);
	}
	
	printf("Please enter the number of printer(consumer) threads: \n");
	if ((arg2 = scanf("%d", &y)) == 0)
	{
		printf("Error: Input must be an integer.\n");
		exit(1);
	}
	
	//for seeding random functions
	srand(time(NULL));
	
	PrintJob job1 = create_job();
	printf("%d \n", job1.bytes);
	User user1 = create_user();
	printf("%d \n", user1.remaining_jobs);
	
	global_queue.count = 0;
	
	sem_init(&full, 1, 0);
	sem_init(&empty, 1, 0);
	sem_init(&rw, 1, 0);
	
	pthread_t tid1, tid2, tid3;
}