/**
single globally shared printer queue that all processes and threads can read from or write to, 25 print jobs
producer-consumer problem

**/


#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define MAX_JOBS 25
#define MAX_USER_JOBS 30
#define MIN_BYTES 100
#define MAX_BYTES 1000
#define SHMSZ 512

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

int min(int x, int y)
{
	return (x < y) ? x : y;
}

typedef struct
{
	sem_t full, empty, rw;
	int f_val, e_val, rw_val;
} Semaphores;

GlobalQueue global_queue;
Semaphores semaphores;
sem_t wait_sem, mutex_sem;
int wait_val, mutex_val;

void wait()
{
	sem_getvalue(&wait_sem,  &mutex_val);
	sem_getvalue(&mutex_sem, &mutex_val);
	
	sem_wait(&wait_sem);
	sem_wait(&mutex_sem);
	global_queue.count = global_queue.count - 1;
	if (global_queue.count > 0)
	{
		sem_post(&wait_sem);
	}
	sem_post(&mutex_sem);
}

void signal1()
{
	sem_getvalue(&wait_sem,  &mutex_val);
	sem_getvalue(&mutex_sem, &mutex_val);
	
	sem_wait(&mutex_sem);
	global_queue.count = global_queue.count + 1;
	if (global_queue.count == 1)
	{
		sem_post(&wait_sem);
	}
	sem_post(&mutex_sem);
}

void * Count(void * r)
{
	int temp, nm;
	for (nm = 0; nm < MAX_JOBS; nm++)
	{
		wait();
		
		signal1();
	}
}

void handle_sigint(int sig)
{
	printf("Caught signal %d, threads terminated\n", sig);
}

int main(int argc, char *argv[])
{
	sem_init(&semaphores.full, 1, 0);
	sem_init(&semaphores.empty, 1, 0);
	sem_init(&semaphores.rw, 1, 0);
	sem_init(&wait_sem, 1, min(1, global_queue.count));
	sem_init(&mutex_sem, 1, 1);
	
	pid_t f1 = fork();
	
	if (f1 != 0)
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
		
		User users[x];
		
		int q;
		for (q = 0; q < x; q++)
		{
			User temp = create_user();
			users[q] = temp;
		}
		
		global_queue.count = 0;
		
		sem_init(&semaphores.full, 1, 0);
		sem_init(&semaphores.empty, 1, 0);
		sem_init(&semaphores.rw, 1, 0);
		sem_init(&wait_sem, 1, min(1, global_queue.count));
		sem_init(&mutex_sem, 1, 1);
		
		pthread_t threads[y];

		int o;
		for(o = 0; o < q; o++)
		{
			fork();
		}
		
		signal(2, handle_sigint);
		
		int totalJobs;
		totalJobs = 0;
		int iter;
		int iter2;
		int totalBytes;
		totalBytes = 0;
		
		for (iter = 0; iter < x; iter++)
		{
			totalJobs = users[iter].remaining_jobs;
		}
		
		printf("Total print jobs: %d\n", totalJobs);
		
		for (iter = 0; iter < x; iter++)
		{
			for (iter2 = 0; iter2 < users[iter].remaining_jobs; iter2++)
			{
				printf("Job size: %d\n", users[iter].jobs[iter2].bytes);
				totalBytes = totalBytes + users[iter].jobs[iter2].bytes;
			}
		}
		
		int mini;
		int k;
		for (mini = 0; mini < y; mini++)
		{
			printf("Printer %d: \n", mini+1);
			printf("Jobs %d: \n", totalJobs/y);
			printf("Bytes Processed %d: \n", totalBytes/y);
		}
		
	}
	else
	{

	}
	
	
	
	return 0;
}