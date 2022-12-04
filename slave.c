/***********************************************************************
 * Programmer	: Oscar Saavedra
 * Class	    : CECS 326-01
 * Due Date	    : November 5, 2022
 * Description	: This program makes use of the POSIX implementation of
 *  the Linux shared memory mechanism. This slave.c program receives its
 *  number and name of the shared memory segment via commandline
 *  arugments from master.c program. This program then outputs a message
 *  to identify itself. The program opens the existing shared memory
 *  segment, acquires access to it, and write its child number into the
 *  next available slot in the shared memory. The pgrogram then closes
 *  the shared memory segment and terminates.
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
#include <semaphore.h>

int main(int argc, char **argv)
{
    // Create variables
    const int child_num = atoi(argv[0]);              // child number
    const char *shared_mem_name = argv[1];            // name of shared memory
    const char *display_semaphore_name = argv[2];     // name of display semaphore
    const int SIZE = sizeof(struct SHARED_MEM_CLASS); // size of struct object
    struct SHARED_MEM_CLASS *shared_mem_struct;       // structure of shared memory
    int shared_mem_fd;                                // shared memory file descriptor
    int local_index;                                  // Local variable for index from shared memory
    sem_t shared_memory_semaphore;                    // uunamed semaphore for shared memory access

    // Create a named semaphore for displaying output
    sem_t *display_sem = sem_open(display_semaphore_name, O_CREAT, 0660, 1);
    if (display_sem == SEM_FAILED)
    {
        printf("Slave: sem_open failed: %s\n", strerror(errno));
        exit(1);
    }

    // Acquire access to monitor for output
    if (sem_wait(display_sem) == -1)
    {
        printf("Slave: sem_wait(display_sem) failed: %s\n", strerror(errno));
        exit(1);
    }

    // Print child number and shared memory name
    printf("\nSlave begins execution\n");
    printf("I am child number %d, received shared memory name %s\n", child_num, shared_mem_name);

    // Done with output. Unlock monitor
    if (sem_post(display_sem) == -1)
    {
        printf("Slave: sem_post(display_sem) failed: %s\n", strerror(errno));
        exit(1);
    }

    // Open the shared memory segment
    shared_mem_fd = shm_open(shared_mem_name, O_RDWR, 0666);
    if (shared_mem_fd == -1)
    {
        printf("\nSlave %d: Shared memory failed: %s\n", child_num, strerror(errno));
        exit(1);
    }
    else
    {
        // Map the shared memory segment
        shared_mem_struct = mmap(NULL, SIZE, PROT_WRITE, MAP_SHARED,
                                 shared_mem_fd, 0);
        if (shared_mem_struct == MAP_FAILED)
        {
            printf("\nSlave %d: Map failed: %s\n", child_num, strerror(errno));
            /* close and shm_unlink */
            exit(1);
        }
        else
        {

            // Acquire access to monitor for output
            if (sem_wait(display_sem) == -1)
            {
                printf("Slave: sem_wait(display_sem) failed: %s\n", strerror(errno));
                exit(1);
            }

            printf("Slave acquires access to shared memory segment, and structures it according to struct SHARED_MEM_CLASS\n");

            // Done with output. Unlock monitor
            if (sem_post(display_sem) == -1)
            {
                printf("Slave: sem_post(display_sem) failed: %s\n", strerror(errno));
                exit(1);
            }

            // Initialize unnamed semaphore to 1 and nanoset it to shared
            if (sem_init(&(shared_memory_semaphore), 0, 1) == -1)
            {
                printf("Slave: sem_init failed: %s\n", strerror(errno));
                exit(1);
            }

            // Critical section to write to shared memory
            if (sem_wait(&(shared_memory_semaphore)) == -1)
            {
                printf("Slave: sem_wait failed: %s\n", strerror(errno));
                exit(1);
            }
            // Acquire access to monitor for output
            if (sem_wait(display_sem) == -1)
            {
                printf("Slave: sem_wait(display_sem) failed: %s\n", strerror(errno));
                exit(1);
            }

            // Copy shared memory index to local variable index
            local_index = shared_mem_struct->index;
            printf("Slave copies index to local variable local_index\n");

            // Write to the shared memory segment
            shared_mem_struct->response[shared_mem_struct->index] = child_num;
            shared_mem_struct->index += 1;
            printf("Slave writes its child number in response[%d]\n", local_index);
            printf("Slave increments index\n");

            // Done with output. Unlock monitor
            if (sem_post(display_sem) == -1)
            {
                printf("Slave: sem_post(display_sem) failed: %s\n", strerror(errno));
                exit(1);
            }
            // Exit critical section after writing to shared memory
            if (sem_post(&(shared_memory_semaphore)) == -1)
            {
                printf("slave: sem_post failed: %s\n", strerror(errno));
                exit(1);
            }

            // Done needing semphore, close it to free it up
            if (sem_destroy(&(shared_memory_semaphore)) == -1)
            {
                printf("Slave: sem_destroy failed: %s\n", strerror(errno));
                exit(1);
            }
        }

        // Unmap the shared memory structure
        if (munmap(shared_mem_struct, SIZE) == -1)
        {
            printf("\nSlave %d: Unmap failed: %s\n", child_num, strerror(errno));
            exit(1);
        }

        // Close the shared memory segment
        if (close(shared_mem_fd) == -1)
        {
            printf("\nSlave %d: Close failed: %s\n", child_num, strerror(errno));
            exit(1);
        }

        // Acquire access to monitor for output
        if (sem_wait(display_sem) == -1)
        {
            printf("Slave: sem_wait(display_sem) failed: %s\n", strerror(errno));
            exit(1);
        }

        printf("Slave closes access to shared memory segment\n");
        printf("I have written my child number [%d] to response[%d] in shared memory\n", child_num, local_index);
        printf("Slave closed access to shared memory and terminates\n");

        // Done with output. Unlock monitor
        if (sem_post(display_sem) == -1)
        {
            printf("Slave: sem_post(display_sem) failed: %s\n", strerror(errno));
            exit(1);
        }

        // Done using monitor, close display semaphore and remove its name
        if (sem_close(display_sem) == -1)
        {
            printf("Slave: sem_close(display_sem) failed: %s\n", strerror(errno));
            exit(1);
        }
       /* if (sem_unlink(display_semaphore_name) == -1)
        {
            printf("Slave: sem_unlink(display_semaphore_name) failed: %s\n", strerror(errno));
            exit(1);
        }*/
    }

    return 0;
}
