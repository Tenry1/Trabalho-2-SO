#ifndef P2_SIMULATOR_H
#define P2_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_PROCESSES 20
#define MAX_FRAMES 7  // 21KB total / 3KB per frame
#define PAGE_SIZE 3072  // 3KB
#define MAX_PROGRAM_SIZE 100
#define MAX_MEMORY 21504  // 21KB in bytes

// Process states
typedef enum {
    NEW, READY, RUN, BLOCKED, EXIT, SIGSEGV, SIGILL, SIGEOF
} ProcessState;

// Frame structure
typedef struct {
    int frame_id;
    bool free;
    int pid;
    int page_number;
    int last_used;
} Frame;

// Page table entry
typedef struct {
    int frame_number;
    bool valid;
    bool dirty;
} PageTableEntry;

// Page table
typedef struct {
    PageTableEntry* entries;
    int page_count;
} PageTable;

// Process Control Block
typedef struct {
    int pid;
    ProcessState state;
    int pc;
    int program_size;
    int* program;
    int address_space;
    int time_left;

    PageTable page_table;
    int* page_access_times;
    int last_used_time;
} PCB;

// Memory Manager
typedef struct {
    Frame frames[MAX_FRAMES];
    int free_frames;
    int total_frames;
    int clock;
} MemoryManager;

// Scheduler
typedef struct {
    PCB* processes[MAX_PROCESSES];
    int process_count;
    PCB* current_process;
} Scheduler;

// Function prototypes
void initialize_memory(MemoryManager* mm);
void initialize_scheduler(Scheduler* sched);
PCB* create_process(int pid, int address_space, int* program, int program_size);
void free_process(PCB* process);
void execute_instruction(PCB* process, MemoryManager* mm, Scheduler* sched);
void handle_memory_access(PCB* process, MemoryManager* mm, int address);
int select_victim_frame_LRU(MemoryManager* mm);
void update_process_states(Scheduler* sched, MemoryManager* mm, int time);
void schedule_next_process(Scheduler* sched);
void print_state(int time, Scheduler* sched, MemoryManager* mm);
bool should_continue(Scheduler* sched);
void simulate(Scheduler* sched, MemoryManager* mm);


#endif //P2_SIMULATOR_H
