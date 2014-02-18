#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"
#include "util.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_RESOLVER_THREADS 10


/* Defines The Struct Sent Into Request/Resolver Functions */
struct thread_info {
	FILE* thread_file;
	pthread_mutex_t* queue_mutex;
	pthread_mutex_t* file_mutex;
	queue* request_queue;
};

/* Functions To Handle What Each Kind Of Thread Should Do */
void* requester_thread_function(void* thread_information);
void* resolver_thread_function(void* thread_information);

