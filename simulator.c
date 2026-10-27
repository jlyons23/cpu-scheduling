/*
 * Driver for the scheduling algorithms.
 *
 * Reads a schedule file, advances one time unit at a time, hands each process
 * to the algorithm as it arrives and asks the algorithm which pid to run.
 * Records when each process finishes and reports turnaround and waiting times.
 *
 * Usage: ./<algorithm> <schedule file> [-d]
 */

#include "scheduler.h"
#include <string.h>

#define MAX_PROCESSES 1024
#define MAX_TIME 1000000

int debug = 0;

typedef struct {
    unsigned int pid;
    unsigned int arrival_time;
    unsigned int processing_time;
    unsigned int ran_for;
    unsigned int finished_at;
    int admitted;
} record;

static record records[MAX_PROCESSES];
static int process_count = 0;

/*
 * Finds the record the simulator keeps for a given pid.
 * parameters:
 *   pid: The process id to look up
 * returns:
 *   A pointer to that process's record, or NULL if no process has that pid
 */
static record *find_record(unsigned int pid) {
    for (int i = 0; i < process_count; i++) {
        if (records[i].pid == pid) {
            return &records[i];
        }
    }
    return NULL;
}

/*
 * Reads a schedule file into the records array.
 * The first line is the number of processes, then one "pid,arrival,service" line each.
 * parameters:
 *   path: The path of the schedule file to read
 * returns:
 *   0 if the whole file was read, 1 if it could not be opened or parsed
 */
static int read_schedule(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Could not open schedule file '%s'\n", path);
        return 1;
    }

    int declared = 0;
    if (fscanf(f, "%d", &declared) != 1) {
        fprintf(stderr, "Schedule file '%s' does not start with a process count\n", path);
        fclose(f);
        return 1;
    }
    if (declared < 1 || declared > MAX_PROCESSES) {
        fprintf(stderr, "Process count %d is out of range (1 to %d)\n", declared, MAX_PROCESSES);
        fclose(f);
        return 1;
    }

    for (int i = 0; i < declared; i++) {
        unsigned int pid, arrival, service;
        if (fscanf(f, " %u,%u,%u", &pid, &arrival, &service) != 3) {
            fprintf(stderr, "Could not read process %d of %d from '%s'\n", i + 1, declared, path);
            fclose(f);
            return 1;
        }
        if (pid == 0) {
            fprintf(stderr, "pid 0 is reserved to mean 'CPU idle' and cannot be used\n");
            fclose(f);
            return 1;
        }
        if (service == 0) {
            fprintf(stderr, "Process %u has a service time of 0\n", pid);
            fclose(f);
            return 1;
        }
        records[i].pid = pid;
        records[i].arrival_time = arrival;
        records[i].processing_time = service;
        records[i].ran_for = 0;
        records[i].finished_at = 0;
        records[i].admitted = 0;
    }

    fclose(f);
    process_count = declared;
    return 0;
}

/*
 * Prints the per process figures and the run summary.
 * Called once every process has completed.
 * parameters:
 *   total_time: The number of time units the run took
 *   idle_time: How many of those time units had no process running
 */
static void report(unsigned int total_time, unsigned int idle_time) {
    double total_turnaround = 0.0;
    double total_waiting = 0.0;

    printf("\n pid  arrival  service  finished  turnaround  waiting\n");
    printf("----  -------  -------  --------  ----------  -------\n");

    for (int i = 0; i < process_count; i++) {
        record *r = &records[i];
        unsigned int turnaround = r->finished_at - r->arrival_time;
        unsigned int waiting = turnaround - r->processing_time;
        total_turnaround += turnaround;
        total_waiting += waiting;

        printf("%4u  %7u  %7u  %8u  %10u  %7u\n",
               r->pid, r->arrival_time, r->processing_time,
               r->finished_at, turnaround, waiting);
    }

    printf("\nProcesses:          %d\n", process_count);
    printf("Total time:         %u\n", total_time);
    printf("CPU idle:           %u\n", idle_time);
    printf("Average turnaround: %.2f\n", total_turnaround / process_count);
    printf("Average waiting:    %.2f\n", total_waiting / process_count);
}

/*
 * Parses the arguments, reads the schedule, then runs the simulation one time unit at a
 * time until every process has completed.
 * parameters:
 *   argc: The argument count
 *   argv: The arguments, expected to be a schedule file and an optional -d flag
 * returns:
 *   0 if the run completed, 1 on a bad argument, an unreadable schedule or a
 *   scheduling error
 */
int main(int argc, char **argv) {
    const char *schedule_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            debug = 1;
        } else if (!schedule_path) {
            schedule_path = argv[i];
        } else {
            fprintf(stderr, "Usage: %s <schedule file> [-d]\n", argv[0]);
            return 1;
        }
    }

    if (!schedule_path) {
        fprintf(stderr, "Usage: %s <schedule file> [-d]\n", argv[0]);
        return 1;
    }

    if (read_schedule(schedule_path)) {
        return 1;
    }

    unsigned int time = 0;
    unsigned int idle_time = 0;
    int completed = 0;

    while (completed < process_count) {
        if (time > MAX_TIME) {
            fprintf(stderr, "Giving up at time %u with %d of %d processes finished\n",
                    time, completed, process_count);
            return 1;
        }

        for (int i = 0; i < process_count; i++) {
            if (!records[i].admitted && records[i].arrival_time == time) {
                process_initial p;
                p.pid = records[i].pid;
                p.arrival_time = records[i].arrival_time;
                p.processing_time = records[i].processing_time;
                records[i].admitted = 1;
                add_to_ready_queue(p);
            }
        }

        unsigned int pid = get_next_scheduled_process();

        if (pid == 0) {
            /* Nothing to run. Either waiting on an arrival or the algorithm has stalled. */
            int outstanding = 0;
            for (int i = 0; i < process_count; i++) {
                if (records[i].admitted &&
                    records[i].ran_for < records[i].processing_time) {
                    outstanding++;
                }
            }
            if (outstanding > 0) {
                fprintf(stderr, "Algorithm returned 0 at time %u with %d process(es) still ready\n",
                        time, outstanding);
                return 1;
            }
            idle_time++;
            if (debug) {
                printf("t=%u: idle\n", time);
            }
        } else {
            record *r = find_record(pid);
            if (!r) {
                fprintf(stderr, "Algorithm returned unknown pid %u at time %u\n", pid, time);
                return 1;
            }
            if (!r->admitted) {
                fprintf(stderr, "Algorithm ran pid %u at time %u before it arrived\n", pid, time);
                return 1;
            }
            if (r->ran_for >= r->processing_time) {
                fprintf(stderr, "Algorithm ran pid %u at time %u after it had finished\n", pid, time);
                return 1;
            }

            r->ran_for++;
            if (debug) {
                printf("t=%u: running pid %u (%u/%u)\n", time, pid, r->ran_for, r->processing_time);
            }

            if (r->ran_for == r->processing_time) {
                r->finished_at = time + 1;
                completed++;
            }
        }

        time++;
    }

    report(time, idle_time);
    return 0;
}
