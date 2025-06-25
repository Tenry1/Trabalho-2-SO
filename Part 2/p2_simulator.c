#include "p2_simulator.h"

void initialize_memory(MemoryManager* mm) {
    mm->total_frames = MAX_FRAMES;
    mm->free_frames = MAX_FRAMES;
    mm->clock = 0;

    for (int i = 0; i < MAX_FRAMES; i++) {
        mm->frames[i].frame_id = i;
        mm->frames[i].free = true;
        mm->frames[i].pid = -1;
        mm->frames[i].page_number = -1;
        mm->frames[i].last_used = -1;
    }
}

void initialize_scheduler(Scheduler* sched) {
    sched->process_count = 0;
    sched->current_process = NULL;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        sched->processes[i] = NULL;
    }
}

PCB* create_process(int pid, int address_space, int* program, int program_size) {
    PCB* process = (PCB*)malloc(sizeof(PCB));
    process->pid = pid;
    process->state = NEW;
    process->pc = 0;
    process->address_space = address_space;
    process->time_left = 0;
    process->last_used_time = 0;

    // Copy program
    process->program = (int*)malloc(program_size * sizeof(int));
    memcpy(process->program, program, program_size * sizeof(int));
    process->program_size = program_size;

    // Initialize page table
    int page_count = (address_space + PAGE_SIZE - 1) / PAGE_SIZE;
    process->page_table.entries = (PageTableEntry*)malloc(page_count * sizeof(PageTableEntry));
    process->page_table.page_count = page_count;

    for (int i = 0; i < page_count; i++) {
        process->page_table.entries[i].frame_number = -1;
        process->page_table.entries[i].valid = false;
        process->page_table.entries[i].dirty = false;
    }

    // Initialize page access times
    process->page_access_times = (int*)malloc(page_count * sizeof(int));
    for (int i = 0; i < page_count; i++) {
        process->page_access_times[i] = -1;
    }

    return process;
}

void free_process(PCB* process) {
    free(process->program);
    free(process->page_table.entries);
    free(process->page_access_times);
    free(process);
}

void execute_instruction(PCB* process, MemoryManager* mm, Scheduler* sched) {
    int instruction = process->program[process->pc];

    if (instruction == 0) { // HALT
        process->state = EXIT;
        process->time_left = 3;
    }
    else if (instruction >= 1 && instruction <= 100) { // JUMPF
        int jump = instruction % 100;
        if (process->pc + jump >= process->program_size) {
            process->state = SIGILL;
            process->time_left = 3;
            return;
        }
        process->pc += jump;
    }
    else if (instruction >= 101 && instruction <= 199) { // JUMPB
        int jump = instruction % 100;
        if (process->pc - jump < 0) {
            process->state = SIGILL;
            process->time_left = 3;
            return;
        }
        process->pc -= jump;
    }
    else if (instruction >= 1000 && instruction <= 15999) { // LOAD/STORE
        int address = instruction - 1000;
        if (address >= process->address_space) {
            process->state = SIGSEGV;
            process->time_left = 3;
            return;
        }
        handle_memory_access(process, mm, address);
    }
    else if (instruction >= 1000000000 && instruction <= 2109999999) { // SWAP/MEMCPY
        // Extract addresses
        int addr1 = (instruction / 100000) % 100000 - 10000;
        int addr2 = instruction % 100000;

        if (addr1 >= process->address_space || addr2 >= process->address_space) {
            process->state = SIGSEGV;
            process->time_left = 3;
            return;
        }

        // Handle both addresses
        handle_memory_access(process, mm, addr1);
        if (process->state == SIGSEGV) return;

        handle_memory_access(process, mm, addr2);
    }
    else { // Other instructions from projeto_SO
        // For simplicity, we'll just advance PC
        process->pc++;
    }

    // Check for program end
    if (process->pc >= process->program_size && process->state != EXIT &&
        process->state != SIGSEGV && process->state != SIGILL) {
        process->state = SIGEOF;
        process->time_left = 3;
    }
}

void handle_memory_access(PCB* process, MemoryManager* mm, int address) {
    int page_number = address / PAGE_SIZE;

    // Check if page is loaded
    if (!process->page_table.entries[page_number].valid) {
        // Page fault - need to load page
        int frame_number = -1;

        // First try to find a free frame
        for (int i = 0; i < mm->total_frames; i++) {
            if (mm->frames[i].free) {
                frame_number = i;
                break;
            }
        }

        // If no free frames, select victim
        if (frame_number == -1) {
            frame_number = select_victim_frame_LRU(mm);

            // Free the victim frame
            PCB* victim_proc = NULL;
            for (int i = 0; i < MAX_PROCESSES; i++) {
                if (sched.processes[i] && sched.processes[i]->pid == mm->frames[frame_number].pid) {
                    victim_proc = sched.processes[i];
                    break;
                }
            }

            if (victim_proc) {
                victim_proc->page_table.entries[mm->frames[frame_number].page_number].valid = false;
            }
        }

        // Load the new page
        mm->frames[frame_number].free = false;
        mm->frames[frame_number].pid = process->pid;
        mm->frames[frame_number].page_number = page_number;
        mm->frames[frame_number].last_used = mm->clock;

        process->page_table.entries[page_number].frame_number = frame_number;
        process->page_table.entries[page_number].valid = true;
        mm->free_frames--;
    }

    // Update access time
    process->page_access_times[page_number] = mm->clock;
    mm->frames[process->page_table.entries[page_number].frame_number].last_used = mm->clock;
    mm->clock++;
}

int select_victim_frame_LRU(MemoryManager* mm) {
    int lru_time = INT_MAX;
    int victim_frame = -1;

    for (int i = 0; i < mm->total_frames; i++) {
        if (mm->frames[i].last_used < lru_time ||
            (mm->frames[i].last_used == lru_time && i < victim_frame)) {
            lru_time = mm->frames[i].last_used;
            victim_frame = i;
        }
    }

    return victim_frame;
}

void update_process_states(Scheduler* sched, MemoryManager* mm, int time) {
    // Update process states
    for (int i = 0; i < sched->process_count; i++) {
        PCB* proc = sched->processes[i];

        if (proc->state == NEW) {
            proc->state = READY;
        }
        else if (proc->state == EXIT || proc->state == SIGSEGV ||
                 proc->state == SIGILL || proc->state == SIGEOF) {
            if (proc->time_left > 0) {
                proc->time_left--;
            } else {
                // Free frames used by this process
                for (int j = 0; j < mm->total_frames; j++) {
                    if (mm->frames[j].pid == proc->pid) {
                        mm->frames[j].free = true;
                        mm->frames[j].pid = -1;
                        mm->frames[j].page_number = -1;
                        mm->free_frames++;
                    }
                }
                // Remove process from scheduler
                free_process(proc);
                sched->processes[i] = NULL;
            }
        }
        else if (proc->state == BLOCKED) {
            // Simplified: unblock after 1 time unit
            proc->state = READY;
        }
    }

    // Compact process array
    int new_count = 0;
    for (int i = 0; i < sched->process_count; i++) {
        if (sched->processes[i] != NULL) {
            sched->processes[new_count++] = sched->processes[i];
        }
    }
    sched->process_count = new_count;
}

void schedule_next_process(Scheduler* sched) {
    // Simplified round-robin scheduling
    if (sched->current_process != NULL && sched->current_process->state == RUN) {
        sched->current_process->state = READY;
    }

    // Find next READY process
    for (int i = 0; i < sched->process_count; i++) {
        if (sched->processes[i]->state == READY) {
            sched->current_process = sched->processes[i];
            sched->current_process->state = RUN;
            break;
        }
    }
}

void print_state(int time, Scheduler* sched, MemoryManager* mm) {
    printf("%-4d", time);

    // Print process states
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (i < sched->process_count) {
            PCB* proc = sched->processes[i];
            char state[20];

            switch (proc->state) {
                case NEW: strcpy(state, "NEW"); break;
                case READY: strcpy(state, "READY"); break;
                case RUN: strcpy(state, "RUN"); break;
                case BLOCKED: strcpy(state, "BLOCKED"); break;
                case EXIT: strcpy(state, "EXIT"); break;
                case SIGSEGV: strcpy(state, "SIGSEGV"); break;
                case SIGILL: strcpy(state, "SIGILL"); break;
                case SIGEOF: strcpy(state, "SIGEOF"); break;
            }

            // Print process state
            printf("%-10s", state);

            // Print frames if process is RUN or BLOCKED
            if (proc->state == RUN || proc->state == BLOCKED) {
                printf("[");
                bool first = true;

                // Collect frames for this process
                int frames[MAX_FRAMES];
                int frame_count = 0;

                for (int j = 0; j < mm->total_frames; j++) {
                    if (mm->frames[j].pid == proc->pid) {
                        frames[frame_count++] = mm->frames[j].frame_id;
                    }
                }

                // Sort frames for consistent output
                for (int j = 0; j < frame_count; j++) {
                    for (int k = j+1; k < frame_count; k++) {
                        if (frames[j] > frames[k]) {
                            int temp = frames[j];
                            frames[j] = frames[k];
                            frames[k] = temp;
                        }
                    }
                }

                // Print sorted frames
                for (int j = 0; j < frame_count; j++) {
                    if (!first) printf(",");
                    printf("F%d", frames[j]);
                    first = false;
                }

                printf("]");
            } else {
                printf("          "); // Padding for alignment
            }
        } else {
            printf("                    "); // Padding for empty slots
        }
    }

    printf("\n");
}

bool should_continue(Scheduler* sched) {
    if (sched->process_count == 0) return false;

    for (int i = 0; i < sched->process_count; i++) {
        if (sched->processes[i]->state != EXIT &&
            sched->processes[i]->state != SIGSEGV &&
            sched->processes[i]->state != SIGILL &&
            sched->processes[i]->state != SIGEOF) {
            return true;
        }
    }

    return false;
}

void simulate(Scheduler* sched, MemoryManager* mm) {
    int time = 1;
    bool running = true;

    // Print header
    printf("time inst  proc1                     proc2                     proc3                     proc4                     proc5                     proc6                     proc7                     proc8                     proc9                     proc10                    proc11                    proc12                    proc13                    proc14                    proc15                    proc16                    proc17                    proc18                    proc19                    proc20                  \n");

    while (running && time <= 100) {
        // Update process states
        update_process_states(sched, mm, time);

        // Schedule next process
        schedule_next_process(sched);

        // Execute current process if in RUN state
        if (sched->current_process && sched->current_process->state == RUN) {
            execute_instruction(sched->current_process, mm, sched);
        }

        // Print current state
        print_state(time, sched, mm);

        // Check if simulation should end
        running = should_continue(sched);

        time++;
    }
}