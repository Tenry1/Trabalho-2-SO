#include "p2_simulator.h"
#include "inputs_part2.h" // Assuming new inputs are here

// Define the number of inputs
#define NUM_INPUTS 12

int main() {
    SimulationInput inputs[NUM_INPUTS] = {
        {input00, 8}, {input01, 6}, {input02, 5}, {input03, 6}, {input04, 6},
        {input05, 6}, {input06, 5}, {input07, 12}, {input08, 12}, {input09, 12},
        {input10, 12}, {input11, 12}
    };

    for (int i = 0; i < NUM_INPUTS; i++) {
        SimulationSystem system;
        char filename[20];
        snprintf(filename, sizeof(filename), "output2T%02d.out", i);

        FILE* output_file = freopen(filename, "w", stdout);
        if (output_file == NULL) {
            perror("Error opening output file");
            continue;
        }

        initialize_system_with_input(&system, inputs[i]);
        run_simulation(&system);

        // Cleanup any remaining processes (for safety, though run_simulation should handle it)
        for (int j = 0; j < MAX_PROCESSES; j++) {
            if (system.processes[j] != NULL) {
                if (system.processes[j]->instructions) {
                    free(system.processes[j]->instructions);
                }
                free(system.processes[j]);
                system.processes[j] = NULL;
            }
        }

        // Cleanup queues
        if (system.new_queue) deleteQueue(system.new_queue);
        if (system.ready_queue) deleteQueue(system.ready_queue);
        if (system.blocked_queue) deleteQueue(system.blocked_queue);
        if (system.exit_queue) deleteQueue(system.exit_queue);

        fclose(stdout);
    }
    
    // Restore console output
    #ifdef _WIN32
        freopen("CONOUT$", "w", stdout);
    #else
        freopen("/dev/tty", "w", stdout);
    #endif
    printf("Generated output files for %d test cases.\n", NUM_INPUTS);


    return 0;
}