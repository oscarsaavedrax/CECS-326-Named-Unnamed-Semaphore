/***********************************************************************
 * Programmer	: Oscar Saavedra
 * Class	    : CECS 326-01
 * Due Date	    : November 5, 2022
 * Description	: This program makes use of the POSIX implementation of
 *  the Linux shared memory mechanism. This master.c program takes in a
 *  name for the shared memory segment and the number of children to
 *  make. The program requests to create a shared memory segment. The
 *  program creates n children and passes each one the name of the shared
 *  memory segment and the child number. The program then waits for all
 *  children to execute. Once all children execute, the program outputs
 *  the content of the shared memory, removes the shared memory, and
 *  exits.
 ***********************************************************************/

#include "myShm.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <string.h>

int main(int argc, char **argv)
{
    // Create variables
    const int num_children = atoi(argv[1]);           // number of child processes to create
    const char *shared_mem_name = argv[2];            // name of shared memory
    const char *display_semaphore_name = argv[3];     // name of the named display semaphore
    const int SIZE = sizeof(struct SHARED_MEM_CLASS); // size of struct object
    struct SHARED_MEM_CLASS *shared_mem_struct;       // structure of shared memory
    int shared_mem_fd;                                // shared memory file descriptor

    // Display identification
    printf("Master begins execution\n");

    // Create shared memory segment
    shared_mem_fd = shm_open(shared_mem_name, O_CREAT | O_RDWR, 0666);
    if (shared_mem_fd == -1)
    {
        printf("\nMaster: Shared memory failed: %s\n", strerror(errno));
        exit(1);
    }
    else
    {
        // Resize the shared memory object to the size of the struct
        ftruncate(shared_mem_fd, SIZE);

        // Map the shared memory segment
        shared_mem_struct = mmap(NULL, SIZE, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, shared_mem_fd, 0);
        if (shared_mem_struct == MAP_FAILED)
        {
            printf("Master: Map failed: %s\n", strerror(errno));
            /* close and shm_unlink */
            exit(1);
        }
        else
            printf("Master created a shared memory segment named %s\n", shared_mem_name);
    }

    // Initialize shared memory index to 0
    shared_mem_struct->index = 0;

    // Create n children
    printf("Master created %d child processes to execute slave\n\n", num_children);
    for (int i = 0; i < num_children; i++)
    {
        // Fork a new child on each iteration
        if (fork() == 0)
        {
            // Convert child integer to char
            char child_num[20];
            sprintf(child_num, "%d", i + 1);

            // Create array with child information
            char *child_info[] = {child_num, argv[2], argv[3], NULL};

            // Child process will execute slave program
            execv("./slave", child_info);
            exit(0);
        }
    }

    // Wait for children to finish to terminate
    for (int i = 0; i < num_children; i++)
        wait(NULL);
    printf("\nMaster waits for all child processes to terminate\n");
    printf("Master received termination signals from all %d child processes\n", num_children);

    // Print contents of the shared memory
    printf("Content of shared memory segment filled by child processes:\n");
    for (int i = 0; i < shared_mem_struct->index; i++)
        printf("%d\n", shared_mem_struct->response[i]);

    // Unmap the shared memory structure
    if (munmap(shared_mem_struct, SIZE) == -1)
    {
        printf("Master: Unmap failed: %s\n", strerror(errno));
        exit(1);
    }

    // Close the shared memory segment
    if (close(shared_mem_fd) == -1)
    {
        printf("Master: Close failed: %s\n", strerror(errno));
        exit(1);
    }

    // Delete the shared memory segment
    shm_unlink(shared_mem_name);

    printf("Master removed shared memory segment and is exiting\n");

    return 0;
}