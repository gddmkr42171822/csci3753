#include "multi-lookup.h"

int main(int argc, char* argv[]){

	/* Initialize Request Queue */
	queue request_q;
    if(queue_init(&request_q, QUEUEMAXSIZE) == QUEUE_FAILURE){
	fprintf(stderr,
		"error: queue_init failed!\n");
    }

    int mutex_error;

	/* Create A Mutex For The Request Queue And Output File */
	pthread_mutex_t queue_mutex;
	pthread_mutex_t output_file_mutex;
	mutex_error = pthread_mutex_init(&queue_mutex, NULL);
	if (mutex_error){
	    fprintf(stderr, "ERROR; return code from pthread_mutex_init() for the queue is %d\n", mutex_error);
	} 
	mutex_error = pthread_mutex_init(&output_file_mutex, NULL);
	if (mutex_error){
	    fprintf(stderr, "ERROR; return code from pthread_mutex_init() for the output file is %d\n", mutex_error);
	} 

	/* Establish Size Of Array To Hold # Of Input Files */
	int num_input_files = argc-2;

	/* Initialize Array Of Input Files */
    FILE* inputfp[num_input_files];

    /* Initialize Output File I.E. Results.txt */
    FILE* outputfp = NULL;

    
    char errorstr[SBUFSIZE];
    int i;

    /* Setup Local Vars */
    pthread_t requester_threads[num_input_files];
    pthread_t resolver_threads[MAX_RESOLVER_THREADS];
    int rc;
    int t;

	/* Check Input Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
    }


    /* Loop Through Input Files */
    for(i=1; i<(argc-1); i++){
	
	/* Open Input Files */
	inputfp[i-1] = fopen(argv[i], "r");
	if(!inputfp[i-1]){
	    sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
	    perror(errorstr);
	    break;
		}
	}

	/* Initialize Requester Threads Struct */
	struct thread_info request_threads[num_input_files];

	//printf("Num of args: %d\n", argc);
	//printf("Num of requester threads: %d\n", num_input_files);
	printf("Creating requester threads!\n");

	/* Create # Requester Threads Equal To Number Of Input Files */
	for(t=0;t < num_input_files;t++){

	/* Initialize A Thread_Info Struct To Hold Information For Each Thread */
	FILE* current_file = inputfp[t];
	request_threads[t].thread_file = current_file;
	request_threads[t].queue_mutex = &queue_mutex;
	request_threads[t].file_mutex = NULL;
	request_threads[t].request_queue = &request_q;

	/* Create Each Thread And Send Them Into the Requester Function With Their Specfic Files */
	rc = pthread_create(&(requester_threads[t]), NULL, requester_thread_function, &(request_threads[t]));
	if (rc){
	    printf("ERROR; return code from pthread_create() is %d\n", rc);
	    exit(EXIT_FAILURE);
		}
	}

    /* Initialize The Resolver Thread Struct */
    struct thread_info resolve_threads[MAX_RESOLVER_THREADS];


    printf("Creating resolver threads!\n");

    /* Create The Resolver Threads */
    for(t=0; t < MAX_RESOLVER_THREADS; t++){

    	/* Put Each Thread Data Into Struct */
    	resolve_threads[t].thread_file = outputfp;
    	resolve_threads[t].queue_mutex = &queue_mutex;
    	resolve_threads[t].file_mutex = &output_file_mutex;
    	resolve_threads[t].request_queue = &request_q;

    	/* Create Each Resolver Thread And Send Them Into The Resolver Function */
    	rc = pthread_create(&(resolver_threads[t]), NULL, resolver_thread_function, &(resolve_threads[t]));
		if (rc){
	    	fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
	    	exit(EXIT_FAILURE);
		}
	}

	int pthread_join_error;

	/* Wait For All Requester Threads To Finish */
    for(t=0;t<num_input_files;t++){
		pthread_join_error = pthread_join(requester_threads[t],NULL);
		if(pthread_join_error){
			fprintf(stderr, "ERROR; return code for pthread_join() for the requester threads is %d\n", pthread_join_error);
		}
    }
    printf("All of the requester threads completed!\n");
	
    
	/* Wait For All Resolver Threads To Finish */
    for(t=0;t<MAX_RESOLVER_THREADS;t++){
		pthread_join_error = pthread_join(resolver_threads[t],NULL);
		if(pthread_join_error){
			printf("ERROR; return code for pthread_join() for the resolver threads is %d\n", pthread_join_error);
		}
    }
    printf("All of the resolver threads completed!\n");

    /* Free Space Taken By Queue */
    queue_cleanup(&request_q);

	/* Close Output File */
    if(fclose(outputfp)){
		perror("Error Closing Output File");
		return EXIT_FAILURE;
    }

    /* Free Space Taken By Mutexes */
    //pthread_mutex_destroy(&queue_mutex);
    //pthread_mutex_destroy(&output_file_mutex);
    
    mutex_error = pthread_mutex_destroy(&queue_mutex);
    if(mutex_error){
    	fprintf(stderr, "ERROR; return code from pthread_mutex_destory() for the queue is %d\n", mutex_error);
    }
    mutex_error = pthread_mutex_destroy(&output_file_mutex);
    if (mutex_error){
	    fprintf(stderr, "ERROR; return code from pthread_mutex_destory() for the output file is %d\n", mutex_error);
	}
	


    return EXIT_SUCCESS;	
}

void* requester_thread_function(void* thread_information){

	/* Create A Pointer To the Thread Struct Coming Into The Function */
	struct thread_info* thread_info = thread_information;

	/* Create Pointers To The Information About The Thread In The Struct */
	FILE* requester_thread_file = thread_info->thread_file;
	pthread_mutex_t* queue_mutex = thread_info->queue_mutex;
	queue* req_queue = thread_info->request_queue;

	/* Initialize A Hostname With A Max Character Size Of 1025*/
	char hostname[SBUFSIZE];

	/* Initialize A Pointer To The Payload Which Will Be The URL From The Input File */
	char* payload;

	/* Variable To Make Sure Each Hostname Is Eventually Pushed To Queue Even If The Queue Is Full */
	int hostname_pushed_to_queue = 0;

	int mutex_lock_error, mutex_unlock_error;

	/* Read File and Process*/
	while(fscanf(requester_thread_file, INPUTFS, hostname) > 0){

		/* This Loop Makes Sure The Host Name Is Put On The Queue Even If It's Full */
		while(!hostname_pushed_to_queue){

			/* Lock The Queue So Only This Thread Can Access It */
	    	mutex_lock_error = pthread_mutex_lock(queue_mutex);
	    	if (mutex_lock_error){
	    		fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the queue is %d\n", mutex_lock_error);
			} 

			/* Check To See If The Queue Is Full.  If So, Make Thread Sleep Until The Queue Is Available */
			if(queue_is_full(req_queue)){
				fprintf(stderr, "queue_is_full reports that the queue is full\n");

				/* Unlock The Queue */
				mutex_unlock_error = pthread_mutex_unlock(queue_mutex);
				if (mutex_unlock_error){
	    			fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the queue is %d\n", mutex_unlock_error);
				} 

				/* Make Thread Sleep for 100 Microseconds */
		    	usleep(100);
	    	}
	    	else{

	    		/* Initialize Size Of Payload */
				payload = malloc(SBUFSIZE); 

	    		/* Copy Each Hostname Of Input File Into Payload */
	    		strncpy(payload, hostname, SBUFSIZE);

	    		if(queue_push(req_queue, payload) == QUEUE_FAILURE){
		    		fprintf(stderr, "error: queue_push failed!\n");
				}

				/* Unlock The Queue */
				mutex_unlock_error = pthread_mutex_unlock(queue_mutex);
				if (mutex_unlock_error){
	    			fprintf(stderr, "ERROR; return code from pthread_mutex_unlock() for the queue is %d\n", mutex_unlock_error);
				} 

				hostname_pushed_to_queue = 1;
	    	}
    	}

    	hostname_pushed_to_queue = 0;
	}	

	/* Close Input File */
	if(fclose(requester_thread_file)){
		perror("Error Closing Input File");
    }

	return NULL;
}


void* resolver_thread_function(void* thread_information){

	/* Create A Pointer To the Thread Struct Coming Into The Function */
	struct thread_info* thread_info = thread_information;

	/* Create Pointers To The Information About The Thread In The Struct */
	FILE* resolver_thread_file = thread_info->thread_file;
	pthread_mutex_t* queue_mutex = thread_info->queue_mutex;
	pthread_mutex_t* file_mutex = thread_info->file_mutex;
	queue* request_queue = thread_info->request_queue;

	/* Initialize A Pointer To The Payload Which Will Be The URL From The Input File */
	char* payload;
	

	/* Initialize Variable To Hold IP Address */
	char firstipstr[INET6_ADDRSTRLEN];

	int mutex_unlock_error, mutex_lock_error;

	/* Lock The Queue So Only This Thread Can Access It */
	/*
    mutex_lock_error = pthread_mutex_lock(queue_mutex);
    if (mutex_lock_error){
		fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the queue is %d\n", mutex_lock_error);
	}
	*/

	/* While The Queue Is Not Empty */
	while(!queue_is_empty(request_queue)){

		/* Lock The Queue So Only This Thread Can Access It */
    	mutex_lock_error = pthread_mutex_lock(queue_mutex);
    	if (mutex_lock_error){
			fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the queue is %d\n", mutex_lock_error);
		} 

    	/* Get Hostname Off Queue */
    	payload = queue_pop(request_queue);

    	if(payload == NULL){
    		fprintf(stderr, "Unable to pop anything off the queue becuase the queue is empty");
    	}

    	/* Unlock The Queue */
		mutex_unlock_error = pthread_mutex_unlock(queue_mutex);
		if (mutex_unlock_error){
	    	fprintf(stderr, "ERROR; return code from pthread_mutex_unlock() for the queue is %d\n", mutex_unlock_error);
		} 

		/* Lookup hostname and get IP string */
	    if(dnslookup(payload, firstipstr, sizeof(firstipstr)) == UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", payload);
			strncpy(firstipstr, "", sizeof(firstipstr));
	    }

	    /* Lock Output File In Order To Write To It */
	    mutex_lock_error = pthread_mutex_lock(file_mutex);
	    if (mutex_lock_error){
	    	fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the output file is %d\n", mutex_lock_error);
		} 

	    /* Write to Output File */
	    fprintf(resolver_thread_file, "%s,%s\n", payload, firstipstr);

	    /* Unlock Output File So Other Threads Can Write To It */
	    mutex_unlock_error = pthread_mutex_unlock(file_mutex);
	    if (mutex_unlock_error){
	    	fprintf(stderr, "ERROR; return code from pthread_mutex_unlock() for the output file is %d\n", mutex_unlock_error);
		} 

	    /* Free Memory Blocks On The Heap Created By Payload */
	    free(payload);
	}

	/* Unlock The Queue */
	/*
	mutex_unlock_error = pthread_mutex_unlock(queue_mutex);
	if (mutex_unlock_error){
	    fprintf(stderr, "ERROR; return code from pthread_mutex_lock() for the queue is %d\n", mutex_unlock_error);
	} 
	*/

	return NULL;
}
