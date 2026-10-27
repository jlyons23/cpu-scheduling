/*
 * Interface between the simulator and whichever scheduling algorithm is linked in.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdio.h>
#include <stdlib.h>

/* The description of a process, handed to the scheduler when the process arrives. */
typedef struct process_initial {
    unsigned int pid;
    unsigned int processing_time;
    unsigned int arrival_time;
} process_initial;

/* Set by the simulator from the -d flag. Algorithms print queue state when it is set. */
extern int debug;

/* Provided by the algorithm. Called once when a process arrives. */
void add_to_ready_queue(const process_initial process);

/*
 * Provided by the algorithm. Called once per time unit.
 * Returns the pid to run for that time unit or 0 if the CPU should idle.
 */
unsigned int get_next_scheduled_process();

#endif
