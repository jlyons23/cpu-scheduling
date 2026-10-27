/*
 * An implementation of Constant Feedback scheduling.
 */

typedef struct constant_process {
    unsigned int pid;
    unsigned int processing_time;
    unsigned int arrival_time;
    unsigned int processed_time;
    struct constant_process *next_process;
} constant_process;

constant_process process_heads[] = {
    {0, 0, 0, 0, NULL}, // Queue 1 (Highest priority)
    {0, 0, 0, 0, NULL},
    {0, 0, 0, 0, NULL},
    {0, 0, 0, 0, NULL}  // Queue 4 (Lowest priority)
};

constant_process *queues[] = {&process_heads[0], &process_heads[1], &process_heads[2], &process_heads[3]};

/*
 * Prints out the list of all constant_processes after the given one.
 * parameters:
 *   node: The constant_process to print all later processes from
 */
void print_list(constant_process *node) {
    while (node->next_process) {
        constant_process *next = node->next_process;
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
 * Adds the constant_process pointed to by next to be immediately after node.
 * parameters:
 *   node: The constant_process to add the next constant_process after
 *   next: The new next_process for node
 */
void add_next(constant_process *node, constant_process *next) {
    next->next_process = node->next_process;
    node->next_process = next;
}

/*
 * Removes the next_process after the given node.
 * If the next_process has a next_process, that next_process becomes node's next_process.
 * parameters:
 *   node: The constant_process to remove the immediate next_process from
 * returns:
 *   The constant_process removed from next to the given node
 */
constant_process *remove_next(constant_process *node) {
    constant_process *next = node->next_process;
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
    constant_process *new_process = malloc(sizeof(constant_process));
    new_process->pid = process.pid;
    new_process->processing_time = process.processing_time;
    new_process->arrival_time = process.arrival_time;
    new_process->processed_time = 0;
    new_process->next_process = NULL;

    constant_process *queue_head = queues[0];
    constant_process *end = queue_head;

    while (end->next_process && end->next_process->arrival_time < new_process->arrival_time) {
        end = end->next_process;
    }

    while (end->next_process && end->next_process->arrival_time == new_process->arrival_time && end->next_process->pid < new_process->pid) {
        end = end->next_process;
    }

    add_next(end, new_process);

    if (debug) {
        printf("Process list after adding process with pid %d:\n", process.pid);
        for (int i = 0; i < 4; i++) {
            printf("Queue %d:\n", i + 1);
            print_list(queues[i]);
        }
    }
}

/*
 * Determines the next process to be scheduled.
 * Implements Constant Feedback, which assigns processes to a set of priority queues,
 * where each queue has a fixed time quantum.
 * If two processes have the same arrival time, it will select the one with the lowest PID first.
 * returns:
 *   The PID of the process to be scheduled next, or 0 if no process should be scheduled
 */
unsigned int get_next_scheduled_process() {
    static int quantum = 0;
    static constant_process *current_process = NULL;
    static int current_queue = 0;
    unsigned int pid = 0;

    if (!current_process) {
        for (int i = 0; i < 4; i++) {
            constant_process *node = queues[i];

            while (node->next_process) {
                constant_process *next = node->next_process;

                if (next->processed_time < next->processing_time) {
                    current_process = next;
                    // Store the queue that the current process was pulled from so we know where to add it later
                    current_queue = i;
                    remove_next(node);
                    break;
                }
                node = node->next_process;
            }

            if (current_process) {
                break;
            }
        }
    }
    if (!current_process) {
        return 0;
    }

    pid = current_process->pid;
    current_process->processed_time++;
    quantum++;

    if (current_process->processed_time == current_process->processing_time) {
        free(current_process);
        current_process = NULL;
        quantum = 0;

        if (debug) {
            printf("Process list after next step (process with pid %d completes):\n", pid);
            for (int i = 0; i < 4; i++) {
                printf("Queue %d:\n", i + 1);
                print_list(queues[i]);
            }
        }
    } else if (quantum == 3) {
        constant_process *expired_process = current_process;
        current_process = NULL;
        quantum = 0;

        // If it's in the lowest priority queue, reinsert into the same queue
        if (current_queue == 3) {
            constant_process *end = queues[current_queue];
            while (end->next_process) {
                end = end->next_process;
            }
            add_next(end, expired_process);
        } else {
            constant_process *end = queues[current_queue + 1];
            while (end->next_process) {
                end = end->next_process;
            }
            add_next(end, expired_process);
        }

        if (debug) {
            printf("Process list after next step (process with pid %d completed its quantum in next step):\n", pid);
            for (int i = 0; i < 4; i++) {
                printf("Queue %d:\n", i + 1);
                print_list(queues[i]);
            }
        }
    }
    return pid;
}
