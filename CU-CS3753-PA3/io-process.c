/*
 * Description: A small i/o bound program to copy N bytes from an input
 *              file to an output file for specific scheduling policy
 *				and number of processes. 
 */

/* Include Flags */
#define _GNU_SOURCE

/* System Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sched.h>

/* Local Defines */
#define MAXFILENAMELENGTH 80
#define DEFAULT_INPUTFILENAME "rwinput"
#define DEFAULT_OUTPUTFILENAMEBASE "rwoutput"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_TRANSFERSIZE 1024*100

/* # Of Process Instances */
#define LOW 10
#define MEDIUM 50
#define HIGH 100

/* For Forking Processes */
#include <sys/types.h> 
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]){

    int rv, num_processes, policy, status, i;
    int inputFD;
    int outputFD;
    char inputFilename[MAXFILENAMELENGTH];
    char outputFilename[MAXFILENAMELENGTH];
    char outputFilenameBase[MAXFILENAMELENGTH];

    ssize_t transfersize = DEFAULT_TRANSFERSIZE;
    ssize_t blocksize = DEFAULT_BLOCKSIZE; 
    char* transferBuffer = NULL;
    ssize_t buffersize;

    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;
    int totalReads = 0;
    ssize_t bytesWritten = 0;
    ssize_t totalBytesWritten = 0;
    int totalWrites = 0;
    int inputFileResets = 0;
    struct sched_param param;
    pid_t pid, wpid;
    int j = 0;

    /* Set input and output filename and size */
    strncpy(inputFilename, DEFAULT_INPUTFILENAME, MAXFILENAMELENGTH);
    strncpy(outputFilenameBase, DEFAULT_OUTPUTFILENAMEBASE, MAXFILENAMELENGTH);

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
		    /* Confirm blocksize is multiple of and less than transfersize*/
		    if(blocksize > transfersize){
			fprintf(stderr, "blocksize can not exceed transfersize\n");
			exit(EXIT_FAILURE);
		    }
		    if(transfersize % blocksize){
			fprintf(stderr, "blocksize must be multiple of transfersize\n");
			exit(EXIT_FAILURE);
		    }

		    /* Allocate buffer space */
		    buffersize = blocksize;
		    if(!(transferBuffer = malloc(buffersize*sizeof(*transferBuffer)))){
			perror("Failed to allocate transfer buffer");
			exit(EXIT_FAILURE);
		    }
			
		    /* Open Input File Descriptor in Read Only mode */
		    if((inputFD = open(inputFilename, O_RDONLY | O_SYNC)) < 0){
			perror("Failed to open input file");
			exit(EXIT_FAILURE);
		    }

		    /* Open Output File Descriptor in Write Only mode with standard permissions*/
		    rv = snprintf(outputFilename, MAXFILENAMELENGTH, "%s-%d",
				  outputFilenameBase, getpid());    
		    if(rv > MAXFILENAMELENGTH){
			fprintf(stderr, "Output filenmae length exceeds limit of %d characters.\n",
				MAXFILENAMELENGTH);
			exit(EXIT_FAILURE);
		    }
		    else if(rv < 0){
			perror("Failed to generate output filename");
			exit(EXIT_FAILURE);
		    }
		    if((outputFD =
			open(outputFilename,
			     O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
			     S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0){
			perror("Failed to open output file");
			exit(EXIT_FAILURE);
		    }

		    /* Print Status */
		    fprintf(stdout, "Reading from %s and writing to %s\n",
			    inputFilename, outputFilename);

		    /* Read from input file and write to output file*/
		    do{
			/* Read transfersize bytes from input file*/
			bytesRead = read(inputFD, transferBuffer, buffersize);
			if(bytesRead < 0){
			    perror("Error reading input file");
			    exit(EXIT_FAILURE);
			}
			else{
			    totalBytesRead += bytesRead;
			    totalReads++;
			}
			
			/* If all bytes were read, write to output file*/
			if(bytesRead == blocksize){
			    bytesWritten = write(outputFD, transferBuffer, bytesRead);
			    if(bytesWritten < 0){
				perror("Error writing output file");
				exit(EXIT_FAILURE);
			    }
			    else{
				totalBytesWritten += bytesWritten;
				totalWrites++;
			    }
			}
			/* Otherwise assume we have reached the end of the input file and reset */
			else{
			    if(lseek(inputFD, 0, SEEK_SET)){
				perror("Error resetting to beginning of file");
				exit(EXIT_FAILURE);
			    }
			    inputFileResets++;
			}
			
		    }while(totalBytesWritten < transfersize);

		    /* Output some possibly helpfull info to make it seem like we were doing stuff */
		    fprintf(stdout, "Read:    %zd bytes in %d reads\n",
			    totalBytesRead, totalReads);
		    fprintf(stdout, "Written: %zd bytes in %d writes\n",
			    totalBytesWritten, totalWrites);
		    fprintf(stdout, "Read input file in %d pass%s\n",
			    (inputFileResets + 1), (inputFileResets ? "es" : ""));
		    fprintf(stdout, "Processed %zd bytes in blocks of %zd bytes\n",
			    transfersize, blocksize);
			
		    /* Free Buffer */
		    free(transferBuffer);

		    /* Close Output File Descriptor */
		    if(close(outputFD)){
			perror("Failed to close output file");
			exit(EXIT_FAILURE);
		    }

		    /* Close Input File Descriptor */
		    if(close(inputFD)){
			perror("Failed to close input file");
			exit(EXIT_FAILURE);
		    }

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
