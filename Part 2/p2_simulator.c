#include "p2_simulator.h"

// --- Memory Management Helpers ---

void initialize_memory(SimulationSystem* system) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        system->physical_memory[i].frame_id = i;
        system->physical_memory[i].process_id = -1;
        system->physical_memory[i].page_number = -1;
        system->physical_memory[i].load_time = -1;
        system->physical_memory[i].last_access_time = -1;
    }
}

int find_page_in_memory(SimulationSystem* system, int pid, int page_num) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (system->physical_memory[i].process_id == pid && system->physical_memory[i].page_number == page_num) {
            return i; // Return frame index
        }
    }
    return -1; // Page not in memory
}

int find_free_frame(SimulationSystem* system) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (system->physical_memory[i].process_id == -1) {
            return i; // Return frame index
        }
    }
    return -1; // No free frames
}

// LRU with Frame ID as tie-breaker
int find_victim_lru(SimulationSystem* system) {
    int victim_frame_idx = -1;
    int min_access_time = INT_MAX;
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (system->physical_memory[i].last_access_time < min_access_time) {
            min_access_time = system->physical_memory[i].last_access_time;
            victim_frame_idx = i;
        } else if (system->physical_memory[i].last_access_time == min_access_time) {
            // Choose the frame with the lower frame ID
            if (victim_frame_idx == -1 || system->physical_memory[i].frame_id < system->physical_memory[victim_frame_idx].frame_id) {
                victim_frame_idx = i;
            }
        }
    }
    return victim_frame_idx;
}

void load_page_into_frame(SimulationSystem* system, int frame_idx, int pid, int page_num, int time) {
    system->physical_memory[frame_idx].process_id = pid;
    system->physical_memory[frame_idx].page_number = page_num;
    system->physical_memory[frame_idx].load_time = time;
    system->physical_memory[frame_idx].last_access_time = time;
}

// Handle memory access and SIGSEGV
int handle_memory_access(SimulationSystem* system, PCB* proc, int address) {
    // Check if address is within the process's allocated memory space
    if (address < 0 || address >= proc->memory_size) {
        return 0; // Invalid access, triggers SIGSEGV
    }

    int page_needed = address / PAGE_SIZE;
    int frame_idx = find_page_in_memory(system, proc->pid, page_needed);

    if (frame_idx != -1) {
        // Page hit, update last access time for LRU
        system->physical_memory[frame_idx].last_access_time = system->current_time;
    } else {
        // Page fault
        int free_frame_idx = find_free_frame(system);
        if (free_frame_idx != -1) {
            // Load into a free frame
            load_page_into_frame(system, free_frame_idx, proc->pid, page_needed, system->current_time);
        } else {
            // No free frames, find a victim using LRU
            int victim_idx = find_victim_lru(system);
            load_page_into_frame(system, victim_idx, proc->pid, page_needed, system->current_time);
        }
    }
    return 1;
}

// Terminating a process moves it to the EXIT state
void terminate_process(SimulationSystem* system, PCB* proc, const char* reason) {
    proc->state = EXIT;
    proc->error_message = reason;
    proc->time_in_state = 0; // Reset timer for the EXIT state
    enqueue(system->exit_queue, proc);
    if (system->running_process == proc) {
        system->running_process = NULL;
    }
}

// Read memory size from first line of input
void initialize_system_with_input(SimulationSystem *system, SimulationInput input) {
    memset(system, 0, sizeof(SimulationSystem));

    system->ready_queue = createQueue();
    system->new_queue = createQueue();
    system->blocked_queue = createQueue();
    system->exit_queue = createQueue();
    
    system->running_process = NULL;
    system->next_pid = 1;
    system->current_time = 0;

    initialize_memory(system);

    for (int i = 0; i < MAX_PROCESSES; ++i) {
        system->processes[i] = NULL;

    }

    for (int prog_id = 0; prog_id < 5; prog_id++) {
        // First row contains the process memory size in bytes
        system->program_mem_sizes[prog_id] = input.programs[0][prog_id];
        system->program_lengths[prog_id] = 0;

        // Subsequent rows contain instructions
        for (int step = 1; step < input.rows; step++) {
            int instruction = input.programs[step][prog_id];
            if(system->program_lengths[prog_id] < MAX_PROGRAM_INSTRUCTIONS) {
                 system->programs[prog_id][system->program_lengths[prog_id]++] = instruction;

            }
            if (instruction == 0) break; // Halt instruction marks end of program
        }
    }
    PCB *first_process = create_new_process(system, 0);
    enqueue(system->new_queue, first_process);
}

PCB *create_new_process(SimulationSystem *system, int prog_id) {
    if (prog_id < 0 || prog_id >= 5 || system->next_pid > MAX_PROCESSES) return NULL;
    PCB *new_process = (PCB *)calloc(1, sizeof(PCB));
    if (!new_process) return NULL;

    new_process->pid = system->next_pid++;
    new_process->program_id = prog_id;
    new_process->state = NEW;
    new_process->pc = 0;
    new_process->error_message = NULL;
    new_process->memory_size = system->program_mem_sizes[prog_id];
    int length = system->program_lengths[prog_id];
    new_process->instruction_count = length;
    new_process->instructions = (int *)malloc(length * sizeof(int));
    if (!new_process->instructions) {
        free(new_process);
        return NULL;
    }
    memcpy(new_process->instructions, system->programs[prog_id], length * sizeof(int));
    system->processes[new_process->pid - 1] = new_process;
    return new_process;
}

void update_blocked_processes(SimulationSystem *system) {
    size_t size = queueSize(system->blocked_queue);
    if (size == 0) return;
    PCB *to_ready[size];
    int ready_count = 0;
    for (size_t i = 0; i < size; i++) {
        PCB *proc = (PCB *)getQueueNodeAt(system->blocked_queue, i);
        if (proc->blocked_until <= system->current_time) {
            to_ready[ready_count++] = proc;
        }
    }
    for (int i = 0; i < ready_count; i++) {
        PCB *proc = to_ready[i];
        if (removeNodeByData(system->blocked_queue, proc)) {
            proc->state = READY;
            proc->time_in_state = 0;
            proc->pc++; 
            enqueue(system->ready_queue, proc);
        }
    }
}

void update_new_processes(SimulationSystem *system) {
    size_t size = queueSize(system->new_queue);
    if (size == 0) return;

    PCB *to_ready[size];
    int ready_count = 0;

    for (size_t i = 0; i < size; i++) {
        PCB *proc = (PCB *)getQueueNodeAt(system->new_queue, i);
        proc->time_in_state++;

        bool should_move_to_ready = false;

        // First process (PID 1) stays in NEW for 2 instants
        // All subsequent processes stay in NEW for only 1 instant
        if (proc->pid == 1) {
            // PID 1 READY at instant 3
            if (proc->time_in_state >= 3) {
                should_move_to_ready = true;
            }
        } else {
            // Subsequent processes READY at the start of their 2nd instant
            if (proc->time_in_state >= 2) {
                should_move_to_ready = true;
            }
        }

        if (should_move_to_ready) {
            to_ready[ready_count++] = proc;
        }
    }

    // Move procces from NEW queue to READY queue
    for (int i = 0; i < ready_count; i++) {
        PCB *proc = to_ready[i];
        if (removeNodeByData(system->new_queue, proc)) {
            proc->state = READY;
            proc->time_in_state = 0;
            enqueue(system->ready_queue, proc);
        }
    }
}

void update_exit_processes(SimulationSystem *system) {
    size_t size = queueSize(system->exit_queue);
    if (size == 0) return;

    PCB *to_remove[size];
    int remove_count = 0;

    for (size_t i = 0; i < size; i++) {
        PCB *proc = (PCB *)getQueueNodeAt(system->exit_queue, i);
        proc->time_in_state++;

        // Check if a process is in an Error
        if (proc->error_message != NULL) {
            // If the process is in an error state, it should transition to EXIT after 1 time instant.
            if (proc->time_in_state >= 1) {
                proc->error_message = NULL; // It will now print as "EXIT"
                proc->time_in_state = 1;    // First of its 3 ticks in EXIT state
            }
        } else { 
            if (proc->time_in_state >= 4) {
                to_remove[remove_count++] = proc;
            }
        }
    }

    // Remove processes that have completed their exit state.
    for (int i = 0; i < remove_count; i++) {
        PCB *proc = to_remove[i];
        if (removeNodeByData(system->exit_queue, proc)) {
            // Free the process's frames from memory.
            for (int f = 0; f < NUM_FRAMES; f++) {
                if (system->physical_memory[f].process_id == proc->pid) {
                    system->physical_memory[f].process_id = -1; // Mark frame as free
                    system->physical_memory[f].page_number = -1;
                    system->physical_memory[f].load_time = -1;
                    system->physical_memory[f].last_access_time = -1;
                }
            }
            if (proc->instructions) free(proc->instructions);
            system->processes[proc->pid - 1] = NULL;
            free(proc);
        }
    }
}

// Scheduler uses the ready queue (FIFO/Round Robin)
void schedule_next_process(SimulationSystem *system) {
    if (system->running_process) return;
    if (!isEmpty(system->ready_queue)) {
        PCB *next_proc = dequeue(system->ready_queue);
        next_proc->state = RUNNING;
        next_proc->time_in_state = 0;
        next_proc->remaining_quantum = 3;
        system->running_process = next_proc;
    }
}

// Print state and sorted list of frames
void print_current_state(SimulationSystem *system) {
    printf("%-10d", system->current_time);
    for (int pid = 1; pid <= 20; pid++) {
        PCB *proc = system->processes[pid - 1];

        if (!proc && system->pre_new_printed[pid - 1] == false) {
            system->will_be_created = false;
            PCB *creator_proc = system->running_process; // The only process that can create another.

            // Check if there is a running process that is about to execute an EXEC instruction.
            if (creator_proc && creator_proc->pc < creator_proc->instruction_count) {
                int instruction = creator_proc->instructions[creator_proc->pc];
                // Check if the instruction is EXEC and if the PID it will create matches the current PID column.
                if ((instruction >= 201 && instruction <= 299) && (system->next_pid == pid)) {
                    system->will_be_created = true;
                }
            }

            if (system->will_be_created) {
                // Print the special "pre-NEW" state and set the flag so we don't do it again.
                printf("\t%-18s", "NEW");
                system->pre_new_printed[pid - 1] = true;
                continue; // Move to the next pid in the loop.
            }
        }

        char output_str[100] = "";
        if (proc) {
            const char *state_str = "";
            switch (proc->state) {
                case NEW:     state_str = "NEW"; break;
                case READY:   state_str = "READY"; break;
                case RUNNING: state_str = "RUN"; break;
                case BLOCKED: state_str = "BLOCKED"; break;
                case EXIT:    state_str = "EXIT"; break;
            }
            strcpy(output_str, state_str);
            if (proc->error_message) {
                 strcpy(output_str, proc->error_message);
            }
            if (proc->state == READY || proc->state == RUNNING || proc->state == BLOCKED || proc->state == EXIT) {
                Frame proc_frames[NUM_FRAMES];
                int frame_count = 0;
                for (int i = 0; i < NUM_FRAMES; i++) {
                    if (system->physical_memory[i].process_id == proc->pid) {
                        proc_frames[frame_count++] = system->physical_memory[i];
                    }
                }
                // Bubble sort frames by page_number
                for (int i = 0; i < frame_count - 1; i++) {
                    for (int j = 0; j < frame_count - i - 1; j++) {
                        if (proc_frames[j].page_number > proc_frames[j + 1].page_number) {
                            Frame temp = proc_frames[j];
                            proc_frames[j] = proc_frames[j + 1];
                            proc_frames[j + 1] = temp;
                        }
                    }
                }
                char frame_list_str[80] = " [";
                for (int i = 0; i < frame_count; i++) {
                    char temp[10];
                    sprintf(temp, "F%d%s", proc_frames[i].frame_id, (i == frame_count - 1) ? "" : ",");
                    strcat(frame_list_str, temp);
                }
                strcat(frame_list_str, "]");
                strcat(output_str, frame_list_str);
            }
        }
        printf("\t%-18s", output_str);
    }
    printf("\n");
}

void run_simulation(SimulationSystem *system) {
    if (!system) return;

    printf("time      ");
    for (int i = 1; i <= 20; i++) printf("\tproc%-15d", i);
    printf("\n");

    PCB* preempted_process = NULL;

    for (int time = 1; time <= 100; time++) {
        system->current_time = time;

        update_new_processes(system);
        update_blocked_processes(system);

        if (preempted_process) {
            enqueue(system->ready_queue, preempted_process);
            preempted_process = NULL;
        }

        schedule_next_process(system);

        // Check for errors in the running process
        bool error_occurred = false;
        const char* error_reason = NULL;

        if (system->running_process) {
            PCB *proc = system->running_process;

            if (proc->pc >= proc->instruction_count) {
                error_occurred = true;
                error_reason = "SIGEOF";
            } else {
                int instruction = proc->instructions[proc->pc];
                if (instruction >= 1000 && instruction <= 15999) {
                    if (!handle_memory_access(system, proc, instruction - 1000)) {
                        error_occurred = true;
                        error_reason = "SIGSEGV";
                    }
                } else if (instruction >= 1 && instruction <= 100) {
                    if (proc->pc + instruction >= proc->instruction_count) {
                        error_occurred = true;
                        error_reason = "SIGILL";
                    }
                } else if (instruction >= 101 && instruction <= 199) {
                    if (proc->pc - (instruction - 100) < 0) {
                        error_occurred = true;
                        error_reason = "SIGILL";
                    }
                }
            }
            
            if (error_occurred) {
                proc->error_message = error_reason;
            }
        }

        // Print the state
        print_current_state(system);

        // Fully execute the logic and state changes
        PCB* proc = system->running_process;
        if (proc) {
            // Check if an error was detected (or if the message was set)
            if (error_occurred) {
                // We already set the message, just finalize the termination
                terminate_process(system, proc, proc->error_message);
            }
            else { // No error, proceed as normal
                int instruction = proc->instructions[proc->pc];
                if (instruction == 0) { // Halt
                    terminate_process(system, proc, NULL);
                } else if (instruction >= 1000 && instruction <= 15999) { // LOAD/STORE
                    proc->pc++;
                } else if (instruction >= 1 && instruction <= 100) { // JUMPF
                    proc->pc += instruction;
                } else if (instruction >= 101 && instruction <= 199) { // JUMPB
                    proc->pc -= (instruction - 100);
                } else if (instruction >= 201 && instruction <= 299) { // EXEC
                    int program_id = (instruction % 100) - 1;
                    if (system->next_pid <= MAX_PROCESSES && program_id >= 0 && program_id < 5) {
                        PCB *new_proc = create_new_process(system, program_id);
                        if (new_proc) enqueue(system->new_queue, new_proc);
                    }
                    proc->pc++;
                } else if (instruction < 0) { // BLOCK
                    proc->state = BLOCKED;
                    proc->blocked_until = system->current_time + (-instruction) + 1;
                    enqueue(system->blocked_queue, proc);
                    system->running_process = NULL;
                } else { // Unknown instruction, treat as NOP
                    proc->pc++;
                }
            }
        }

        // Handle quantum expiration
        if (system->running_process) {
             system->running_process->remaining_quantum--;
             if (system->running_process->remaining_quantum <= 0) {
                 system->running_process->state = READY;
                 preempted_process = system->running_process;
                 system->running_process = NULL;
             }
        }

        // Cleanup exit processes
        update_exit_processes(system);

        // Check for simulation end
        if (isEmpty(system->new_queue) && isEmpty(system->ready_queue) &&
            isEmpty(system->blocked_queue) && isEmpty(system->exit_queue) &&
            !system->running_process && !preempted_process) {
            break;
        }
    }
}