#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "queue.h"
#include <stdbool.h>

// --- System Constants ---
#define MAX_TIME 100
#define MAX_PROCESSES 20
#define MAX_PROGRAMS 20
#define MAX_PROG_LEN 20
#define QUANTUM 3
#define PAGE_SIZE 3000
#define MEMORY_KB 21
#define NUM_FRAMES (MEMORY_KB * 1000 / PAGE_SIZE) // Should be 7

// --- Process States ---
typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_EXIT
} ProcessState;

// --- Memory Frame Structure ---
typedef struct {
    int frame_id;
    int process_id;
    int page_number;
    int load_time;
    int last_access_time;
} Frame;

// --- Process Control Block (PCB) ---
typedef struct {
    int pid;
    int program_id;
    ProcessState state;
    int time_in_state;
    int pc; // Program Counter

    // Scheduler related
    int remaining_quantum;
    int blocked_until_time;

    // Memory related
    int memory_size;

    // Program instructions
    int* instructions;
    int instruction_count;
    bool has_halt;

    // Error handling
    bool has_error;
    char error_code[10];

} PCB;

// --- Main System Structure ---
typedef struct {
    // Core components
    int current_time;
    int next_pid;
    PCB* processes[MAX_PROCESSES];
    PCB* running_process;

    // State queues
    Queue* new_queue;
    Queue* ready_queue;
    Queue* blocked_queue;
    Queue* exit_queue;

    // Memory components
    Frame physical_memory[NUM_FRAMES];

    // Program data loaded from input
    int programs[MAX_PROGRAMS][MAX_PROG_LEN];
    int program_mem_sizes[MAX_PROGRAMS];
    int program_lengths[MAX_PROGRAMS];
    bool program_has_halt[MAX_PROGRAMS];

} SimulationSystem;

// --- Function Prototypes ---

// Initialization and Cleanup
void initialize_system(SimulationSystem* system, int (*input_programs)[20], int num_rows);
void cleanup_system(SimulationSystem* system);

// Main Simulation Loop
void run_simulation(SimulationSystem* system);

// State Update Functions
void update_new_processes(SimulationSystem* system);
void update_blocked_processes(SimulationSystem* system);
void update_exit_processes(SimulationSystem* system);
void update_scheduler(SimulationSystem* system);

// Instruction Execution
void execute_running_process(SimulationSystem* system);

// Memory Management
bool handle_memory_access(SimulationSystem* system, PCB* proc, int address);
void free_process_memory(SimulationSystem* system, int pid);

// Output
void print_header();
void print_system_state(SimulationSystem* system);

#endif // SIMULATOR_H