#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "p2_simulator.h"

// --- FORWARD DECLARATIONS ---
static PCB* create_new_process(SimulationSystem* system, int prog_id);
static void set_process_error(PCB* proc, const char* error_code);
static void schedule_next_process(SimulationSystem* system);
static int find_free_frame(SimulationSystem* system);
static int find_victim_lru(SimulationSystem* system);
void execute_running_process(SimulationSystem* system);

// =================================================================
//                    INITIALIZATION & CLEANUP
// =================================================================

// Initializes the system
void initialize_system(SimulationSystem* system, int (*input_programs)[20], int num_rows) {
    memset(system, 0, sizeof(SimulationSystem));
    system->current_time = 0;
    system->next_pid = 1;

    // Initializes all queues
    system->new_queue = createQueue();
    system->ready_queue = createQueue();
    system->blocked_queue = createQueue();
    system->exit_queue = createQueue();

    // Initializes all frames 
    for (int i = 0; i < NUM_FRAMES; i++) {
        system->physical_memory[i].frame_id = i;
        system->physical_memory[i].process_id = INACTIVE_PROCESS;
        system->physical_memory[i].last_access_time = system->current_time;

    }

    // Initializes all programs
    for (int i = 0; i < MAX_PROGRAMS; i++) {
        system->program_mem_sizes[i] = input_programs[0][i];
        system->program_lengths[i] = 0;
        system->program_has_halt[i] = false;

        // Cycles all instructions
        for (int j = 1; j < num_rows; j++) {
            // Stores the current instruction
            int instruction = input_programs[j][i];
            system->programs[i][j - 1] = instruction;

            // Checks if the current instruction is an halt, if so sets the program length
            if (instruction == 0 && !system->program_has_halt[i]) {
                system->program_lengths[i] = j - 1;
                system->program_has_halt[i] = true;

            }
        }

        // If there is not any halt instruction, sets the maximum program length
        if (!system->program_has_halt[i]) {
            system->program_lengths[i] = num_rows - 1;

        }
    }

    // Creates the initial process
    create_new_process(system, 1);

}


// Cleans up all system memory
void cleanup_system(SimulationSystem* system) {
    // Frees all processes memory
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (system->processes[i]) {
            free(system->processes[i]->instructions);
            free(system->processes[i]);
        }
    }

    // Deletes all queues
    deleteQueue(system->new_queue);
    deleteQueue(system->ready_queue);
    deleteQueue(system->blocked_queue);
    deleteQueue(system->exit_queue);
}

// =================================================================
//                      MAIN SIMULATION LOOP (Based on Assignment 1)
// =================================================================

// Responsible for running all the system simulation
void run_simulation(SimulationSystem* system) {
    print_header();

    for (system->current_time = 1; system->current_time <= MAX_TIME; system->current_time++) {
        
        // --- FIRST: Perform all calculations for the current time tick ---
        update_new_processes(system);
        update_blocked_processes(system);
        update_exit_processes(system);

        if (system->running_process) {
            execute_running_process(system);
        }
        
        if (!system->running_process) {
            schedule_next_process(system);
        }
        

        // --- LAST: Print the final state AFTER all work is done ---
        print_system_state(system);

        // Check for simulation termination after printing
        bool system_is_empty = !system->running_process && isEmpty(system->new_queue) &&
                               isEmpty(system->ready_queue) && isEmpty(system->blocked_queue) &&
                               isEmpty(system->exit_queue);
                               
        if (system_is_empty) {
            bool any_left = false;

            for (int i = 0; i < MAX_PROCESSES; i++) 
                if (system->processes[i]) 
                    any_left = true;

            if (!any_left) break;
        }
    }
}

// =================================================================
//                 STATE UPDATE & SCHEDULING
// =================================================================

// 
void update_new_processes(SimulationSystem* system) {
    size_t initial_size = queueSize(system->new_queue);

    for (size_t i = 0; i < initial_size; i++) {
        PCB* proc = (PCB*)dequeue(system->new_queue);
        proc->time_in_state++;

        // Change back to >= 2 to ensure 2 time units in NEW state
        if (proc->time_in_state >= 2) {
            proc->state = STATE_READY;
            proc->time_in_state = 0;

            enqueue(system->ready_queue, proc);

        } else {
            enqueue(system->new_queue, proc);

        }
    }
}

void update_blocked_processes(SimulationSystem* system) {
    size_t initial_size = queueSize(system->blocked_queue);

    for (size_t i = 0; i < initial_size; i++) {
        PCB* proc = (PCB*)dequeue(system->blocked_queue);
        proc->time_in_state++;

        if (proc->time_in_state >= abs(proc->instructions[proc->pc])) {
            proc->state = STATE_READY;
            proc->time_in_state = 0;
            proc->pc++;

            enqueue(system->ready_queue, proc);

        } else {
            enqueue(system->blocked_queue, proc);
            
        }
    }
}

void update_exit_processes(SimulationSystem* system) {
    size_t initial_size = queueSize(system->exit_queue);

    for (size_t i = 0; i < initial_size; i++) {
        PCB* proc = (PCB*)dequeue(system->exit_queue);
        proc->time_in_state++;

        if (proc->time_in_state >= 3) {
            free_process_memory(system, proc->pid);
            system->processes[proc->pid - 1] = NULL;

            free(proc->instructions);
            free(proc);

        } else {
            enqueue(system->exit_queue, proc);

        }
    }
}

static void schedule_next_process(SimulationSystem* system) {
    if (system->running_process) return;

    if (!isEmpty(system->ready_queue)) {
        PCB* proc_to_run = (PCB*)dequeue(system->ready_queue);
        proc_to_run->state = STATE_RUNNING;
        proc_to_run->time_in_state = 0;
        proc_to_run->remaining_quantum = QUANTUM;
        
        system->running_process = proc_to_run;

    }
}

// =================================================================
//                       INSTRUCTION EXECUTION
// =================================================================

void execute_running_process(SimulationSystem* system) {
    PCB* proc = system->running_process;

    proc->time_in_state++;

    // Check for errors *before* fetching instruction
    if (proc->pc < 0) set_process_error(proc, "SIGILL");
    else if (proc->pc >= proc->instruction_count) set_process_error(proc, proc->has_halt ? "SIGILL" : "SIGEOF");
    
    if (proc->has_error) {
        proc->state = STATE_EXIT;
        proc->time_in_state = 0;
        enqueue(system->exit_queue, proc);
        system->running_process = NULL;
        return;
    }

    int instruction = proc->instructions[proc->pc];
    bool pc_managed_by_instruction = false;

    // --- Execute Instruction ---
    if (instruction == 0) { // HALT
        proc->state = STATE_EXIT;

    } else if (instruction < 0) { // I/O
        proc->state = STATE_BLOCKED;

    } else if (instruction >= 1 && instruction <= 100) { // JUMPF
        proc->pc += instruction;
        pc_managed_by_instruction = true;

    } else if (instruction >= 101 && instruction <= 199) { // JUMPB
        proc->pc -= (instruction % 100);
        pc_managed_by_instruction = true;

    } else if (instruction >= 201 && instruction <= 299) { // EXEC
        create_new_process(system, instruction % 100);

    } else if (instruction >= 1000 && instruction <= 15999) { // LOAD/STORE
        if (!handle_memory_access(system, proc, instruction - 1000)) {
            set_process_error(proc, "SIGSEGV");
            proc->state = STATE_EXIT;

        }
    }
    
    // Increment PC if it wasn't a jump
    if (!pc_managed_by_instruction && proc->state != STATE_BLOCKED) {
        proc->pc++;
    }

    // --- Handle post-execution state changes ---
    if (proc->state == STATE_EXIT) {
        proc->time_in_state = 0;
        enqueue(system->exit_queue, proc);
        system->running_process = NULL;
    } else if (proc->state == STATE_BLOCKED) {
        proc->time_in_state = 0;
        enqueue(system->blocked_queue, proc);
        system->running_process = NULL;
    } else if (proc->time_in_state >= proc->remaining_quantum) { // Quantum expired
        proc->state = STATE_READY;
        proc->time_in_state = 0;
        enqueue(system->ready_queue, proc);
        system->running_process = NULL;
    }
}

// =================================================================
//                      MEMORY MANAGEMENT
// =================================================================

// Verify if the address used by the process is valid in memory
bool handle_memory_access(SimulationSystem* system, PCB* proc, int address) {
    // Checks if the address is not in a valid range
    if (address < 0 || address >= proc->memory_size) return false;

    int page_num = address / PAGE_SIZE;

    // Cycles all pages and finds the correspondent pid and page_num for the given process
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (system->physical_memory[i].process_id == proc->pid && system->physical_memory[i].page_number == page_num) {
            system->physical_memory[i].last_access_time = system->current_time;

            return true;

        }
    }

    int frame_idx = find_free_frame(system);

    // Updates the frame variables
    system->physical_memory[frame_idx].process_id = proc->pid;
    system->physical_memory[frame_idx].page_number = page_num;
    system->physical_memory[frame_idx].last_access_time = system->current_time;

    return true;

}


// Finds a free frame, if none are free gets a victim frame
static int find_free_frame(SimulationSystem* system) {
    // Cycles all frames, searching for a free frame, with no active process
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (system->physical_memory[i].process_id == INACTIVE_PROCESS) {
            return i;

        }
    }

    // If no frame is found, gets a victim frame
    return find_victim_lru(system);

}


// Finds a substitute frame, with LRU scheduling
static int find_victim_lru(SimulationSystem* system) {
    int victim_frame = DEFAULT_FRAME;
    int min_access_time = INT_MAX;

    // Cycles all frames
    for (int i = 0; i < NUM_FRAMES; i++) {
        // Gets the frame with the minimum access time
        if (system->physical_memory[i].last_access_time < min_access_time) {
            min_access_time = system->physical_memory[i].last_access_time;
            victim_frame = i;
        
        }
        
        // If there are 2 frames with the same time, gets the smallest frame
        if (system->physical_memory[i].last_access_time == min_access_time) {
            if (victim_frame == DEFAULT_FRAME || i < victim_frame) {
                victim_frame = i;
            
            }
        }
    }

    return victim_frame;

}


// Frees a frame with a specific pid
void free_process_memory(SimulationSystem* system, int pid) {
    // Cycles all frames
    for (int i = 0; i < NUM_FRAMES; i++) {
        // Finds the pid in the current frame
        if (system->physical_memory[i].process_id == pid) {
            system->physical_memory[i].process_id = INACTIVE_PROCESS;

        }
    }
}

// =================================================================
//                      PROCESS & ERROR HELPERS
// =================================================================

static PCB* create_new_process(SimulationSystem* system, int prog_id) {
    int prog_idx = prog_id - 1;
    if (system->next_pid > MAX_PROCESSES || prog_idx < 0 || prog_idx >= MAX_PROGRAMS) return NULL;
    
    PCB* proc = (PCB*)calloc(1, sizeof(PCB));

    proc->pid = system->next_pid++;
    proc->program_id = prog_id;
    proc->state = STATE_NEW;
    proc->pc = 0;

    if (proc->pid == 1) {
        proc->time_in_state = -1;
    } else {
        proc->time_in_state = 0;
    }

    proc->memory_size = system->program_mem_sizes[prog_idx];
    proc->instruction_count = system->program_lengths[prog_idx];
    proc->has_halt = system->program_has_halt[prog_idx];

    proc->instructions = (int*)malloc(proc->instruction_count * sizeof(int));
    memcpy(proc->instructions, system->programs[prog_idx], proc->instruction_count * sizeof(int));
    
    system->processes[proc->pid - 1] = proc;
    enqueue(system->new_queue, proc);
return proc;
}

static void set_process_error(PCB* proc, const char* error_code) {
    if (!proc->has_error) {
        proc->has_error = true;
        strncpy(proc->error_code, error_code, sizeof(proc->error_code) - 1);
    }
}

// =================================================================
//                            OUTPUT
// =================================================================

void print_header() {
    printf("%-5s", "time");
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        char header[20];
        sprintf(header, "proc%d", i);
        printf("%-26s", header);
    }
    printf("\n");
}

void print_system_state(SimulationSystem* system) {
    printf("%-5d", system->current_time);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB* proc = system->processes[i];
        if (!proc) {
            printf("%-26s", "");
            continue;
        }

        char state_str[10] = "";
        char frame_str[100] = "[]";

        // Logic to determine the string for the state
        if (proc->has_error && proc->state != STATE_READY && proc->state != STATE_NEW) {
            if (proc->state == STATE_EXIT && proc->time_in_state > 0) {
                 strcpy(state_str, "EXIT");
            } else {
                 strcpy(state_str, proc->error_code);
            }
        } else {
            switch (proc->state) {
                case STATE_NEW:     strcpy(state_str, "NEW"); break;
                case STATE_READY:   strcpy(state_str, "READY"); break;
                case STATE_RUNNING: strcpy(state_str, "RUN"); break;
                case STATE_BLOCKED: strcpy(state_str, "BLOCKED"); break;
                case STATE_EXIT:    strcpy(state_str, "EXIT"); break;
            }
        }
        
        // Build frame string
        int proc_frames[NUM_FRAMES], proc_pages[NUM_FRAMES], frame_count = 0;
        for (int f = 0; f < NUM_FRAMES; f++) {
            if (system->physical_memory[f].process_id == proc->pid) {
                proc_frames[frame_count] = system->physical_memory[f].frame_id;
                proc_pages[frame_count] = system->physical_memory[f].page_number;
                frame_count++;
            }
        }
        
        if (frame_count > 0) {
            for(int k=0; k<frame_count-1; k++) {
                for(int m=0; m<frame_count-k-1; m++) {
                    if(proc_pages[m] > proc_pages[m+1]) {
                        int temp_p = proc_pages[m]; proc_pages[m] = proc_pages[m+1]; proc_pages[m+1] = temp_p;
                        int temp_f = proc_frames[m]; proc_frames[m] = proc_frames[m+1]; proc_frames[m+1] = temp_f;
                    }
                }
            }
            char temp_frame_str[100] = "";
            char temp_buf[10];
            for (int k = 0; k < frame_count; k++) {
                sprintf(temp_buf, "F%d%s", proc_frames[k], (k < frame_count - 1) ? "," : "");
                strcat(temp_frame_str, temp_buf);
            }
            sprintf(frame_str, "[%s]", temp_frame_str);
        }
        
        char final_output[150];
        sprintf(final_output, "%s %s", state_str, frame_str);
        printf("%-26s", final_output);
    }
    printf("\n");
}