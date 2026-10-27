/*
 * An implementation of Round Robin scheduling.
 */

typedef struct rr_process {
    unsigned int pid;
    unsigned int processing_time;
    unsigned int arrival_time;
    unsigned int processed_time;
    struct rr_process *next_process;
} rr_process;

rr_process process_list = {0, 0, 0, 0, NULL};

/*
 * Prints out the list of all rr_processes after the given one.
 * parameters:
 *   node: The rr_process to print all later processes from
 */
void print_list(rr_process *node) {
    while (node->next_process) {
        rr_process *next = node->next_process;
        printf("pid: %d, processing_time %d, arrival_time: %d, processed_time: %d, next_process.pid: %d\n",
               next->pid,
               next->processing_time,
               next->arrival_time,
               next->processed_time,
               next->next_process ? next->next_process->pid : 0);
        node = node->next_process;
    }
}

/*
 * Adds the rr_process pointed to by next to be immediately after node.
 * parameters:
 *   node: The rr_process to add the next rr_process after
 *   next: The new next_process for node
 */
void add_next(rr_process *node, rr_process *next) {
    next->next_process = node->next_process;
    node->next_process = next;
}

/*
 * Removes the next_process after the given node.
 * If the next_process has a next_process, that next_process becomes node's next_process.
 * parameters:
 *   node: The rr_process to remove the immediate next_process from
 * returns:
 *   The rr_process removed from next to the given node
 */
rr_process *remove_next(rr_process *node) {
    rr_process *next = node->next_process;
    if (next) {
        node->next_process = next->next_process;
    }
    return next;
}

/*
 * Adds the given process to the ready queue, indicating it is ready to be scheduled.
 * Keeps a list of all processes, sorted by arrival time, then pid.
 * parameters:
 *   process - the process to add to the ready queue
 */
void add_to_ready_queue(const process_initial process) {
    rr_process *new_process = malloc(sizeof(rr_process));
    new_process->pid = process.pid;
    new_process->processing_time = process.processing_time;
    new_process->arrival_time = process.arrival_time;
    new_process->processed_time = 0;
    new_process->next_process = NULL;

    rr_process *end = &process_list;
    while (end->next_process && end->next_process->arrival_time < new_process->arrival_time) {
        end = end->next_process;
    }
    while (end->next_process && end->next_process->arrival_time == new_process->arrival_time && end->next_process->pid < new_process->pid) {
        end = end->next_process;
    }

    add_next(end, new_process);

    if (debug) {
        printf("Process list after adding process with pid %d in next step:\n", process.pid);
        print_list(&process_list);
        printf("\n");
    }
}

/*
 * Determines the next process to be scheduled.
 * Implements Round Robin, meaning it will cycle through each process in order of arrival and assign it
 * a quantum of CPU time, removing any process when it completes.
 * If two processes have the same arrival time, it will select the one with the lowest PID first.
 * returns:
 *   The PID of the process to be scheduled next, or 0 if no process should be scheduled
 */
unsigned int get_next_scheduled_process() {
    static int quantum = 0;
    static rr_process *current_process = NULL;
    unsigned int pid = 0;
    rr_process *node = &process_list;

    if (!current_process) {
        while (node->next_process) {
            rr_process *next = node->next_process;
            if (next->processed_time < next->processing_time) {
                current_process = next;
                break;
            }
            node = node->next_process;
        }
    }

    if (!current_process) {
        return 0;
    }

    pid = current_process->pid;
    current_process->processed_time++;
    quantum++;

    if (current_process->processed_time == current_process->processing_time) {
        rr_process *finished_process = current_process;
        current_process = NULL;
        quantum = 0;
        node = &process_list;
        // Find the node that points to the current node
        while (node->next_process) {
            if (node->next_process->pid == finished_process->pid) {
                remove_next(node);
                free(finished_process);
                break;
            }
            node = node->next_process;
        }
        if (debug) {
            printf("Process list after next step (pid %d has completed in next step):\n", pid);
            print_list(&process_list);
            printf("\n");
        }
    } else if (quantum == 3) {
        rr_process *expired_process = current_process;
        current_process = NULL;
        quantum = 0;
        node = &process_list;
        while (node->next_process) {
            if (node->next_process->pid == expired_process->pid) {
                remove_next(node);
                break;
            }
            node = node->next_process;
        }
        rr_process *end = &process_list;
        while (end->next_process) {
            end = end->next_process;
        }
        add_next(end, expired_process);

        if (debug) {
            printf("Process list after next step (pid %d has completed its quantum in next step):\n", pid);
            print_list(&process_list);
            printf("\n");
        }
    }

    return pid;
}
