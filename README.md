# CPU Scheduling Algorithms

Four CPU scheduling algorithms in C, with a driver that runs a process schedule through any
one of them and reports turnaround and waiting times.

Each algorithm implements two functions: `add_to_ready_queue()`, called when a process
arrives, and `get_next_scheduled_process()`, called once per time unit to ask which process
holds the CPU. It owns its own queues, each a singly linked list with a dummy head node.

## Running It

```
make
./custom schedules/spec_schedule.txt
```

`make` builds one executable per algorithm: `rr`, `constant`, `exponential` and `custom`.
`-d` prints queue state at every step.

A schedule file starts with the process count, then one `pid,arrival,service` line each:

```
3
1,0,10
2,1,1
3,2,15
```

## The Algorithms

**`rr.c`** Round robin, quantum 3. One queue ordered by arrival time then pid. A process
that uses its full quantum goes to the back.

**`constant.c`** Multilevel feedback, four queues, quantum 3 in all of them. Arrivals enter
the highest priority queue. A process that uses its full quantum drops one level. The
lowest queue cycles into itself.

**`exponential.c`** Same structure, quantum scaled per level at 2k², so 2, 8, 18 and 32.

**`custom.c`** Preemptive HRRN, below.

## Preemptive HRRN

This selects processes based on their response ratio and assigns them a fixed time quantum
for execution before potentially preempting them. The response ratio is

```
Response Ratio = (Waiting time + Service time) / Service time
```

where

```
Waiting time = (Current time - Arrival time) - Processed time
```

Each time `get_next_scheduled_process()` is called, the algorithm evaluates all processes in
the queue and selects the one with the highest response ratio. If multiple processes have
the same ratio, it selects the process with the lowest PID. Once the quantum expires, all
processes are re-evaluated on their updated response ratios and the process with the
highest ratio is selected for the next execution cycle. When a process completes it is
removed from the queue and its memory is freed.

### Why HRRN

One of the key limitations of HRRN is that it requires knowledge of each process's service
time in advance. That information is available here, so I believed HRRN would be an
effective and suitable choice.

The other advantage is more balanced and fair scheduling, with less starvation than
non-preemptive algorithms like FCFS and SPN. My reasoning was that a response ratio would
ensure processes that have been waiting a long time and processes that only take a short
time to complete are given priority, reducing turnaround time.

I implemented preemption because I believed it would offer the most balanced approach to
handling a diverse range of schedules. Where a schedule includes processes with very long
service times followed by short ones, turnaround time suffers significantly without it.
Through experimentation I found that a very short quantum provided the best performance
overall, and the implementation uses 1. While a quantum that short would be inefficient in
practice due to context switching overhead, the simulator assumes zero overhead, which made
it an opportunity to optimise performance this way.

## Results

`schedules/spec_schedule.txt`, ten processes arriving one per time unit, service times 1 to 25.

| Algorithm | Average turnaround | Average waiting |
|---|---|---|
| Round robin | 28.70 | 21.00 |
| Constant feedback | 27.50 | 19.80 |
| Exponential feedback | 22.60 | 14.90 |
| Preemptive HRRN | 19.50 | 11.80 |

All four finish at time 77 with no idle CPU, so the difference is ordering alone.

## The Driver

`simulator.c` reads the schedule, advances one time unit at a time, admits arrivals and
calls `get_next_scheduled_process()`. It records completion times and prints the table.

It also validates the answer. A pid that does not exist, has not arrived or has already
finished stops the run, as does a 0 returned while processes are still ready.

The algorithm sources carry no `#include` lines. The makefile force includes `scheduler.h`
into every translation unit with `gcc -include scheduler.h`.
