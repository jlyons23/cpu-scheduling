/*
 * An implementation of Preemptive Highest Response Ratio Next (HRRN) scheduling.
 */

typedef struct hrrn_process {
    unsigned int pid;
    unsigned int processing_time;
    unsigned int arrival_time;
    unsigned int processed_time;
    struct hrrn_process *next_process;
} hrrn_process;

hrrn_process process_list = {0, 0, 0, 0, NULL};

/*
 * Prints out the list of all hrrn_processes after the given one.
 * parameters:
 *   node: The hrrn_process to print all later processes from
 */
void print_list(hrrn_process *node) {
    while (node->next_process) {
        hrrn_process *next = node->next_process;
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
 * Adds the hrrn_process pointed to by next to be immediately after node.
 * parameters:
 *   node: The hrrn_process to add the next hrrn_process after
 *   next: The new next_process for node
 */
void add_next(hrrn_process *node, hrrn_process *next) {
    next->next_process = node->next_process;
    node->next_process = next;
}

/*
 * Removes the next_process after the given node.
 * If the next_process has a next_process, that next_process becomes node's next_process.
 * parameters:
 *   node: The hrrn_process to remove the immediate next_process from
 * returns:
 *   The hrrn_process removed from next to the given node
 */
hrrn_process *remove_next(hrrn_process *node) {
    hrrn_process *next = node->next_process;
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
    hrrn_process *new_process = malloc(sizeof(hrrn_process));
    new_process->pid = process.pid;
    new_process->processing_time = process.processing_time;
    new_process->arrival_time = process.arrival_time;
    new_process->processed_time = 0;
    new_process->next_process = NULL;

    hrrn_process *end = &process_list;
    while (end->next_process && end->next_process->arrival_time < new_process->arrival_time) {
        end = end->next_process;
    }
    while (end->next_process && end->next_process->arrival_time == new_process->arrival_time && end->next_process->pid < new_process->pid) {
        end = end->next_process;
    }

    add_next(end, new_process);

    if (debug) {
        printf("Process list after adding process with pid %d:\n", process.pid);
        print_list(&process_list);
    }
}

/*
 * Calculates the response ratio of a process:
 * RR = (Wait Time + Service Time) / Service Time
 * parameters:
 *   process - the process to calculate the response ratio for
 *   current_time - the current time in the system
 */
double calculate_rr(hrrn_process *process, unsigned int current_time) {
    unsigned int wait_time = (current_time - process->arrival_time) - process->processed_time;
    unsigned int service_time = process->processing_time;
    return (double)(wait_time + service_time) / service_time;
}

/*
 * Determines the next process to be scheduled.
 * Implements a preemptive version of HRRN, where processes with the highest response ratio
 * are served the CPU first. Preemption is achieved using quanta.
 * If two processes have the same response ratio, it will select the one with the lowest PID first.
 * returns:
 *   The PID of the process to be scheduled next, or 0 if no process should be scheduled
 */
unsigned int get_next_scheduled_process() {
    static int quantum = 0;
    static hrrn_process *current_process = NULL;
    unsigned int pid = 0;
    hrrn_process *node = &process_list;
    static int current_time = 0;

    if (!current_process) {
        double highest_rr = -1;
        hrrn_process *selected_process = NULL;
        while (node->next_process) {
            hrrn_process *next = node->next_process;

            double rr = calculate_rr(next, current_time);

            if (rr > highest_rr || (rr == highest_rr && next->pid < selected_process->pid)) {
                selected_process = next;
                highest_rr = rr;
            }
            node = node->next_process;
        }
        current_process = selected_process;
    }

    if (!current_process) {
        // An idle step is still a step, so the clock has to move even with nothing running
        current_time++;
        return 0;
    }

    pid = current_process->pid;
    current_process->processed_time++;
    quantum++;
    current_time++;

    if (current_process->processed_time == current_process->processing_time) {
        hrrn_process *finished_process = current_process;
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
    } else if (quantum == 1) {
        // Quantum expired. The process stays in the list and is reconsidered next step
        current_process = NULL;
        quantum = 0;

        if (debug) {
            printf("Process list after next step (pid %d has completed its quantum in next step):\n", pid);
            print_list(&process_list);
            printf("\n");
        }
    }

    return pid;
}
