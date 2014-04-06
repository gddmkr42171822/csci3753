/*
 * 	This file contains a simple program for statistically
 *      calculating pi using a specific scheduling policy
 *      and number of processes.
 */

/* Local Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>

/* For Forking Processes */
#include <sys/types.h> 
#include <unistd.h>
#include <sys/wait.h>

#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)

/* # Of Process Instances */
#define LOW 10
#define MEDIUM 50
#define HIGH 100

inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

int main(int argc, char* argv[]){

    long i;
    long iterations = DEFAULT_ITERATIONS;
    struct sched_param param;
    int policy, num_processes, status;
    double x, y;
    double inCircle = 0.0;
    double inSquare = 0.0;
    double pCircle = 0.0;
    double piCalc = 0.0;
    pid_t pid, wpid;
    int j = 0;

    /* Set default policy if not supplied */
    if(argc < 2){
        policy = SCHED_OTHER;
    }

    /* Set default number of processes */
    if(argc < 3){
        num_processes = LOW;
    }
    /* Set policy if supplied */
    if(argc > 1){
	if(!strcmp(argv[1], "SCHED_OTHER")){
	    policy = SCHED_OTHER;
	}
	else if(!strcmp(argv[1], "SCHED_FIFO")){
	    policy = SCHED_FIFO;
	}
	else if(!strcmp(argv[1], "SCHED_RR")){
	    policy = SCHED_RR;
	}
	else{
	    fprintf(stderr, "Unhandeled scheduling policy\n");
	    exit(EXIT_FAILURE);
	}
    }

    /* Set # Of Processes */
    if(argc > 2){
        if(!strcmp(argv[2], "LOW")){
            num_processes = LOW;
        }
        else if(!strcmp(argv[2], "MEDIUM")){
            num_processes = MEDIUM;
        }
        else if(!strcmp(argv[2], "HIGH")){
            num_processes = HIGH;
        }
        else{
            fprintf(stderr, "Unhandeled number of processes\n");
            exit(EXIT_FAILURE);
        }
    }
    
    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);
    
    /* Set new scheduler policy */
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
    if(sched_setscheduler(0, policy, &param)){
	perror("Error setting scheduler policy");
	exit(EXIT_FAILURE);
    }
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

    /* Fork number of processes specified */
    printf("Number of processes to be forked %d \n", num_processes);
    for(i = 0; i < num_processes; i++){
        if((pid = fork())==-1){
            fprintf(stderr, "Error Forking Child Process");
            exit(EXIT_FAILURE); /*Fork Failed*/
        } 
        if(pid == 0){ /* Child Process */
            /* Calculate pi using statistical methode across all iterations*/
            for(i=0; i<iterations; i++){
            x = (random() % (RADIUS * 2)) - RADIUS;
            y = (random() % (RADIUS * 2)) - RADIUS;
            if(zeroDist(x,y) < RADIUS){
                inCircle++;
            }
            inSquare++;
            }

            /* Finish calculation */
            pCircle = inCircle/inSquare;
            piCalc = pCircle * 4.0;

            /* Print result */
            fprintf(stdout, "pi = %f\n", piCalc);

            exit(0);
        }
    }
    /* Parent Process Waits For All Of the children to terminate */
    while((wpid = wait(&status)) > 0){
        if(WIFEXITED(status)){ /*Process terminated normally */
        printf("Exit status of child process %d was normal \n", wpid);
        j++;
        }
    }
    printf("Total # of Processes terminated was %d\n", j);
    return EXIT_SUCCESS;  
}
