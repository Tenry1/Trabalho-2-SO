#ifndef SIMULATION_H
#define SIMULATION_H

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 
#include <stdbool.h> 
#include "queue.h"

// --- Configuration from Part 2 ---
#define PAGE_SIZE 3000
#define NUM_FRAMES 7  // 21KB total memory / 3KB per frame
#define MAX_PROCESSES 20
#define MAX_PROGRAM_INSTRUCTIONS 100 // A reasonable limit for instructions per program

// --- Memory Management Structures ---
typedef struct {
    int frame_id;
    int process_id;
    int page_number;
    int load_time;
    int last_access_time;
} Frame;

// --- Process and System Structures ---
typedef enum {
    NEW, READY, RUNNING, BLOCKED, EXIT
} ProcessState;

typedef struct {
    int pid;
    int program_id;
    ProcessState state;
    const char* error_message; // For SIGSEGV, SIGILL, etc.

    int pc; // Program Counter
    int time_in_state;
    int remaining_quantum;
    int blocked_until;

    // Memory and instruction info
    int memory_size;
    int* instructions;
    int instruction_count;

} PCB;

typedef struct {
    // Queues
    Queue *new_queue;
    Queue *ready_queue;
    Queue *blocked_queue;
    Queue *exit_queue;

    // CPU and System State
    PCB *running_process;
    int current_time;
    int next_pid;

    // Process and Program Storage
    PCB *processes[MAX_PROCESSES];
    int programs[5][MAX_PROGRAM_INSTRUCTIONS];
    int program_lengths[5];
    int program_mem_sizes[5];
    bool pre_new_printed[MAX_PROCESSES];
    bool will_be_created;


    // Physical Memory
    Frame physical_memory[NUM_FRAMES];

} SimulationSystem;

// Input Structure (assuming it's defined elsewhere or passed directly)
typedef struct {
    int (*programs)[20];
    int rows;
} SimulationInput;


// Function Prototypes
void initialize_system_with_input(SimulationSystem *system, SimulationInput input);
void run_simulation(SimulationSystem *system);
PCB *create_new_process(SimulationSystem *system, int prog_id);
void print_current_state(SimulationSystem *system);
// ... other internal functions ...

#endif // SIMULATION_H