#ifndef P1_SIMULATOR_H
#define P1_SIMULATOR_H

#include <stdbool.h>

typedef enum { FIFO, LRU } ReplacementAlgo;

typedef struct {
    int frame_id;
    int process_id;
    int page_number;
    int load_time;
    int last_access_time;
} Frame;

typedef struct {
    int pid;
    int memory_size;
    bool terminated;
    bool sigsegv_printed;
} ProcessInfo;

int find_victim_lru();
void run_simulation_logic(ReplacementAlgo algo, int num_procs, const int mem_sizes[], const int exec_trace[], int trace_len);
void print_header(int num_procs);

#endif