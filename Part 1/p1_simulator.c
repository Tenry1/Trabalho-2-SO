#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "inputs_part1.h"
#include "p1_simulator.h"

// --- Configuration ---
#define PAGE_SIZE (3 * 1000)
#define NUM_FRAMES 7
#define MAX_PROCESSES 20

// --- Global State ---
Frame physical_memory[NUM_FRAMES]; // Represents the physical memory frames
ProcessInfo processes[MAX_PROCESSES]; // Holds information about each process

// --- Helper Functions ---

// Searches physical memory to see if a specific page for a specific process is already loaded.
int find_page_in_memory(int pid, int page_num) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        // Get the frame we are currently looking at
        Frame current_frame = physical_memory[i];
        
        // Check if this frame belongs to the process we want
        if (current_frame.process_id == pid) {
            // If it does, check if it has the page we want
            if (current_frame.page_number == page_num) {
                return i; // Return the index of the frame
            }
        }
    }

    return -1;
}

// Finds the first available frame in physical memory.
int find_free_frame() {
    for (int i = 0; i < NUM_FRAMES; i++) {
        // A free frame is one with a process ID of -1
        if (physical_memory[i].process_id == -1) {
            return i; // Return the index of the first free one we find
        }
    }
    return -1; // No free frames were found
}

// Implements the FIFO page replacement algorithm to find the page that has been in memory the longest.
int find_victim_fifo() {
    int frame_to_replace = -1;

    int smallest_load_time = INT_MAX;
    
    for (int i = 0; i < NUM_FRAMES; i++) {
        // If the load time of this frame is smaller than the smallest we've seen so far...
        if (physical_memory[i].load_time < smallest_load_time) {
            // ...then this is our new candidate to be replaced.
            smallest_load_time = physical_memory[i].load_time;
            frame_to_replace = i;
        }
    }
    return frame_to_replace;
}

// Implements the LRU page replacement algorithm to find the page that has not been accessed for the longest time.
int find_victim_lru() {
    int frame_to_replace = -1;

    int smallest_access_time = INT_MAX;

    for (int i = 0; i < NUM_FRAMES; i++) {
        // If the last access time of this frame is smaller than the smallest we've seen...
        if (physical_memory[i].last_access_time < smallest_access_time) {
            // ...then this is our new candidate.
            smallest_access_time = physical_memory[i].last_access_time;
            frame_to_replace = i;
        } else if (physical_memory[i].last_access_time == smallest_access_time) {
            // Tie-breaking rule: choose the frame with the smaller ID.
            if (frame_to_replace == -1 || physical_memory[i].frame_id < frame_to_replace) {
                frame_to_replace = i;
            }
        }
    }
    return frame_to_replace;
}

// Updates a frame in physical memory with the new page information.
void load_page_into_frame(int frame_id, int pid, int page_num, int current_time) {
    physical_memory[frame_id].process_id = pid;
    physical_memory[frame_id].page_number = page_num;
    physical_memory[frame_id].load_time = current_time;
    physical_memory[frame_id].last_access_time = current_time;
}

// Initializes the simulation state with the given number of processes and their memory sizes.
void initialize_simulation(int num_procs, const int mem_sizes[]) {
    // Set all frames to be free
    for (int i = 0; i < NUM_FRAMES; i++) {
        physical_memory[i].frame_id = i;
        physical_memory[i].process_id = -1; // -1 means free
    }
    // Set up the processes for this test case
    for (int i = 0; i < num_procs; i++) {
        processes[i].pid = i + 1;
        processes[i].memory_size = mem_sizes[i];
        processes[i].terminated = false;
        processes[i].sigsegv_printed = false;
    }
}

// Prints the header of the output table.
void print_header(int num_procs) {
    printf("%-4s %-4s", "time", "inst");
    
    for (int i = 1; i <= num_procs; i++) {
        char header_text[16];
        // Create the "proc1", "proc2", etc. text
        sprintf(header_text, "proc%d", i);
        // Print it with padding
        printf(" %-18s", header_text);
    }
    printf("\n");
}

// Prints one row of the output table, representing the system state at a specific time.
void print_state(int current_time, int num_procs) {
    printf("%-5d ", current_time);
    printf("%-3s", ""); // The empty "inst" column

    // loop through each process and print its column
    for (int i = 1; i <= num_procs; i++) {
        char string_for_this_column[100];
        strcpy(string_for_this_column, ""); // Make sure the string is empty to start
        
        // CHeck if the proccess has terminated due to segmentation fault
        ProcessInfo current_process_info = processes[i-1];
        if (current_process_info.terminated == true) {
            // Only print SIGSEGV one time
            if (current_process_info.sigsegv_printed == false) {
                strcpy(string_for_this_column, "SIGSEGV");
                processes[i-1].sigsegv_printed = true; // Prevent "SIGSEGV" from being printed more than once
            }
        } else {
            // If the process is not terminated, find its frames
            int frames_this_process_owns[NUM_FRAMES];
            int pages_in_those_frames[NUM_FRAMES];
            int number_of_frames_found = 0;

            for(int j = 0; j < NUM_FRAMES; j++) {
                if (physical_memory[j].process_id == i) {
                    frames_this_process_owns[number_of_frames_found] = physical_memory[j].frame_id;
                    pages_in_those_frames[number_of_frames_found] = physical_memory[j].page_number;
                    number_of_frames_found = number_of_frames_found + 1;
                }
            }

            // Bubble sort the frames based on the page number
            for (int k = 0; k < number_of_frames_found - 1; k++) {
                for (int l = 0; l < number_of_frames_found - k - 1; l++) {
                    if (pages_in_those_frames[l] > pages_in_those_frames[l+1]) {
                        // Swap the pages
                        int temp_p = pages_in_those_frames[l];
                        pages_in_those_frames[l] = pages_in_those_frames[l+1];
                        pages_in_those_frames[l+1] = temp_p;
                        // Swap the frames
                        int temp_f = frames_this_process_owns[l];
                        frames_this_process_owns[l] = frames_this_process_owns[l+1];
                        frames_this_process_owns[l+1] = temp_f;
                    }
                }
            }
            
            // Build the final string like "F1,F5,F6"
            for(int k = 0; k < number_of_frames_found; k++) {
                char temp_string[10];
                if (k < number_of_frames_found - 1) {
                    // If it's not the last one, add a comma
                    sprintf(temp_string, "F%d,", frames_this_process_owns[k]);
                } else {
                    // If it is the last one, no comma
                    sprintf(temp_string, "F%d", frames_this_process_owns[k]);
                }
                strcat(string_for_this_column, temp_string);
            }
        }
        // Print the final string for the column, with padding
        printf(" %-18s", string_for_this_column);
    }
    printf("\n");
    // Make sure everything is written to the file right away
    fflush(stdout);
}

// The main simulation engine
void run_simulation_logic(ReplacementAlgo algo, int num_procs, const int mem_sizes[], const int exec_trace[], int trace_len) {
    // Reset everything for this new simulation run
    initialize_simulation(num_procs, mem_sizes);

    // This pointer keeps track of where we are in the execution list
    int execution_pointer = 0;

    // The simulation output format expects the first instruction to be processed before the main loop starts,
    // This ensures that the first process's first page is loaded into memory and appears in the initial state printout.
    if (trace_len > 0) {
        int first_pid = exec_trace[execution_pointer];
        int first_address = exec_trace[execution_pointer + 1];
        int first_page = first_address / PAGE_SIZE;
        // Place the first page of the first process into the first physical frame (frame 0) at time 0.
        load_page_into_frame(0, first_pid, first_page, 0);
        // Advance the instruction pointer so the main loop starts with the next instruction.
        execution_pointer = execution_pointer + 2;
    }

    // Main loop for each time step
    for (int time_step = 0; time_step < trace_len; time_step++) {
        // First, print the state of memory as it is at the start of this time step
        print_state(time_step, num_procs);

        // Stop if we have reached the end of the instruction list
        if (exec_trace[execution_pointer] == 0 || execution_pointer >= trace_len * 2) {
            break;
        }

        // Get the instruction for this time step
        int current_pid = exec_trace[execution_pointer];
        int current_address = exec_trace[execution_pointer + 1];
        
        // Any memory event (load or access) that happens now is marked with the *next* time step's time
        int time_of_the_event = time_step + 1;

        // Check if the process ID is valid or if the process has already been terminated
        if (current_pid > num_procs || processes[current_pid - 1].terminated == true) {
             // If so, just skip to the next instruction
             execution_pointer = execution_pointer + 2;
             continue;
        }
        
        // Check for Segmentation Fault (accessing memory outside the process's allowed space)
        if (current_address >= processes[current_pid - 1].memory_size) {
            processes[current_pid - 1].terminated = true;
            // When a process dies, all its frames become free
            int i;
            for (i = 0; i < NUM_FRAMES; i++) {
                if (physical_memory[i].process_id == current_pid) {
                    physical_memory[i].process_id = -1; // Mark as free
                }
            }
        } else {
            // If the access is valid, figure out which page is needed
            int needed_page = current_address / PAGE_SIZE;
            
            // See if that page is already in a frame (a "page hit")
            int frame_index = find_page_in_memory(current_pid, needed_page);
            
            if (frame_index != -1) {
                // This is a PAGE HIT. We just need to update the last access time for LRU.
                physical_memory[frame_index].last_access_time = time_of_the_event;
            } else {
                // This is a PAGE FAULT. The page is not in memory.
                
                // First, check if there is a free frame we can use
                int free_frame_index = find_free_frame();
                
                if (free_frame_index != -1) {
                    // A free frame exists, so we use it.
                    load_page_into_frame(free_frame_index, current_pid, needed_page, time_of_the_event);
                } else {
                    // Memory is full. We must replace a page.
                    int victim_frame_index;
                    if (algo == FIFO) {
                        victim_frame_index = find_victim_fifo();
                    } else {
                        victim_frame_index = find_victim_lru();
                    }
                    // Load our new page into the victim's frame
                    load_page_into_frame(victim_frame_index, current_pid, needed_page, time_of_the_event);
                }
            }
        }
        // Move our pointer to the next instruction for the next time step
        execution_pointer = execution_pointer + 2;
    }
}
